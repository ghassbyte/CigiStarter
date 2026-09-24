//=============================================================================
//  CigiStarter — minimal CIGI 3.3 Image Generator (IG) for a new IG PC
//
//  Role
//    Plays the "Master IG" side of a CIGI link so the HOST begins streaming
//    flight data. The HOST stays silent until the IG transmits Start-of-Frame
//    (SOF) messages, so this program continuously sends SOF at a configurable
//    rate and answers HAT/HOT requests with the terrain height from an offline
//    SRTM tile set (TerrainDb.h), falling back to the local Prepar3D via FSUIPC
//    where it has no coverage (switched on by default; an unreachable sim just
//    means the FSUIPC fallback stays invalid until it appears).
//
//  SOF rate and flow control
//    The HOST answers each SOF with one datagram, and each answer carries a fresh
//    attitude (the position comes on every other one), so the SOF rate is what
//    sets how smoothly the aircraft moves. The default rate is 150 Hz with flow
//    control on: the transmit loop holds a SOF back while the previous one is
//    still unanswered, so the stream settles at whatever the HOST on site can
//    absorb rather than piling requests up on it (SofPacer.h). The rate box is
//    therefore a ceiling, not a promise -- the reference Master IG ran at ~100 Hz
//    and the HOST kept up, and the site runs the IG higher than that without the
//    HOST falling behind. `--no-flow-control` restores the plain rate schedule,
//    for a HOST that turns out to tolerate more. The SOF itself echoes the HOST's
//    own frame counter (cigi.cpp); that field is what ties the two rates together.
//
//  Transport (fixed by the deployment; hardcoded below)
//    HOST 192.168.70.2  -> IG 192.168.70.70 : 8000   (downstream data)
//    IG   192.168.70.70 -> HOST 192.168.70.2 : 8001  (upstream SOF / responses)
//
//  Operator interface
//    A small fixed-size Win32 window (Gui.cpp) showing the link, the live
//    stream statistics and the terrain sample, with START / STOP / EXIT plus
//    a HAT/HOT check box, a flow-control check box and a SOF rate box.
//    EXIT is the only way out -- the title-bar X and Alt+F4 are swallowed, so
//    the IG cannot be taken off the link by a stray click. This file owns the
//    worker threads and the statistics that window displays.
//
//  Fan-out to the extra IG PCs
//    A deployment with more than one IG PC needs the others fed too, and they
//    are passive: they want the flight data and nothing else. `client.txt`
//    next to this program lists them, and every datagram the HOST sends here
//    is copied to each of them, byte for byte, appearing to come from the HOST
//    itself (so the client PCs need no configuration). Relay.h explains how,
//    and why that means Npcap.
//
//  Dependencies
//    Windows sockets, the 1 ms timer and user32 (Ws2_32.lib, Winmm.lib,
//    User32.lib). While HAT/HOT answering is switched on (the default) the
//    FSUIPC SDK is linked as well, to read the terrain height out of the local
//    Prepar3D (isolated in TerrainSource.cpp). Npcap is used for the fan-out
//    but is loaded at run time, never linked, so a PC without it is still a
//    perfectly good IG (Relay.cpp).
//
//  Usage
//    CigiStarter.exe [--fps 150] [--ig-mode 0] [--no-respond-hathot]
//                    [--no-flow-control] [--terrain-dir <dir>]
//                    [--terrain-offset <m>] [--help]
//    Being a windowed program there is no console, so --help and any startup
//    error are reported in a message box.
//=============================================================================
#include "cigi.h"
#include "SofPacer.h"
#include "Gui.h"
#include "Relay.h"
#include "TerrainDb.h"
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
    // 150 Hz by default: flow control (below) holds the stream back to whatever
    // the HOST can absorb, so this is a ceiling rather than a promise, and the
    // higher rate keeps the aircraft's motion smooth.
    int     fps           = 150;  // SOF transmission rate
    uint8_t igMode        = 0;    // 0 = Standby (0x30), 1 = Operate (0x31)
    bool    respondHatHot = true;
    bool    flowControl   = true; // pace the SOF stream to the HOST's replies
    // Start transmitting as soon as the program starts, and minimise the window
    // once it is. At a deployment site the IG PC is switched on rather than
    // operated, so there should be nothing to click before the link is live --
    // the reference IG was free-running from the moment it started. The window
    // is still in the taskbar, and START / STOP / EXIT still work from it.
    bool    autoStart     = true;

    // The HAT/HOT terrain source. An offline SRTM tile set answers the request
    // regardless of the sim's state, which is what keeps the HOST's altitude
    // loop honest while the aircraft is flown into Prepar3D with FSUIPC slew
    // writes (the sim's own ground model is frozen in slew). Empty means the
    // "terrain" folder next to this program (TerrainDb.h).
    std::string terrainDir;       // "" = <exe folder>\terrain
    double      terrainOffset = 0.0;  // metres added to database heights
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
    // Downstream datagrams that came from the HOST, i.e. answers to our SOFs.
    // The transmit loop watches this to tell whether the HOST has caught up
    // (SofPacer.h); it is the only flow-control signal there is.
    std::atomic<uint64_t> hostReplies{0};
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
// SOF stream on/off (START / STOP). Seeded from --auto-start, so it is on from
// launch unless the operator asks for the old start-paused behaviour.
static std::atomic<bool> g_sending{false};
static Stats             g_stats;           // filled in by the receive thread
// Answering HAT/HOT (which also decides whether the terrain poller runs), the
// SOF transmit rate and the flow-control mode. All are seeded from the command
// line and from then on owned by the window's controls -- the window thread
// cannot see main()'s Config, so the live values live here.
static std::atomic<bool> g_respondHatHot{true};
static std::atomic<int>  g_targetFps{150};
static std::atomic<bool> g_flowControl{true};

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
        "Options (these only set the starting state -- the SOF rate, HAT/HOT\n"
        "answering and flow control can also be changed from the window while it\n"
        "runs):\n\n"
        "  --fps <rate>        SOF transmit rate in Hz           (default 150)\n"
        "                      With flow control on this is a ceiling: the stream is\n"
        "                      held back while the HOST has not answered, so a rate\n"
        "                      above what the HOST manages costs nothing.\n"
        "  --ig-mode <0..3>    IG mode reported in the SOF until the HOST sends\n"
        "                      its first IG Control (0=Standby 1=Operate)\n"
        "                                                         (default 0)\n"
        "  --respond-hathot    answer HAT/HOT requests, reporting the terrain\n"
        "                      height from the local terrain database, or from\n"
        "                      the local Prepar3D via FSUIPC where it has none\n"
        "                      (this is the default)\n"
        "  --no-respond-hathot do not answer HAT/HOT requests, and leave FSUIPC\n"
        "                      untouched\n"
        "  --flow-control      hold a SOF back while the HOST has not answered the\n"
        "                      previous one, so the stream fits the HOST (this is\n"
        "                      the default)\n"
        "  --no-flow-control   transmit on the rate alone, without waiting for the\n"
        "                      HOST\n"
        "  --auto-start        start transmitting as soon as the program starts,\n"
        "                      and minimise the window so it is out of the way\n"
        "                      (this is the default)\n"
        "  --no-auto-start     start with the stream stopped and the window up, so\n"
        "                      START has to be clicked\n"
        "  --terrain-dir <dir> folder of SRTM \".hgt\" terrain tiles to answer\n"
        "                      HAT/HOT from           (default: terrain\\ beside\n"
        "                      this program; see README.md)\n"
        "  --terrain-offset <m> constant added to every database height, for\n"
        "                      calibrating against the sim          (default 0)\n"
        "  --help              show this help\n"
        "\n"
        "Extra (passive) IG PCs to copy the HOST's stream to are listed in\n"
        "client.txt next to this program -- see README.md.\n",
        kHostIp, kHostPort, kIgIp, kIgPort, argv0);
}

