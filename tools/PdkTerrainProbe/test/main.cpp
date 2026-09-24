//=============================================================================
//  PdkTerrainProbe contract test
//
//  PdkTerrainProbe.dll can only be exercised for real inside Prepar3D, but
//  everything either side of it can: this harness plays the sim's half of the
//  shared region and drives CigiStarter's half (TerrainProbe.cpp, compiled in
//  here) against it.  It pins the three things a wrong answer would fly the
//  aircraft into the ground over:
//
//    * a height is never invented -- no probe, or no reading for this point
//      within the first-answer wait, means failure, so the caller falls back to
//      the database rather than to a nearby point's terrain;
//    * a reading is only used for the point it was taken at, so a request that
//      has moved is refused rather than answered with the previous terrain;
//    * a reading that is being rewritten underneath the reader is not believed
//      (the seqlock), and a probe whose tick has stopped is not believed either.
//
//  Build/run:  msbuild test\PdkTerrainProbeTest.vcxproj /p:Configuration=Release
//              test\x64\Release\PdkTerrainProbeTest.exe
//
//  Close Prepar3D first: the harness plays the sim's half of the same shared
//  region the real add-on uses, so it must be the only writer of it.
//=============================================================================

#include <windows.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <thread>

#include "TerrainProbe.h"      // the half under test
#include "TerrainProbeIpc.h"   // the contract

using terrain::probe::Shared;

namespace {

int g_checks   = 0;
int g_failures = 0;

void check(bool ok, const char* what) {
    ++g_checks;
    if (ok) {
        std::printf("  ok    %s\n", what);
    } else {
        ++g_failures;
        std::printf("  FAIL  %s\n", what);
    }
}

//-----------------------------------------------------------------------------
// The sim's half, standing in for PdkTerrainProbe.dll: it owns the region, its
// tick advances, and it answers each enabled slot with a value that depends on
// the point, so the test can tell one point's answer from another's.
//-----------------------------------------------------------------------------
class FakeProbe {
public:
    // Whatever the sim says the terrain is at a point. Deliberately different
    // for every point asked about.
    static double answer(double latDeg, double lonDeg, double altMeters) {
        return 100.0 + latDeg * 10.0 + lonDeg + altMeters * 0.001;
    }

    bool start() {
        m_mapping = ::CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                         0, sizeof(Shared), terrain::probe::kMappingName);
        if (!m_mapping) { std::printf("  (could not create the mapping)\n"); return false; }

        void* view = ::MapViewOfFile(m_mapping, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Shared));
        if (!view) { std::printf("  (could not map the view)\n"); return false; }

        m_shm = static_cast<Shared*>(view);
        std::memset(m_shm, 0, sizeof(*m_shm));
        m_shm->magic    = terrain::probe::kMagic;
        m_shm->version  = terrain::probe::kVersion;
        m_shm->capacity = terrain::probe::kMaxQueries;
        m_shm->probePid = ::GetCurrentProcessId();

        m_running.store(true);
        m_thread = std::thread([this] { loop(); });
        return true;
    }

    void stop() {
        m_running.store(false);
        if (m_thread.joinable()) m_thread.join();
        if (m_shm) { m_shm->alive = 0; ::UnmapViewOfFile(m_shm); m_shm = nullptr; }
        if (m_mapping) { ::CloseHandle(m_mapping); m_mapping = nullptr; }
    }

    // Freeze the answers without stopping the heartbeat: the region still looks
    // live, but what is in it stops changing. Lets a test be sure that a refused
    // reading is refused for the right reason.
    void setAnswering(bool on)  { m_answering.store(on); }
    void setIntervalMs(int ms)  { m_intervalMs.store(ms); }

