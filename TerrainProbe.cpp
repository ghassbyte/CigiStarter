//=============================================================================
//  TerrainProbe.cpp -- the CigiStarter side of the PDK terrain probe
//
//  See TerrainProbe.h for what this is for.  It is deliberately small: it owns
//  one block of shared memory and a table of slots, and it never talks to
//  Prepar3D itself.
//
//  Threading
//    The HAT/HOT answering path calls elevation() once per request, from the
//    transmitting thread, and the window calls available() for its status block
//    from its own. Every entry point takes a lock, so both may call in at once.
//    The shared region is written by the transmitting thread and read by the
//    add-on's simulation callback inside Prepar3D; the seqlock in the contract
//    (TerrainProbeIpc.h) is the only synchronisation with the other process.
//=============================================================================

#include "TerrainProbe.h"
#include "TerrainProbeIpc.h"

#include <windows.h>

#include <cmath>
#include <cstring>
#include <mutex>

namespace terrain {
namespace probe {
namespace {

HANDLE  g_mapping = nullptr;
Shared* g_shm     = nullptr;

// The transmitting thread calls elevation() once per HAT/HOT request and the
// window thread calls available() for the status block, so the module's own
// state is guarded. Recursive so elevation() can use available() unchanged; the
// lock is uncontended and held for the length of one lookup.
std::recursive_mutex g_mutex;

bool  g_everTried     = false;
DWORD g_nextTryAtMs   = 0;

// Which caller `key` owns each slot, and when its reading was last seen to
// advance. A slot is only re-used when the probe has stopped answering for it,
// so the HOST's six probes settle into six slots and stay there.
unsigned g_slotKey[kMaxQueries] = {};
bool     g_slotUsed[kMaxQueries] = {};

// The last reading accepted for each slot, and the point it was taken at. Used
// only when the sim is caught mid-rewrite and the fresh read cannot be trusted:
// a reading for the *same* point is at most one tick old, which is much better
// than making the caller fall back to a different terrain source for a frame.
double g_slotHot[kMaxQueries]    = {};
double g_slotHotLat[kMaxQueries] = {};
double g_slotHotLon[kMaxQueries] = {};
double g_slotHotAlt[kMaxQueries] = {};
bool   g_slotHasHot[kMaxQueries] = {};
bool   g_slotProbed[kMaxQueries] = {};  // the sim has answered this slot's query

// How far a reading may be from the point that was asked about and still count
// as an answer to it. The HOST's probes sit within a few metres of the
// aircraft, so at 50-100 Hz a one-tick-old reading is a couple of metres out;
// anything beyond this is a reading for somewhere else and must be refused.
constexpr double kEchoToleranceDeg  = 1e-4;  // ~11 m
constexpr double kAltToleranceMeters = 5.0;  // the sim's answer is layer-dependent

// Re-try the mapping this often while Prepar3D is not up.
constexpr DWORD kReopenIntervalMs = 2000;

// The port never ticks for this long and we treat it as gone.
constexpr DWORD kStaleAfterMs = 1500;

// A point the sim has not been asked about before is only answered on one of its
// own ticks, and a tick that lands a fresh query arrives on the order of 30-60 ms
// after the write was made -- measured against Prepar3D v5, whose callback also
// re-queries every enabled slot on each pass, so the worst seen was 79 ms. An
// immediate read misses that first reply and drops the caller onto the offline
// database; wait this long instead, so the wait covers the worst case and still
// ends the moment the answer lands. Only ever paid once per point -- every later
// reply is already in the table.
constexpr DWORD kFirstAnswerWaitMs = 120;

bool tryOpen() {
    if (g_shm) return true;

    const DWORD now = ::GetTickCount();
    if (g_everTried && static_cast<LONG>(now - g_nextTryAtMs) < 0) return false;
    g_everTried   = true;
    g_nextTryAtMs = now + kReopenIntervalMs;

    HANDLE h = ::OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, kMappingName);
    if (!h) return false;

    void* view = ::MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Shared));
    if (!view) {
        ::CloseHandle(h);
        return false;
    }

    Shared* s = static_cast<Shared*>(view);
    if (s->magic != kMagic || s->version != kVersion || s->capacity != kMaxQueries) {
        // Somebody else's mapping, or a version we do not speak.
        ::UnmapViewOfFile(view);
        ::CloseHandle(h);
        return false;
    }

    g_mapping = h;
    g_shm     = s;
    std::memset(g_slotUsed, 0, sizeof(g_slotUsed));
    std::memset(g_slotHasHot, 0, sizeof(g_slotHasHot));
    std::memset(g_slotProbed, 0, sizeof(g_slotProbed));
    return true;
}

} // namespace

