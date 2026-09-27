//=============================================================================
//  TerrainSource.cpp — FSUIPC poller feeding terrain::Sample
//
//  See TerrainSource.h for the rationale and the offset table. Everything
//  FSUIPC-facing in this program lives here so the rest of CigiStarter stays
//  unaware of the SDK.
//=============================================================================
#include "TerrainSource.h"

#include <windows.h>            // DWORD / BOOL / BYTE -- must precede the SDK header
#include "FSUIPC_User64.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

namespace terrain {
namespace {

// ---------------------------------------------------------------------------
// FSUIPC offsets. Source of truth: "FSUIPC for Programmers"; the values match
// CigiP3dBridge/src/FsuipcOffsets.h so the two programs read the same fields.
// ---------------------------------------------------------------------------
constexpr DWORD kGroundAltOffset = 0x0020;  // 4 bytes, signed, ground altitude m x 256
constexpr DWORD kLatitudeOffset  = 0x0560;  // 8 bytes, SINT64
constexpr DWORD kLongitudeOffset = 0x0568;  // 8 bytes, SINT64
constexpr DWORD kAltitudeLow     = 0x0570;  // 4 bytes, UINT32, fractional metres
constexpr DWORD kAltitudeHigh    = 0x0574;  // 4 bytes, INT32,  integer metres
// The sim's pause: 0x0264 says whether it is paused, 0x0262 is the lever that
// sets it. Reading the indicator costs nothing extra -- it rides the round trip
// the terrain reads already make -- and the lever is only written when the
// indicator says the sim is running (setAlwaysPause).
constexpr DWORD kPauseIndicator  = 0x0264;  // 2 bytes, 0 = running, 1 = paused
constexpr DWORD kPauseControl    = 0x0262;  // 2 bytes, 0 = run,     1 = pause

// Fixed-point scale factors, identical to CigiP3dBridge's Conversion.h.
constexpr double kLatScale    = 90.0 / (10001750.0 * 65536.0 * 65536.0);
constexpr double kLonScale    = 360.0 / (65536.0 * 65536.0 * 65536.0 * 65536.0);
constexpr double kAltFraction = 1.0 / (65536.0 * 65536.0);

std::atomic<bool> g_running{false};
std::atomic<bool> g_linked{false};   // FSUIPC_Open succeeded and is still live
std::atomic<bool> g_alwaysPause{false};  // hold the sim paused (setAlwaysPause)
std::thread       g_poller;
std::mutex        g_mutex;
Sample            g_sample;
// The pause indicator as it read last poll. Poller thread only. The watchdog
// decides on this rather than on the poll's own reading, because that reading
// does not exist until Process() has returned -- by which time the write has
// already gone. It starts false so the very first poll asserts the pause
// instead of waiting a round trip to be told the sim is running.
bool              g_lastPaused = false;

// One FSUIPC read round trip into `out`. Poller thread only: FSUIPC's request
// buffer is process-global, so this must never race with another reader.
bool pollOnce(Sample& out) {
    DWORD    dw      = 0;
    int32_t  ground  = 0;
    long long latRaw = 0;
    long long lonRaw = 0;
    uint32_t altLow  = 0;
    int32_t  altHigh = 0;
    uint16_t paused  = 0;
    uint16_t pauseOn = 1;

    // All the reads are queued first and then committed by a single Process();
    // the destination buffers must stay alive until it returns, so they are
    // plain locals of this function.
    if (!FSUIPC_Read(kGroundAltOffset, 4, &ground,  &dw) ||
        !FSUIPC_Read(kLatitudeOffset,  8, &latRaw,  &dw) ||
        !FSUIPC_Read(kLongitudeOffset, 8, &lonRaw,  &dw) ||
        !FSUIPC_Read(kAltitudeLow,     4, &altLow,  &dw) ||
        !FSUIPC_Read(kAltitudeHigh,    4, &altHigh, &dw) ||
        !FSUIPC_Read(kPauseIndicator,  2, &paused,  &dw)) {
        return false;
    }

    // Hold the sim paused, whoever unpaused it: assert the lever on the
    // strength of the previous poll's reading, so the write rides this round
    // trip. One write per unpause -- an already-paused sim is not written to.
    if (g_alwaysPause.load() && !g_lastPaused) {
        FSUIPC_Write(kPauseControl, 2, &pauseOn, &dw);
    }

    if (!FSUIPC_Process(&dw)) {
        return false;
    }

    out.connected         = true;
    out.groundAltMeters   = static_cast<double>(ground) / 256.0;
    out.aircraftAltMeters = static_cast<double>(altHigh) +
                            static_cast<double>(altLow) * kAltFraction;
    out.latDeg            = static_cast<double>(latRaw) * kLatScale;
    out.lonDeg            = static_cast<double>(lonRaw) * kLonScale;
    out.paused            = (paused != 0);
    g_lastPaused          = out.paused;   // what the next poll's write decides on
    return true;
}

void publish(const Sample& s) {
    std::lock_guard<std::mutex> lk(g_mutex);
    g_sample = s;
}

void markDisconnected() {
    std::lock_guard<std::mutex> lk(g_mutex);
    g_sample.connected = false;
}

void pollLoop(int intervalMs) {
    const auto period = std::chrono::milliseconds(intervalMs > 0 ? intervalMs : 50);
    uint64_t tick = 0;

    while (g_running.load()) {
        // (Re)connect if we are not linked yet. Cheap when the sim is down, so
        // CigiStarter may be started before Prepar3D.
        if (!g_linked.load()) {
            DWORD dw = 0;
            if (FSUIPC_Open(SIM_ANY, &dw)) {
                g_linked.store(true);
            }
        }

        if (g_linked.load()) {
            Sample s;
            if (pollOnce(s)) {
                s.tick = ++tick;
                publish(s);
            } else {
                // Sim gone (or FSUIPC unloaded): drop the link and retry.
                FSUIPC_Close();
                g_linked.store(false);
                markDisconnected();
            }
        }

        std::this_thread::sleep_for(period);
    }
}

} // namespace

bool start(int pollIntervalMs) {
    if (g_running.exchange(true)) {
        return false; // already polling
    }
    g_poller = std::thread(pollLoop, pollIntervalMs);
    return true;
}

void stop() {
    if (!g_running.exchange(false)) {
        return; // never started, or already stopped
    }
    if (g_poller.joinable()) {
        g_poller.join();
    }
    if (g_linked.exchange(false)) {
        FSUIPC_Close();
    }
    // The poller has gone, so the cached sample must stop claiming a live link:
    // callers show "no FSUIPC link" rather than terrain that is now stale.
    markDisconnected();
    // Forget the pause reading too: the next start() must assert the pause
    // again rather than trust a reading from the run that has just ended.
    g_lastPaused = false;
}

void setAlwaysPause(bool on) {
    g_alwaysPause.store(on);
}

Sample latest() {
    std::lock_guard<std::mutex> lk(g_mutex);
    return g_sample;
}

} // namespace terrain