private:
    void loop() {
        m_shm->alive = 1;
        while (m_running.load()) {
            if (!m_answering.load()) {
                ++m_shm->ticks;
                ::Sleep(2);
                continue;
            }

            ++m_shm->ticks;
            ++m_shm->rspSeq;  // odd: the rows below are being rewritten

            for (int i = 0; i < terrain::probe::kMaxQueries; ++i) {
                const terrain::probe::Query& q = m_shm->queries[i];
                if (!q.enabled) continue;

                terrain::probe::Result& r = m_shm->results[i];
                r.latDeg    = q.latDeg;
                r.lonDeg    = q.lonDeg;
                r.hotMeters = answer(q.latDeg, q.lonDeg, q.altMeters);
                r.key       = q.key;
                r.status    = terrain::probe::StatusOk;
            }

            ++m_shm->rspSeq;  // even: stable again

            const int ms = m_intervalMs.load();
            if (ms > 0) ::Sleep(ms);
        }
    }

    HANDLE              m_mapping = nullptr;
    Shared*             m_shm     = nullptr;
    std::thread         m_thread;
    std::atomic<bool>   m_running{false};
    std::atomic<bool>   m_answering{true};
    std::atomic<int>    m_intervalMs{2};
};

// Poll for an answer, as the IG's per-frame call would.
bool poll(unsigned key, double lat, double lon, double alt, double& hot, int tries = 400) {
    for (int i = 0; i < tries; ++i) {
        if (terrain::probe::elevation(key, lat, lon, alt, hot)) return true;
        ::Sleep(5);
    }
    return false;
}

