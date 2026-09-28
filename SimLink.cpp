//=============================================================================
//  SimLink.cpp -- the CigiStarter side of the PDK entity writer
//
//  See SimLink.h for what this is for.  It is deliberately small: it owns one
//  block of shared memory and a table of entity rows, and it never talks to
//  Prepar3D itself.
//
//  Threading
//    publish() is called once per Entity Control message, from the CIGI receive
//    thread; health() is called from the window thread.  Every entry point takes
//    a lock, so both may call in at once.  The shared region is written here and
//    read by the add-on's frame callback inside Prepar3D; the per-row seqlock in
//    the contract (SimLinkIpc.h) is the only synchronisation with the other
//    process.
//=============================================================================

#include "SimLink.h"
#include "SimLinkIpc.h"
#include "SimLinkMap.h"   // slotOf(): the one definition of which slot an id lands in

#include <windows.h>

#include <cstring>
#include <mutex>

namespace simlink {
namespace {

HANDLE  g_mapping = nullptr;
Shared* g_shm     = nullptr;

// publish() and health() come in from different threads, so the module's own
// state is guarded.  Recursive so either may use a helper that locks too; the
// lock is uncontended and held for the length of one write.
std::recursive_mutex g_mutex;

bool g_everTried   = false;
DWORD g_nextTryAtMs = 0;

// The status-word watchdog, shared by health() so both judge staleness from the
// same clock.  A mapping left behind by a Prepar3D that has since exited would
// otherwise look usable forever.  `g_mutex` must be held.
uint32_t g_lastTicks  = 0;
DWORD    g_lastChange = 0;

// Re-try the mapping this often while Prepar3D is not up.
constexpr DWORD kReopenIntervalMs = 2000;

// The writer is treated as gone when its heartbeat has not moved for this long.
constexpr DWORD kStaleAfterMs = 1500;

bool tryOpen() {
    if (g_shm) return true;

    const DWORD now = ::GetTickCount();
    if (g_everTried && static_cast<LONG>(now - g_nextTryAtMs) < 0) return false;
    g_everTried   = true;
    g_nextTryAtMs = now + kReopenIntervalMs;

    HANDLE h = ::OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, kMappingName);
    if (!h) return false;

    // Map whatever the add-on created rather than our own idea of the size, so
    // a pair built from different revisions is rejected by the version check
    // below -- with the region left untouched -- instead of failing with an
    // unexplained "not loaded".
    void* view = ::MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!view) {
        ::CloseHandle(h);
        return false;
    }

    Shared* s = static_cast<Shared*>(view);
    if (s->magic != kMagic || s->version != kVersion || s->capacity != kMaxEntities) {
        // Somebody else's mapping, or a version we do not speak.
        ::UnmapViewOfFile(view);
        ::CloseHandle(h);
        return false;
    }

    g_mapping = h;
    g_shm     = s;
    g_lastTicks  = 0;
    g_lastChange = 0;
    return true;
}

// True while the add-on's heartbeat is advancing; false once it has not moved
// for kStaleAfterMs.  `g_mutex` must be held.
bool tickFresh() {
    const uint32_t ticks = g_shm->ticks;
    const DWORD    now   = ::GetTickCount();
    if (ticks != g_lastTicks) {
        g_lastTicks  = ticks;
        g_lastChange = now;
    } else if (static_cast<DWORD>(now - g_lastChange) > kStaleAfterMs) {
        return false;
    }
    return true;
}

} // namespace

void publish(const cigi::EntityControl& e) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    if (!tryOpen()) return;

    // The table is indexed by the low bits of the entity ID (SimLinkMap.h's
    // slotOf(): a power of two, so this is a mask).  Entity IDs in this
    // deployment are a handful, so no two entities map to the same slot; if one
    // ever does, the resident row wins and the new one is dropped rather than
    // silently overwriting another entity's pose.
    Entity& row = g_shm->entities[slotOf(e.entityId)];
    if (row.enabled && row.entityId != e.entityId) return;

    const bool usable = cigi::poseUsable(e);

    // Seqlock: an odd `seq` tells the add-on the row is being rewritten and it
    // should retry rather than trust a half-written pose.  The fields are plain
    // stores between the two bumps; x64 does not reorder stores with stores, and
    // `volatile` keeps the compiler from doing so either.
    ++row.seq;
    row.entityId  = e.entityId;
    row.enabled   = 1;
    row.flags     = usable ? kFlagPoseValid : 0u;
    row.state     = static_cast<uint32_t>(e.stateByte & 0x03u);
    row.latDeg    = e.latDeg;
    row.lonDeg    = e.lonDeg;
    row.altMeters = e.altMeters;
    row.rollDeg   = static_cast<double>(e.rollDeg);
    row.pitchDeg  = static_cast<double>(e.pitchDeg);
    row.yawDeg    = static_cast<double>(e.yawDeg);
    ++row.seq;
}

Health health() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    Health s;
    if (!tryOpen()) return s;   // mappingOpen stays false
    s.mappingOpen = true;

    s.alive          = g_shm->alive != 0;
    s.lastError      = g_shm->lastError;
    s.ticks          = g_shm->ticks;
    s.frameTicks     = g_shm->frameTicks;
    s.objectsCreated = g_shm->objectsCreated;
    s.objectsRemoved = g_shm->objectsRemoved;
    s.createFails    = g_shm->createFails;
    s.setFails       = g_shm->setFails;
    s.applied        = g_shm->applied;

    const uint32_t d = g_shm->diag;
    s.diagnosed = (d & diag::kPresent) != 0;
    if (s.diagnosed) {
        s.hasManager = (d & diag::kHasManager) != 0;
        s.hasTick    = (d & diag::kHasTick) != 0;
        s.noTitle    = (d & diag::kNoTitle) != 0;
    }

    s.ticking = s.alive && tickFresh();
    return s;
}

void start() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    tryOpen();
}

void stop() {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);

    if (g_shm) {
        // Retire every row we published, so the add-on removes the objects it
        // made for them rather than leaving them parked in the sim.
        for (int i = 0; i < kMaxEntities; ++i) {
            if (g_shm->entities[i].enabled) {
                ++g_shm->entities[i].seq;
                g_shm->entities[i].enabled = 0;
                ++g_shm->entities[i].seq;
            }
        }
        ::UnmapViewOfFile(g_shm);
        g_shm = nullptr;
    }
    if (g_mapping) {
        ::CloseHandle(g_mapping);
        g_mapping = nullptr;
    }
    g_everTried   = false;
    g_nextTryAtMs = 0;
    g_lastTicks   = 0;
    g_lastChange  = 0;
}

} // namespace simlink
