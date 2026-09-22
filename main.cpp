//=============================================================================
//  CigiStarter — minimal CIGI 3.3 Image Generator (IG) for a new IG PC
//
//  Role
//    Plays the "Master IG" side of a CIGI link so the HOST begins streaming
//    flight data. The HOST stays silent until the IG transmits Start-of-Frame
//    (SOF) messages, so this program continuously sends SOF at a configurable
//    rate and answers HAT/HOT requests with the terrain height read from the
//    local Prepar3D (switched on by default; an unreachable sim just means the
//    answers stay invalid until it appears).
//
//  Transport (fixed by the deployment; hardcoded below)
//    HOST 192.168.70.2  -> IG 192.168.70.70 : 8000   (downstream data)
//    IG   192.168.70.70 -> HOST 192.168.70.2 : 8001  (upstream SOF / responses)
//
//  Operator interface
//    A small fixed-size Win32 window (Gui.cpp) showing the link, the live
//    stream statistics and the terrain sample, with START / STOP / EXIT plus
//    a HAT/HOT check box and a SOF rate box.
//    EXIT is the only way out -- the title-bar X and Alt+F4 are swallowed, so
//    the IG cannot be taken off the link by a stray click. This file owns the
//    worker threads and the statistics that window displays.
//
//  Dependencies
//    Windows sockets, the 1 ms timer and user32 (Ws2_32.lib, Winmm.lib,
//    User32.lib). While HAT/HOT answering is switched on (the default) the
//    FSUIPC SDK is linked as well, to read the terrain height out of the local
//    Prepar3D (isolated in TerrainSource.cpp).
//
//  Usage
//    CigiStarter.exe [--fps 100] [--ig-mode 0] [--no-respond-hathot] [--help]
//    Being a windowed program there is no console, so --help and any startup
//    error are reported in a message box.
//=============================================================================
#include "cigi.h"
#include "Gui.h"
#include "TerrainSource.h"

#include <winsock2.h>
#include <ws2tcpip.h>

// <windows.h> must come after <winsock2.h>, never before: windows.h otherwise
// pulls in the v1 winsock.h and every socket type gets redeclared.
#include <windows.h>
#include <mmsystem.h>  // timeBeginPeriod / timeEndPeriod (high-resolution timer)

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

//-----------------------------------------------------------------------------
// Fixed link configuration (deployment-specific, hardcoded)
//-----------------------------------------------------------------------------
constexpr char     kHostIp[] = "192.168.70.2";   // HOST address (upstream SOF)
constexpr uint16_t kHostPort = 8001;             // HOST port (IG -> HOST)
constexpr char     kIgIp[]   = "192.168.70.70";  // this IG's address (identity)
constexpr uint16_t kIgPort   = 8000;             // IG port (HOST -> IG)

//-----------------------------------------------------------------------------
// Runtime configuration (command-line adjustable)
//-----------------------------------------------------------------------------
struct Config {
    int     fps           = 100;  // SOF transmission rate
    uint8_t igMode        = 0;    // 0 = Standby (0x30), 1 = Operate (0x31)
    bool    respondHatHot = true;
};

// Most HOSTs send one HAT/HOT request per frame, so one response per SOF. The
// cap only matters if a HOST floods us; it keeps the upstream datagram inside
// a 576-byte payload so it can never be fragmented.
constexpr int kMaxResponsesPerFrame = 8;

//-----------------------------------------------------------------------------
// Shared runtime statistics (written by the receive thread, read elsewhere)
//-----------------------------------------------------------------------------
struct Stats {
    std::atomic<uint64_t> datagrams{0};
    std::atomic<uint64_t> bytes{0};
    std::atomic<uint64_t> messages{0};
    std::atomic<uint64_t> igControl{0};
    std::atomic<uint64_t> entityControl{0};
    std::atomic<uint64_t> hatHotReq{0};
    std::atomic<uint32_t> lastHostFrame{0};
    // SOF frames this program has transmitted. The transmit thread owns the
    // counter; publishing it here is what lets the window show the TX rate.
    std::atomic<uint32_t> sofFrames{0};
    // IG mode the HOST last commanded in an IG Control packet. The IG does not
    // get to pick its own mode -- the SOF reports this value back (see
    // cigi::sofIgModeByte), which is how a real IG behaves.
    std::atomic<uint8_t>  hostIgMode{0};
};