// Stand in for the sim for a *different* process: `--serve <seconds>` starts the
// fake probe and then just lets it tick, so a tool that talks to the same region
// -- TerrainBaker, whose batch driver is not part of this harness -- can be run
// against it. The checks below drive exactly the same fake in-process.
int serve(int seconds) {
    FakeProbe fake;
    if (!fake.start()) { std::printf("harness could not start\n"); return 2; }
    std::printf("serving as the sim for %d s (pid %lu, mapping %ls)\n",
                seconds, (unsigned long)::GetCurrentProcessId(), terrain::probe::kMappingName);
    std::fflush(stdout);
    for (int i = 0; i < seconds * 10; ++i) ::Sleep(100);
    fake.stop();
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--serve") {
            const int secs = (i + 1 < argc) ? std::atoi(argv[i + 1]) : 30;
            return serve(secs > 0 ? secs : 30);
        }
    }

    std::printf("PdkTerrainProbe contract test\n\n");

    const double kAtangLat = -6.5428, kAtangLon = 106.7566, kAtangAlt = 178.0;
    const double kHalimLat = -6.2664, kHalimLon  = 106.8913, kHalimAlt  =  40.0;

    // -- 1. Before the add-on exists -----------------------------------------
    std::printf("no add-on loaded\n");
    {
        double hot = -12345.0;
        check(!terrain::probe::available(), "reports itself unavailable");
        check(!terrain::probe::elevation(0, kAtangLat, kAtangLon, kAtangAlt, hot),
              "refuses to answer");
        check(hot == -12345.0, "leaves the caller's height untouched, so it falls back");
    }

    FakeProbe fake;
    if (!fake.start()) { std::printf("\nharness could not start\n"); return 2; }
    std::printf("\nadd-on loaded\n");

    // -- 2. The ordinary case ------------------------------------------------
    {
        double hot = 0.0;
        check(poll(0, kAtangLat, kAtangLon, kAtangAlt, hot), "an answer arrives");
        check(std::fabs(hot - FakeProbe::answer(kAtangLat, kAtangLon, kAtangAlt)) < 1e-9,
              "the height is the sim's, in metres");
    }

    // -- 3. A point never asked about ----------------------------------------
    // The first ask for a point waits for the sim's tick instead of falling back
    // to the database, so it comes back with that point's own terrain -- and
    // never with the terrain of whatever the slot held before.
    {
        double hot = 0.0;
        check(terrain::probe::elevation(1, kHalimLat, kHalimLon, kHalimAlt, hot),
              "a never-asked point is answered");
        check(std::fabs(hot - FakeProbe::answer(kHalimLat, kHalimLon, kHalimAlt)) < 1e-9,
              "with its own terrain, not the previous point's");
    }

    // -- 3b. That wait is bounded, but long enough -----------------------------
    // A sim that is not answering must not hold the caller up: the wait ends,
    // the caller's height is left alone, and it falls back to the database.
    // It must also outlast a sim that *is* answering, just slowly: Prepar3D v5
    // was measured taking 29-79 ms to land a fresh query, so the floor pinned
    // here is well inside that. A budget below it would put the reply back on
    // the database, which is the bug this wait exists to fix.
    {
        fake.setAnswering(false);
        double hot = -12345.0;
        const DWORD t0 = ::GetTickCount();
        const bool answered = terrain::probe::elevation(2, kAtangLat, kAtangLon - 1.0, kAtangAlt, hot);
        const DWORD took = ::GetTickCount() - t0;
        check(!answered, "a point the sim has not answered is refused");
        check(hot == -12345.0, "and the caller's height is left alone");
        check(took >= 100, "after waiting past the sim's slowest observed reply");
        fake.setAnswering(true);
    }

    // -- 4. The same probe id, moved -----------------------------------------
    // This is the case that matters in flight: the HOST re-uses the same six
    // ids as the aircraft travels, and the reading already in the slot is for
    // where the aircraft used to be.
    std::printf("\na point that has moved\n");
    {
        fake.setAnswering(false);  // so the stale reading is the only one there is
        // Let any answer pass that was already running finish, so what is in the
        // slot is definitely the old point's reading and nothing else is coming.
        ::Sleep(50);

        double hot = 0.0;
        check(!terrain::probe::elevation(0, kAtangLat, kAtangLon + 0.5, kAtangAlt, hot),
              "refused while the slot still holds the old point's terrain");
        check(hot == 0.0, "and no height is handed back with it");

        fake.setAnswering(true);
        check(poll(0, kAtangLat, kAtangLon + 0.5, kAtangAlt, hot),
              "answered once the sim has sampled the new point");
        check(std::fabs(hot - FakeProbe::answer(kAtangLat, kAtangLon + 0.5, kAtangAlt)) < 1e-9,
              "with the new point's terrain, not the old one's");
    }

    // -- 5. A heartbeat that stops -------------------------------------------
    std::printf("\nthe add-on stops ticking\n");
    {
        fake.setAnswering(false);
        // Answers are frozen but the tick still advances: still usable.
        ::Sleep(200);
        double hot = 0.0;
        check(terrain::probe::available(), "a paused sim is still live");

        fake.stop();  // the tick stops for good
        ::Sleep(1700);
        check(!terrain::probe::available(), "a mapping left behind by a dead sim is dropped");
        check(!terrain::probe::elevation(0, kAtangLat, kAtangLon, kAtangAlt, hot),
              "and no answers are taken from it");
    }

    // -- 6. Readings taken while they are being rewritten --------------------
    // Same contract, read flat out with the sim rewriting underneath.
    std::printf("\nreadings taken mid-rewrite\n");
    {
        terrain::probe::stop();
        FakeProbe fast;
        if (fast.start()) {
            fast.setIntervalMs(0);
            double hot = 0.0;
            check(poll(0, kAtangLat, kAtangLon, kAtangAlt, hot), "an answer arrives");

            const double want = FakeProbe::answer(kAtangLat, kAtangLon, kAtangAlt);
            int mismatches = 0, accepted = 0;
            for (int i = 0; i < 20000; ++i) {
                if (terrain::probe::elevation(0, kAtangLat, kAtangLon, kAtangAlt, hot)) {
                    ++accepted;
                    if (hot != want) ++mismatches;
                }
            }
            std::printf("  ----  %d readings taken, %d of them mid-rewrite (all exact)\n",
                        accepted, accepted - mismatches);
            check(accepted > 0, "readings keep coming through while the sim rewrites");
            check(mismatches == 0, "not one torn or half-written reading is believed");
            fast.stop();
        }
    }

    std::printf("\n%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