static void showUsage(const char* argv0) {
    char msg[2048];
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
        } else if (a == "--flow-control") {
            cfg.flowControl = true;     // the default; kept so old command lines work
        } else if (a == "--no-flow-control") {
            cfg.flowControl = false;
        } else if (a == "--auto-start") {
            cfg.autoStart = true;       // the default; kept for symmetry
        } else if (a == "--no-auto-start") {
            cfg.autoStart = false;
        } else if (a == "--terrain-dir") {
            const char* v = next("--terrain-dir"); if (!v) return false;
            cfg.terrainDir = v;
        } else if (a == "--terrain-offset") {
            const char* v = next("--terrain-offset"); if (!v) return false;
            cfg.terrainOffset = std::atof(v);
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
// Two terrain sources are available. The offline SRTM database (TerrainDb.h)
// is preferred: it is live for the whole flight, whereas the sim's own ground
// altitude freezes while the aircraft is flown into Prepar3D with FSUIPC slew
// writes. FSUIPC still supplies the aircraft's position and altitude (and the
// fallback terrain, where the database has no coverage).
//
// The original Master IG answered from its own terrain database for the same
// reason, so the database is the reference behaviour rather than a workaround.
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

    // Work out the point whose terrain the HOST is asking about, and the
    // altitude of the test point itself.
    double testAlt     = 0.0;
    double lat         = 0.0;
    double lon         = 0.0;
    bool   simFallback = false;  // may FSUIPC stand in if the database misses?

    if (req.coordSystem == cigi::kHatHotEntity) {
        if (req.entityId != 0) {
            return ans;  // not the aircraft the sim is flying
        }
        if (!sim.connected) {
            return ans;  // no aircraft position/altitude to anchor the request
        }
        // Entity axes are local NED: +Z points out of the bottom, so a
        // positive Z offset sits *below* the reference point.
        testAlt = sim.aircraftAltMeters - req.z;
        // The HOST's entity-relative probes sit within ~10 m of the aircraft
        // (the reference capture's six points reach 10.4 m back and 3 m
        // forward). Terrain data is far coarser than that, so the terrain
        // under the aircraft answers every one of them.
        lat = sim.latDeg;
        lon = sim.lonDeg;
        simFallback = true;
    } else {
        testAlt = req.z;  // geodetic requests carry the altitude directly
        lat = req.x;
        lon = req.y;
        // FSUIPC only knows the terrain under the aircraft, so it can stand in
        // for a geodetic request only when that request is close to it.
        simFallback = sim.connected && nearAircraft(lat, lon, sim);
    }

    // The database first: unlike the sim, it stays live for the whole flight.
    bool   ok  = false;
    double hot = terrain::db::elevation(lat, lon, ok);
    if (!ok && simFallback) {
        hot = sim.groundAltMeters;
        ok  = true;
    }
    if (!ok) {
        // No terrain to report: leave Valid = 0 so the HOST ignores the packet
        // rather than following a fabricated HOT of 0 into the ground.
        return ans;
    }

    ans.hot   = hot;
    ans.hat   = testAlt - hot;
    ans.valid = true;
    return ans;
}