// Hat/Hot responses are queued by the receive thread and flushed by the SOF
// thread so they ride along on the upstream stream (as in the capture).
struct HatHotRequestQueue {
    std::mutex                       mtx;
    std::vector<cigi::HatHotRequest> items;
};

// A request the SOF thread currently owes an answer for. One-shot requests
// leave the list once answered; requests carrying an update period stay and
// are re-answered every `updatePeriod` frames.
struct ActiveHatHotRequest {
    cigi::HatHotRequest req;
    uint32_t            framesSince = 0;
};

// Process-wide state, shared between the worker threads and the operator window.
static std::atomic<bool> g_running{true};
static std::atomic<bool> g_sending{false};  // SOF stream on/off (START / STOP)
static Stats             g_stats;           // filled in by the receive thread
// Answering HAT/HOT (which also decides whether the terrain poller runs) and
// the SOF transmit rate. Both are seeded from the command line and from then on
// owned by the window's controls -- the window thread cannot see main()'s
// Config, so the live values live here.
static std::atomic<bool> g_respondHatHot{true};
static std::atomic<int>  g_targetFps{100};

//-----------------------------------------------------------------------------
// Error / help reporting
//
// CigiStarter is a windowed program, so there is no console to print to: a
// startup failure -- or --help -- has to appear in a message box instead.
// fail() always returns 1, so call sites read `return fail(...)`.
//-----------------------------------------------------------------------------
static int fail(const char* fmt, ...) {
    char msg[1024];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    MessageBoxA(nullptr, msg, "CigiStarter - error", MB_OK | MB_ICONERROR);
    return 1;
}

static void buildUsage(char* buf, size_t n, const char* argv0) {
    std::snprintf(
        buf, n,
        "CigiStarter - minimal CIGI 3.3 Image Generator\n\n"
        "Link (fixed): HOST %s:%u  <->  IG %s:%u\n\n"
        "Usage: %s [options]\n\n"
        "Options (these only set the starting state -- the SOF rate and HAT/HOT\n"
        "answering can also be changed from the window while it runs):\n\n"
        "  --fps <rate>        SOF transmit rate in Hz           (default 100)\n"
        "  --ig-mode <0..3>    IG mode reported in the SOF until the HOST sends\n"
        "                      its first IG Control (0=Standby 1=Operate)\n"
        "                                                         (default 0)\n"
        "  --respond-hathot    answer HAT/HOT requests, reporting the terrain\n"
        "                      height read from the local Prepar3D via FSUIPC\n"
        "                      (this is the default)\n"
        "  --no-respond-hathot do not answer HAT/HOT requests, and leave FSUIPC\n"
        "                      untouched\n"
        "  --help              show this help\n",
        kHostIp, kHostPort, kIgIp, kIgPort, argv0);
}

static void showUsage(const char* argv0) {
    char msg[1024];
    buildUsage(msg, sizeof(msg), argv0);
    MessageBoxA(nullptr, msg, "CigiStarter", MB_OK);
}