bool available() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    if (!tryOpen()) return false;
    if (!g_shm->alive) return false;

    // The tick has to be advancing: a mapping left behind by a Prepar3D that
    // has since exited would otherwise look usable forever.
    static uint32_t lastTicks  = 0;
    static DWORD    lastChange = 0;

    const uint32_t ticks = g_shm->ticks;
    const DWORD    now   = ::GetTickCount();
    if (ticks != lastTicks) {
        lastTicks  = ticks;
        lastChange = now;
    } else if (static_cast<DWORD>(now - lastChange) > kStaleAfterMs) {
        return false;
    }
    return true;
}

bool elevation(unsigned key, double latDeg, double lonDeg, double altMeters, double& hotMeters) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    if (!available()) return false;

    unsigned slot = kMaxQueries;
    for (unsigned i = 0; i < kMaxQueries; ++i) {
        if (g_slotUsed[i] && g_slotKey[i] == key) { slot = i; break; }
    }
    if (slot == kMaxQueries) {
        for (unsigned i = 0; i < kMaxQueries; ++i) {
            if (!g_slotUsed[i]) {
                slot            = i;
                g_slotUsed[i]   = true;
                g_slotKey[i]    = key;
                g_slotProbed[i] = false;  // nothing has been asked of the sim yet
                break;
            }
        }
    }
    if (slot == kMaxQueries) return false;  // more callers than slots

    Query& q = g_shm->queries[slot];
    q.latDeg    = latDeg;
    q.lonDeg    = lonDeg;
    q.altMeters = altMeters;
    q.key       = key;
    q.enabled   = 1;
    ++g_shm->reqSeq;

    // Seqlock read: take the version before and after, and only believe the row
    // if it did not move underneath us.
    bool   fresh    = false;
    double freshHot = 0.0;

    // A point the sim has not answered yet takes a tick before it has an answer
    // at all, so wait for that first reply rather than falling through to the
    // offline database. Only for the first reply, though: once the sim has
    // answered this slot -- refused points included -- every later reply is
    // already in the row, so steady state never waits.
    const bool  firstAnswer = !g_slotProbed[slot];
    const DWORD waitUntil   = ::GetTickCount() +
                              (firstAnswer ? kFirstAnswerWaitMs : 0);

    for (int attempt = 0; !fresh; ++attempt) {
        const uint32_t before = g_shm->rspSeq;

        if (!(before & 1u)) {                       // the sim is not mid-rewrite
            Result r;
            std::memcpy(&r, &g_shm->results[slot], sizeof(r));

            if (g_shm->rspSeq == before &&          // it did not move while we read
                r.key == key &&
                std::fabs(r.latDeg - latDeg) <= kEchoToleranceDeg &&
                std::fabs(r.lonDeg - lonDeg) <= kEchoToleranceDeg) {
                if (r.status == StatusOk) {
                    g_slotProbed[slot] = true;
                    freshHot = r.hotMeters;
                    fresh    = true;
                } else if (r.status != StatusNone) {
                    g_slotProbed[slot] = true;      // answered, just refused
                    if (firstAnswer) break;
                }
                // StatusNone: the sim has not looked at it yet; keep waiting.
            }
        }
        if (fresh) break;

        if (firstAnswer) {
            if (::GetTickCount() >= waitUntil) break;
            ::Sleep(1);                             // let the sim's tick land
        } else if (attempt >= 3) {
            break;                                  // the cache below covers it
        } else {
            ::Sleep(0);
        }
    }

    if (fresh) {
        g_slotHot[slot]    = freshHot;
        g_slotHotLat[slot] = latDeg;
        g_slotHotLon[slot] = lonDeg;
        g_slotHotAlt[slot] = altMeters;
        g_slotHasHot[slot] = true;
        hotMeters = freshHot;
        return true;
    }

    // Could not get a clean read of the sim's row. A reading already held for
    // this same point is one tick old at worst, so use it rather than dropping
    // the caller onto the database for a frame.
    if (g_slotHasHot[slot] &&
        std::fabs(g_slotHotLat[slot] - latDeg) <= kEchoToleranceDeg &&
        std::fabs(g_slotHotLon[slot] - lonDeg) <= kEchoToleranceDeg &&
        std::fabs(g_slotHotAlt[slot] - altMeters) <= kAltToleranceMeters) {
        hotMeters = g_slotHot[slot];
        return true;
    }

    return false;
}

void start() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    tryOpen();
}

void stop() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    if (g_shm) {
        // Leave nothing behind for the probe to keep querying.
        for (unsigned i = 0; i < kMaxQueries; ++i) {
            if (g_slotUsed[i]) g_shm->queries[i].enabled = 0;
        }
        ::UnmapViewOfFile(g_shm);
        g_shm = nullptr;
    }
    if (g_mapping) {
        ::CloseHandle(g_mapping);
        g_mapping = nullptr;
    }
    g_everTried = false;
    g_nextTryAtMs = 0;
    std::memset(g_slotUsed, 0, sizeof(g_slotUsed));
    std::memset(g_slotHasHot, 0, sizeof(g_slotHasHot));
    std::memset(g_slotProbed, 0, sizeof(g_slotProbed));
}

} // namespace probe
} // namespace terrain