//-----------------------------------------------------------------------------
// Window callbacks
//
// All of them run on the window's thread. The window owns no state of its own,
// so it polls the statistics through uiStatus() and pokes the atomics the
// worker threads already watch.
//-----------------------------------------------------------------------------
static gui::Status uiStatus() {
    gui::Status s;
    s.sending           = g_sending.load();
    s.respondingHatHot  = g_respondHatHot.load();
    s.flowControl       = g_flowControl.load();
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

    const terrain::db::Status tdb = terrain::db::status();
    s.terrainTiles  = tdb.tiles;
    s.terrainLoaded = tdb.loaded;
    s.terrainBytes  = tdb.bytes;
    s.terrainDir    = tdb.dir;
    s.terrainError  = tdb.error;

    // The fan-out to the extra IG PCs. It is optional and never fatal, so its
    // state is reported here rather than gating anything.
    const relay::Status fan = relay::status();
    s.relayFileFound = fan.fileFound;
    s.relayClients   = fan.clients;
    s.relayReady     = fan.ready;
    s.relaySkipped   = fan.skipped;
    s.relayForwarded = fan.forwarded;
    s.relayDropped   = fan.dropped;
    s.relayDevice    = fan.device;
    s.relayError     = fan.error;
    return s;
}

static void uiSetSending(bool on) { g_sending.store(on); }
static void uiRequestQuit()       { g_running.store(false); }