//-----------------------------------------------------------------------------
// Minimal command-line parser
//-----------------------------------------------------------------------------
static bool parseArgs(int argc, char** argv, Config& cfg) {
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                fail("error: %s requires a value", name);
                return nullptr;
            }
            return argv[++i];
        };
        if (a == "--help" || a == "-h") {
            showUsage(argv[0]);
            return false;
        } else if (a == "--fps") {
            const char* v = next("--fps"); if (!v) return false;
            cfg.fps = std::atoi(v);
        } else if (a == "--ig-mode") {
            const char* v = next("--ig-mode"); if (!v) return false;
            cfg.igMode = static_cast<uint8_t>(std::atoi(v));
        } else if (a == "--respond-hathot") {
            cfg.respondHatHot = true;   // the default; kept so old command lines work
        } else if (a == "--no-respond-hathot") {
            cfg.respondHatHot = false;
        } else {
            fail("error: unknown option '%s'\n\nRun with --help for the option list.",
                 a.c_str());
            return false;
        }
    }
    if (cfg.fps < gui::kMinFps || cfg.fps > gui::kMaxFps) {
        fail("error: --fps must be in [%d, %d]", gui::kMinFps, gui::kMaxFps);
        return false;
    }
    if (cfg.igMode > 3) {
        fail("error: --ig-mode must be in [0, 3]");
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
// Resolve an IPv4 address string into a sockaddr_in (returns false on error)
//-----------------------------------------------------------------------------
static bool resolveV4(const char* ip, uint16_t port, sockaddr_in& out) {
    std::memset(&out, 0, sizeof(out));
    out.sin_family = AF_INET;
    out.sin_port = htons(port);
    return inet_pton(AF_INET, ip, &out.sin_addr) == 1;
}

//-----------------------------------------------------------------------------
// HAT/HOT answering
//-----------------------------------------------------------------------------
// FSUIPC exposes the terrain height under the *player* aircraft and nothing
// else, so that is the only test point we can answer for. An entity-relative
// request must therefore be for entity 0 -- the aircraft CigiP3dBridge drives
// into Prepar3D -- and a geodetic request must be close enough to the aircraft
// that the ground underneath is the same terrain.
constexpr double kGeodeticToleranceMeters = 500.0;
constexpr double kPi                      = 3.14159265358979323846;
constexpr double kEarthRadiusMeters       = 6371000.0;

static bool nearAircraft(double latDeg, double lonDeg, const terrain::Sample& sim) {
    const double dLat    = (latDeg - sim.latDeg) * kPi / 180.0;
    const double meanLat = (latDeg + sim.latDeg) * 0.5 * kPi / 180.0;
    const double dLon    = (lonDeg - sim.lonDeg) * kPi / 180.0 * std::cos(meanLat);
    const double north   = kEarthRadiusMeters * dLat;
    const double east    = kEarthRadiusMeters * dLon;
    return (north * north + east * east) <=
           (kGeodeticToleranceMeters * kGeodeticToleranceMeters);
}

// The answer to one HAT/HOT request. `valid` follows the ICD: it is cleared
// whenever the IG cannot return the requested datum, which here means "we have
// no terrain to report" (no FSUIPC link, a request for another entity, or a
// geodetic point too far from the aircraft). Answering Valid = 0 makes the HOST
// ignore the packet -- far better than a fabricated HOT of 0, which the HOST
// would follow straight into the ground.
struct HatHotAnswer {
    bool   valid = false;
    double hat   = 0.0;  // metres above the terrain surface
    double hot   = 0.0;  // metres MSL
};

static HatHotAnswer answerHatHot(const cigi::HatHotRequest& req,
                                 const terrain::Sample& sim) {
    HatHotAnswer ans;
    if (!sim.connected) {
        return ans;
    }

    double testAlt;
    if (req.coordSystem == cigi::kHatHotEntity) {
        if (req.entityId != 0) {
            return ans;  // not the aircraft FSUIPC knows about
        }
        // Entity axes are local NED: +Z points out of the bottom, so a
        // positive Z offset sits *below* the reference point.
        testAlt = sim.aircraftAltMeters - req.z;
    } else {
        if (!nearAircraft(req.x, req.y, sim)) {
            return ans;
        }
        testAlt = req.z;  // geodetic requests carry the altitude directly
    }

    ans.hot   = sim.groundAltMeters;
    ans.hat   = testAlt - ans.hot;
    ans.valid = true;
    return ans;
}

//-----------------------------------------------------------------------------
// Window callbacks
//
// All three run on the window's thread. The window owns no state of its own,
// so it polls the statistics through uiStatus() and pokes the two atomics the
// worker threads already watch.
//-----------------------------------------------------------------------------
static gui::Status uiStatus() {
    gui::Status s;
    s.sending           = g_sending.load();
    s.respondingHatHot  = g_respondHatHot.load();
    s.datagrams         = g_stats.datagrams.load();
    s.entityUpdates     = g_stats.entityControl.load();
    s.sofFrames         = g_stats.sofFrames.load();
    s.targetFps         = g_targetFps.load();
    s.hostIgMode        = g_stats.hostIgMode.load();
    s.lastHostFrame     = g_stats.lastHostFrame.load();

    const terrain::Sample sim = terrain::latest();
    s.terrainConnected  = sim.connected;
    s.groundAltMeters   = sim.groundAltMeters;
    s.aircraftAltMeters = sim.aircraftAltMeters;
    return s;
}

static void uiSetSending(bool on) { g_sending.store(on); }
static void uiRequestQuit()       { g_running.store(false); }

// Switching HAT/HOT answering on and off starts and stops the terrain poller
// with it: the poller is the only thing in this process that touches FSUIPC and
// there is no point reading the sim while the answer goes unused. It reconnects
// on its own, so toggling is safe in any order (including before Prepar3D is
// running, where it just retries).
static void uiSetRespondHatHot(bool on) {
    if (g_respondHatHot.exchange(on) == on) {
        return;  // no change
    }
    if (on) {
        terrain::start();
    } else {
        terrain::stop();
    }
}

// The window range-checks the rate box first; this is belt and braces, because
// a bad rate would either stall the transmit loop or make it spin.
static void uiSetFps(int fps) {
    if (fps < gui::kMinFps || fps > gui::kMaxFps) {
        return;
    }
    g_targetFps.store(fps);
}

int main(int argc, char** argv) {
    Config cfg;
    if (!parseArgs(argc, argv, cfg)) {
        return 1;
    }

    // Initialise Winsock.
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return fail("error: WSAStartup failed (%d)\n", WSAGetLastError());
    }

    // Raise the system timer resolution to 1 ms for the lifetime of the
    // process. Windows' default scheduler granularity is 15.6 ms (~64 Hz), so
    // at any SOF rate above ~60 Hz sleep_until() overshoots its deadline and
    // the transmit loop emits bursts of back-to-back SOF datagrams. The HOST
    // cannot absorb those bursts and drops out. timeBeginPeriod(1) keeps the
    // 100/112 Hz pacing even, matching the reference Master IG.
    timeBeginPeriod(1);

    // ---- Receive socket: bound to the port the HOST streams to. ----------
    // Bind to INADDR_ANY (0.0.0.0) rather than 192.168.70.70 so the program
    // starts even if that address is not yet assigned to a local NIC; it still
    // receives datagrams addressed to 192.168.70.70:8000 once configured.
    SOCKET rx = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (rx == INVALID_SOCKET) {
        WSACleanup();
        return fail("error: socket() failed (%d)\n", WSAGetLastError());
    }
    sockaddr_in bindAddr{};
    bindAddr.sin_family      = AF_INET;
    bindAddr.sin_port        = htons(kIgPort);
    bindAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(rx, reinterpret_cast<sockaddr*>(&bindAddr), sizeof(bindAddr)) == SOCKET_ERROR) {
        const int err = WSAGetLastError();
        closesocket(rx); WSACleanup();
        return fail("error: bind(0.0.0.0:%u) failed (%d)\n"
                    "  (is another program already using UDP port %u?)\n",
                    kIgPort, err, kIgPort);
    }
    // Short receive timeout so the loop can observe g_running on shutdown.
    DWORD rxTimeout = 500;
    setsockopt(rx, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&rxTimeout),
               sizeof(rxTimeout));

    // ---- Send socket: transmits SOF to the HOST. -------------------------
    SOCKET tx = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (tx == INVALID_SOCKET) {
        closesocket(rx); WSACleanup();
        return fail("error: socket() failed (%d)\n", WSAGetLastError());
    }
    sockaddr_in hostAddr;
    if (!resolveV4(kHostIp, kHostPort, hostAddr)) {
        closesocket(rx); closesocket(tx); WSACleanup();
        return fail("error: invalid host address '%s'\n", kHostIp);
    }

    // Seed the window's controls; from here on they are the live state.
    g_respondHatHot.store(cfg.respondHatHot);
    g_targetFps.store(cfg.fps);

    // The IG does not choose its own mode: it reports back whatever the HOST
    // last commanded. --ig-mode therefore only seeds the value used until the
    // first IG Control arrives, after which the receive thread takes over.
    g_stats.hostIgMode.store(static_cast<uint8_t>(cfg.igMode & 0x03u));

    // The terrain poller owns every FSUIPC call in this process, so it runs
    // only while HAT/HOT answering is enabled -- which it is by default, since
    // a wrong or missing HOT flies the aircraft into the ground. The window's
    // check box turns it on and off at runtime.
    if (g_respondHatHot.load()) {
        terrain::start();
    }

    HatHotRequestQueue hatHot;

    // ---- Receive thread: parse inbound CIGI and update statistics. -------
    std::thread rxThread([&] {
        std::vector<uint8_t> buf(65536);
        while (g_running.load()) {
            sockaddr_in from;
            int fromLen = sizeof(from);
            const int n = recvfrom(rx, reinterpret_cast<char*>(buf.data()),
                                   static_cast<int>(buf.size()), 0,
                                   reinterpret_cast<sockaddr*>(&from), &fromLen);
            if (n == SOCKET_ERROR) {
                const int err = WSAGetLastError();
                if (err == WSAETIMEDOUT || err == WSAEWOULDBLOCK) {
                    continue; // poll timeout; check g_running again
                }
                if (!g_running.load()) break;
                std::fprintf(stderr, "recvfrom error (%d)\n", err);
                continue;
            }
            if (n < 2) continue;

            g_stats.datagrams.fetch_add(1);
            g_stats.bytes.fetch_add(static_cast<uint64_t>(n));

            cigi::forEachMessage(buf.data(), static_cast<size_t>(n),
                                 [&](uint8_t id, const uint8_t* msg, size_t msgSize) {
                g_stats.messages.fetch_add(1);
                switch (id) {
                    case cigi::MsgIGControl: {
                        g_stats.igControl.fetch_add(1);
                        // IGCtrl layout (16 bytes): [id][size][ver][db][mode]
                        // [pad][byte-swap u16][frame-ctr u32][timestamp u32].
                        //
                        // Bits 0-1 of byte 4 are the mode the HOST is putting
                        // the IG into. The IG mirrors it back in the SOF rather
                        // than choosing for itself, so record it here.
                        if (msgSize >= 5) {
                            g_stats.hostIgMode.store(
                                static_cast<uint8_t>(msg[4] & 0x03u));
                        }
                        // The host frame counter is a uint32 LE at full-message
                        // offset 8; echo it back in the SOF's last-host-frame.
                        if (msgSize >= 12) {
                            const uint32_t fc = static_cast<uint32_t>(msg[8]) |
                                                (static_cast<uint32_t>(msg[9]) << 8) |
                                                (static_cast<uint32_t>(msg[10]) << 16) |
                                                (static_cast<uint32_t>(msg[11]) << 24);
                            g_stats.lastHostFrame.store(fc);
                        }
                        break;
                    }
                    case cigi::MsgEntityControl:
                        g_stats.entityControl.fetch_add(1);
                        break;
                    case cigi::MsgHATHOTRequest: {
                        g_stats.hatHotReq.fetch_add(1);
                        // Decode the whole request here; the SOF thread turns it
                        // into an answer against the latest terrain sample.
                        cigi::HatHotRequest req;
                        if (g_respondHatHot.load() &&
                            cigi::parseHatHotRequest(msg, msgSize, req)) {
                            std::lock_guard<std::mutex> lk(hatHot.mtx);
                            if (hatHot.items.size() < 64) {
                                hatHot.items.push_back(req);
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
            });
        }
    });

    // ---- SOF thread: transmit Start-of-Frame at the configured rate. -----
    std::thread txThread([&] {
        uint8_t sof[24];
        uint32_t frame = 0;
        const auto started = std::chrono::steady_clock::now();
        // The rate is re-read every frame, so the window can retune the stream
        // without stopping it.
        int  fps    = g_targetFps.load();
        auto period = std::chrono::microseconds(1000000 / fps);
        auto next   = std::chrono::steady_clock::now();

        // HAT/HOT requests this thread still owes an answer for. Kept across
        // frames so requests with an update period can be answered repeatedly.
        std::vector<ActiveHatHotRequest> hatHotActive;

        while (g_running.load()) {
            // Follow a rate change made in the window. The schedule is resynced
            // rather than kept, so a retune cannot fire a burst of catch-up
            // frames the way a rate *increase* would.
            const int wantedFps = g_targetFps.load();
            if (wantedFps != fps) {
                fps    = wantedFps;
                period = std::chrono::microseconds(1000000 / fps);
                next   = std::chrono::steady_clock::now();
            }

            if (g_sending.load()) {
                const auto now = std::chrono::steady_clock::now();
                const uint32_t ts = static_cast<uint32_t>(
                    std::chrono::duration_cast<std::chrono::microseconds>(now - started).count());

                cigi::packStartOfFrame(sof, frame, ts, g_stats.lastHostFrame.load(),
                                       cigi::sofIgModeByte(g_stats.hostIgMode.load()));

                // Build the upstream datagram: the SOF followed by every HAT/HOT
                // response that is due this frame, mirroring the reference
                // capture's upstream framing.
                uint8_t pkt[24 + kMaxResponsesPerFrame * cigi::kHatHotExtRespSize];
                std::memcpy(pkt, sof, 24);
                size_t used = 24;

                if (g_respondHatHot.load()) {
                    // Adopt the requests the receive thread decoded.
                    {
                        std::lock_guard<std::mutex> lk(hatHot.mtx);
                        for (const cigi::HatHotRequest& r : hatHot.items) {
                            auto it = std::find_if(
                                hatHotActive.begin(), hatHotActive.end(),
                                [&](const ActiveHatHotRequest& a) {
                                    return a.req.hatHotId == r.hatHotId;
                                });
                            if (it != hatHotActive.end()) {
                                it->req        = r;  // Host recycled the id
                                it->framesSince = 0;
                            } else {
                                ActiveHatHotRequest a;
                                a.req = r;
                                hatHotActive.push_back(a);
                            }
                        }
                        hatHot.items.clear();
                    }

                    const terrain::Sample sim = terrain::latest();
                    const uint8_t lsn = static_cast<uint8_t>(
                        g_stats.lastHostFrame.load() & 0x0Fu);
                    int answered = 0;

                    for (size_t i = 0;
                         i < hatHotActive.size() && answered < kMaxResponsesPerFrame; ) {
                        ActiveHatHotRequest& a = hatHotActive[i];
                        const bool due = a.req.updatePeriod == 0 ||
                                         a.framesSince % a.req.updatePeriod == 0;
                        if (!due) {
                            ++i;
                            continue;
                        }

                        const HatHotAnswer ans = answerHatHot(a.req, sim);
                        const uint8_t valid = ans.valid ? 1 : 0;

                        if (a.req.requestType == cigi::kHatHotRequestExtended) {
                            cigi::packHatHotExtResponse(pkt + used, a.req.hatHotId,
                                                        valid, lsn, ans.hat, ans.hot);
                            used += cigi::kHatHotExtRespSize;
                        } else {
                            // HAT (0) / HOT (1): the 16-byte response carries
                            // only the datum the Host asked for.
                            const bool wantHot =
                                a.req.requestType == cigi::kHatHotRequestHot;
                            cigi::packHatHotResponse(pkt + used, a.req.hatHotId,
                                                     valid, a.req.requestType, lsn,
                                                     wantHot ? ans.hot : ans.hat);
                            used += cigi::kHatHotRespSize;
                        }
                        ++answered;

                        if (a.req.updatePeriod == 0) {
                            // One-shot: answered, so it is done.
                            hatHotActive.erase(hatHotActive.begin() +
                                               static_cast<ptrdiff_t>(i));
                        } else {
                            ++a.framesSince;
                            ++i;
                        }
                    }
                } else {
                    // Answering was switched off in the window: drop whatever is
                    // queued so switching it back on starts from a clean slate
                    // instead of replaying terrain against stale requests.
                    std::lock_guard<std::mutex> lk(hatHot.mtx);
                    hatHot.items.clear();
                    hatHotActive.clear();
                }

                sendto(tx, reinterpret_cast<const char*>(pkt),
                       static_cast<int>(used), 0,
                       reinterpret_cast<sockaddr*>(&hostAddr), sizeof(hostAddr));

                ++frame;
                g_stats.sofFrames.store(frame);
            }

            next += period;

            // If a scheduling stall put us more than one frame behind, resync
            // the schedule to now instead of firing a burst of catch-up SOFs.
            // (sleep_until() would otherwise return immediately for every
            // missed tick, flooding the HOST with back-to-back frames.)
            const auto after = std::chrono::steady_clock::now();
            if (after - next > period) {
                next = after;
            }

            std::this_thread::sleep_until(next);
        }
    });

    // ---- Operator window (main thread): blocks until EXIT is clicked. -----
    // The worker threads keep running underneath. The window only polls their
    // statistics and pokes g_sending / g_running -- it never touches a socket.
    gui::Hooks hooks;
    hooks.status           = uiStatus;
    hooks.setSending       = uiSetSending;
    hooks.setRespondHatHot = uiSetRespondHatHot;
    hooks.setFps           = uiSetFps;
    hooks.requestQuit      = uiRequestQuit;

    gui::Link link;
    link.hostIp   = kHostIp;
    link.hostPort = kHostPort;
    link.igIp     = kIgIp;
    link.igPort   = kIgPort;

    const int exitCode = gui::run(hooks, link);

    // ---- Shutdown ---------------------------------------------------------
    g_running.store(false);  // in case run() returned without EXIT
    terrain::stop();         // joins the poller before FSUIPC/the CRT goes away
    closesocket(rx);
    closesocket(tx);
    if (rxThread.joinable()) rxThread.join();
    if (txThread.joinable()) txThread.join();
    timeEndPeriod(1);
    WSACleanup();
    return exitCode;
}