// Flow control is read afresh every frame by the transmit loop, so switching it
// needs no other bookkeeping: the loop's pacer state is kept across the change
// (a held-back SOF simply becomes due again).
static void uiSetFlowControl(bool on) { g_flowControl.store(on); }

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
    // sleep_until() overshoots its deadline at any rate above ~60 Hz and the
    // transmit loop emits bursts of back-to-back SOF datagrams; the HOST is the
    // one that cannot absorb those. timeBeginPeriod(1) makes the rate the
    // operator asks for the rate the loop actually sends, and flow control
    // (SofPacer.h) then keeps that stream inside what the HOST can take.
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

    // Seed the live state; from here on the window owns it. HAT/HOT answering,
    // the rate and flow control are the window's controls, seeded from the
    // command line; the stream state is seeded too, so that --auto-start (the
    // default) has the stream on before the window is ever created.
    g_respondHatHot.store(cfg.respondHatHot);
    g_targetFps.store(cfg.fps);
    g_flowControl.store(cfg.flowControl);
    g_sending.store(cfg.autoStart);

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

    // ---- The offline terrain database (SRTM tiles) ------------------------
    // The HAT/HOT answer is taken from here in preference to the sim, because
    // the sim's ground model is frozen while the aircraft is driven into it
    // with FSUIPC slew writes (see TerrainDb.h). Indexing the folder is cheap;
    // each tile is read from disk the first time it is asked about. Optional
    // and never fatal -- no tiles just means the FSUIPC fallback, as before.
    terrain::db::setVerticalOffset(cfg.terrainOffset);
    if (cfg.terrainDir.empty()) {
        terrain::db::load();
    } else {
        terrain::db::loadFromDirectory(cfg.terrainDir);
    }

    // ---- Fan the HOST's stream out to the extra IG PCs (client.txt) ------
    // Optional and never fatal: the client list lives next to the executable,
    // and with no client.txt this does nothing at all. See Relay.h.
    relay::start(kHostIp, kHostPort, kIgIp, kIgPort);

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
            // Copy the datagram to the extra IG PCs before doing anything else
            // with it -- before even the malformed-frame check -- so that they
            // receive exactly what the HOST sent. Only the HOST is copied: the
            // client IG PCs are passive and have nothing to say back.
            if (from.sin_addr.s_addr == hostAddr.sin_addr.s_addr) {
                // This datagram is the HOST's answer to a SOF, which is the
                // signal the transmit loop's flow control waits for. Counted
                // before anything can reject the frame: a reply is a reply even
                // if we cannot parse it.
                g_stats.hostReplies.fetch_add(1);
                relay::forward(buf.data(), static_cast<size_t>(n));
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
        // Only feeds the frame count the window shows: the SOF itself carries
        // the HOST's frame counter, not this one (see cigi.cpp).
        uint32_t frame = 0;
        // The rate is re-read every frame, so the window can retune the stream
        // without stopping it.
        int  fps    = g_targetFps.load();
        auto period = std::chrono::microseconds(1000000 / fps);
        auto next   = std::chrono::steady_clock::now();

        // Flow-control state: which HOST reply the stream is up to, and when the
        // last SOF went out -- the age of the SOF that may still be waiting for
        // an answer (SofPacer.h).
        pacer::State pacerState;
        auto lastSend = std::chrono::steady_clock::now();

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

            // A due SOF is held back while the HOST has not answered the previous
            // one, so the stream cannot outrun the HOST's own frame rate
            // (SofPacer.h). The deadline pacing below is unchanged either way:
            // the configured rate stays an upper bound, and a frame that was held
            // back is dropped rather than replayed, so it cannot become a burst.
            const auto now = std::chrono::steady_clock::now();
            const pacer::Inputs in{
                g_sending.load(),
                g_flowControl.load(),
                g_stats.hostReplies.load(),
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - lastSend).count(),
            };

            if (pacer::shouldSend(pacerState, in)) {
                // The frame-number slot takes the HOST's own counter, as in the
                // reference capture: it advances with the HOST's frame rate
                // rather than with the SOF rate (see cigi.cpp).
                cigi::packStartOfFrame(sof, g_stats.lastHostFrame.load(),
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
                pacer::noteSent(pacerState, in);
                lastSend = now;
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
    hooks.setFlowControl   = uiSetFlowControl;
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
    relay::stop();           // only now: no receive thread is injecting frames
    timeEndPeriod(1);
    WSACleanup();
    return exitCode;
}
