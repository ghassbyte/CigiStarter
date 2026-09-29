//=============================================================================
//  CigiStarter — minimal CIGI 3.3 Image Generator (IG) for a new IG PC
//
//  Role
//    Plays the "Master IG" side of a CIGI link so the HOST begins streaming
//    flight data. The HOST stays silent until the IG transmits Start-of-Frame
//    (SOF) messages, so this program continuously sends SOF at a configurable
//    rate and answers HAT/HOT requests with the terrain height from the local
//    Prepar3D's own terrain engine, through the PdkTerrainProbe add-on
//    (TerrainProbe.h). An offline SRTM tile set (TerrainDb.h) is the fallback
//    for a PC whose sim is not answering. Answering is on by default; an
//    unreachable sim just leaves the fallback to do the work until it appears.
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
//  Transport (the deployment's link below is the default; the four endpoints
//  can be overridden on the command line for a bench test, without touching it)
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
//    User32.lib) -- and nothing else. Npcap is used for the fan-out but is
//    loaded at run time, never linked, so a PC without it is still a perfectly
//    good IG (Relay.cpp).
//
//  Usage
//    CigiStarter.exe [--fps 150] [--ig-mode 0] [--no-respond-hathot]
//                    [--no-flow-control] [--no-relay-spoof]
//                    [--terrain-dir <dir>]
//                    [--terrain-offset <m>] [--help]
//    Being a windowed program there is no console, so --help and any startup
//    error are reported in a message box.
//=============================================================================
#include "cigi.h"
#include "SofPacer.h"
#include "Environment.h"
#include "Gui.h"
#include "Relay.h"
#include "SimLink.h"
#include "TerrainDb.h"
#include "TerrainProbe.h"

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
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

//-----------------------------------------------------------------------------
// Default link configuration (the deployment's addresses)
//
// These are what the program uses unless --host-ip / --host-port / --ig-ip /
// --ig-port are given, which is only ever for a bench test. The deployment
// itself passes none of them.
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
    // Publish the HOST's entities into the local Prepar3D through the
    // PdkSimLink add-on (SimLink.h).  The CIGI stream is the whole world, so
    // there is nothing to configure on this side; the add-on is loaded with the
    // sim and picks the poses up out of shared memory.  On by default, because
    // an IG that is not rendering the HOST's world is not doing its job; the
    // switch is here so a bench run can leave the sim alone.
    bool    simLink       = true;
    // Start transmitting as soon as the program starts, and minimise the window
    // once it is. At a deployment site the IG PC is switched on rather than
    // operated, so there should be nothing to click before the link is live --
    // the reference IG was free-running from the moment it started. The window
    // is still in the taskbar, and START / STOP / EXIT still work from it.
    bool    autoStart     = true;

    // The relay to the extra IG PCs labels its copies as the HOST by default,
    // which is what their capture filter looks for. --no-relay-spoof sends them
    // with this IG's own address instead, for a client driver pointed at the IG.
    bool    relaySpoof    = true;

    // The link endpoints. The deployment's are the defaults; they are only ever
    // changed for a bench test that runs the HOST somewhere other than
    // 192.168.70.2.
    std::string hostIp   = kHostIp;
    uint16_t    hostPort = kHostPort;
    std::string igIp     = kIgIp;
    uint16_t    igPort   = kIgPort;

    // The offline HAT/HOT terrain source: the fallback for when the local
    // Prepar3D's own terrain probe cannot answer. It is a copy of the world, so
    // it keeps answering whatever the sim is doing, which is what keeps the
    // HOST's altitude loop honest while the aircraft is positioned into
    // Prepar3D from outside. Empty means the "terrain" folder next to this
    // program (TerrainDb.h); the probe is preferred and needs no configuration.
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
    std::atomic<uint64_t> viewControl{0};
    std::atomic<uint64_t> celestialSphere{0};
    std::atomic<uint64_t> atmosphere{0};
    std::atomic<uint64_t> weatherControl{0};
    std::atomic<uint64_t> cloudPatterns{0};
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
// Whether the HOST's entities are published to the local Prepar3D (SimLink.h).
// Seeded from the command line and otherwise fixed: there is no window control
// for it in this cut.
static std::atomic<bool> g_simLinkEnabled{true};

// The pose the HOST is commanding for the aircraft (its Entity Control for
// entity 0), kept so the SOF thread can answer entity-relative HAT/HOT requests
// against it. Those requests name no position of their own -- they are offsets
// from the aircraft -- so the answer is only right when it is computed around
// the aircraft the HOST believes it is flying. The sim's own aircraft was once
// the fallback anchor, read over FSUIPC, and on the rig it froze at the
// departure airfield while the HOST flew the aircraft to its destination: every
// answer kept describing the departure terrain, so the HOST levelled off on
// ground that was 550 ft too high. The HOST's Entity Control is the fix, and it
// arrives with the first frame, so there is no fallback to keep -- entity-
// relative requests are answered Valid = 0 until one has been seen.
struct HostPose {
    bool   seen      = false;
    double latDeg    = 0.0;
    double lonDeg    = 0.0;
    double altMeters = 0.0;  // metres MSL
};

static std::mutex g_hostPoseMtx;
static HostPose   g_hostPose;

// Copy the latest pose (thread-safe).
static HostPose hostPose() {
    std::lock_guard<std::mutex> lk(g_hostPoseMtx);
    return g_hostPose;
}

// Record the pose from one Entity Control message.
static void noteHostPose(const cigi::EntityControl& e) {
    // A HOST also sends Entity Control for entities it is not driving, and those
    // carry an all-zero pose (the reference capture's entity 5 does exactly
    // that). An all-zero pose means "no position", not the Gulf of Guinea, so it
    // is not adopted -- and neither is anything off the globe. Leaving the pose
    // unset means there is no anchor at all. The same rule decides which
    // entities the sim link renders (cigi::poseUsable), so the anchor and the
    // rendered world cannot disagree about which entities exist.
    if (!cigi::poseUsable(e)) return;

    std::lock_guard<std::mutex> lk(g_hostPoseMtx);
    g_hostPose.seen      = true;
    g_hostPose.latDeg    = e.latDeg;
    g_hostPose.lonDeg    = e.lonDeg;
    g_hostPose.altMeters = e.altMeters;
}

//-----------------------------------------------------------------------------
// The HOST's world beyond the entities
//-----------------------------------------------------------------------------
// What the HOST has said about the clock, the air and the weather, accumulated
// from the CIGI environment messages as they arrive (Environment.h owns the
// folding and the resolution; this is only the place it is kept). Guarded
// because the receive thread writes it and the window thread reads it for the
// status line.
static std::mutex               g_hostEnvMtx;
static cigienv::HostEnvironment g_hostEnv;

// The operator's overrides, set by the window's two combo boxes and read by the
// receive thread when it resolves. `Host` (the zero value) is "follow the HOST",
// so an untouched IG applies exactly what the HOST commanded.
static std::atomic<int> g_timePreset{static_cast<int>(cigienv::TimePreset::Host)};
static std::atomic<int> g_weatherPreset{static_cast<int>(cigienv::WeatherPreset::Host)};

// Resolve the HOST's world with the operator's overrides on top and hand it to
// the add-on (SimLink.h). Cheap to call as often as a message arrives: the
// publisher drops it unless something actually changed, which is what keeps a
// commanded 12:00 from re-pinning the sim's clock every frame. The operator's
// combo boxes call this too, so a manual change takes effect without waiting for
// another message.
static void publishWorld() {
    if (!g_simLinkEnabled.load()) return;

    cigienv::HostEnvironment host;
    {
        std::lock_guard<std::mutex> lk(g_hostEnvMtx);
        host = g_hostEnv;
    }
    const auto time    = static_cast<cigienv::TimePreset>(g_timePreset.load());
    const auto weather = static_cast<cigienv::WeatherPreset>(g_weatherPreset.load());
    simlink::publishEnvironment(cigienv::resolve(host, time, weather));
}

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
        "Link (defaults): HOST %s:%u  <->  IG %s:%u\n\n"
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
        "                      height from the Prepar3D the aircraft is flying in\n"
        "                      (through the PdkTerrainProbe add-on), or from the\n"
        "                      local terrain database, in that order\n"
        "                                                         (this is the default)\n"
        "  --no-respond-hathot do not answer HAT/HOT requests\n"
        "  --flow-control      hold a SOF back while the HOST has not answered the\n"
        "                      previous one, so the stream fits the HOST (this is\n"
        "                      the default)\n"
        "  --no-flow-control   transmit on the rate alone, without waiting for the\n"
        "                      HOST\n"
        "  --sim-link          publish the HOST's entities into the local Prepar3D\n"
        "                      through the PdkSimLink add-on, so the sim renders\n"
        "                      the world the HOST is flying (this is the default)\n"
        "  --no-sim-link       do not publish; leave the local Prepar3D alone\n"
        "  --auto-start        start transmitting as soon as the program starts,\n"
        "                      and minimise the window so it is out of the way\n"
        "                      (this is the default)\n"
        "  --no-auto-start     start with the stream stopped and the window up, so\n"
        "                      START has to be clicked\n"
        "  --relay-spoof       label the copies sent to the extra IG PCs in\n"
        "                      client.txt with the HOST's address, so a client's\n"
        "                      capture filter finds them like the real stream\n"
        "                      (this is the default)\n"
        "  --no-relay-spoof    label those copies with this IG's own address\n"
        "                      instead, for a client driver pointed at the IG\n"
        "  --terrain-dir <dir> folder of SRTM \".hgt\" terrain tiles to answer\n"
        "                      HAT/HOT from           (default: terrain\\ beside\n"
        "                      this program; see README.md)\n"
        "  --terrain-offset <m> constant added to every database height, for\n"
        "                      calibrating against the sim          (default 0)\n"
        "  --host-ip <addr>    HOST address SOFs and responses go to, for a\n"
        "                      bench test            (default 192.168.70.2)\n"
        "  --host-port <port>  HOST port SOFs and responses go to\n"
        "                                                  (default 8001)\n"
        "  --ig-ip <addr>      this IG's address, as the extra IG PCs see it\n"
        "                                             (default 192.168.70.70)\n"
        "  --ig-port <port>    port the HOST streams flight data to\n"
        "                                                  (default 8000)\n"
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
// Parse a UDP port number, rejecting anything outside 1..65535.
static bool parsePort(const char* s, uint16_t& out) {
    char* end = nullptr;
    const unsigned long v = std::strtoul(s, &end, 10);
    if (end == s || *end != '\0' || v == 0 || v > 65535) {
        return false;
    }
    out = static_cast<uint16_t>(v);
    return true;
}

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
        } else if (a == "--sim-link") {
            cfg.simLink = true;         // the default; kept for symmetry
        } else if (a == "--no-sim-link") {
            cfg.simLink = false;
        } else if (a == "--auto-start") {
            cfg.autoStart = true;       // the default; kept for symmetry
        } else if (a == "--no-auto-start") {
            cfg.autoStart = false;
        } else if (a == "--relay-spoof") {
            cfg.relaySpoof = true;      // the default; kept for symmetry
        } else if (a == "--no-relay-spoof") {
            cfg.relaySpoof = false;
        } else if (a == "--terrain-dir") {
            const char* v = next("--terrain-dir"); if (!v) return false;
            cfg.terrainDir = v;
        } else if (a == "--terrain-offset") {
            const char* v = next("--terrain-offset"); if (!v) return false;
            cfg.terrainOffset = std::atof(v);
        } else if (a == "--host-ip") {
            const char* v = next("--host-ip"); if (!v) return false;
            cfg.hostIp = v;
        } else if (a == "--host-port") {
            const char* v = next("--host-port"); if (!v) return false;
            if (!parsePort(v, cfg.hostPort)) {
                fail("error: --host-port must be a port number (1..65535)");
                return false;
            }
        } else if (a == "--ig-ip") {
            const char* v = next("--ig-ip"); if (!v) return false;
            cfg.igIp = v;
        } else if (a == "--ig-port") {
            const char* v = next("--ig-port"); if (!v) return false;
            if (!parsePort(v, cfg.igPort)) {
                fail("error: --ig-port must be a port number (1..65535)");
                return false;
            }
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
// Two terrain sources are available, tried in this order: the local Prepar3D's
// own terrain engine through the PdkTerrainProbe add-on (TerrainProbe.h), which
// is the surface the sim will actually fly the aircraft onto; and an offline
// SRTM database (TerrainDb.h) for a PC whose sim is not answering.
//
// Both answer about the *world* rather than about where the aircraft is sitting,
// so neither goes stale while the aircraft is positioned into Prepar3D from
// outside. The original Master IG answered from its own terrain database for the
// same reason.
//
// Which aircraft the request is *about* is a separate question from where the
// terrain is, and it is answered by the HOST's own Entity Control (hostPose()
// above), not by the sim link. An entity-relative request is six fixed offsets
// from the aircraft, identical wherever the aircraft is, so the wire itself
// cannot show an anchor that has gone stale -- the answer just quietly describes
// the wrong terrain until the aircraft parks on it.

// The terrain height for one point, from the same chain every HAT/HOT answer is
// taken from: the sim's own terrain engine through the PDK add-on (the surface
// Prepar3D will actually put the aircraft on), else the offline database.
// `key` is the probe slot to ask on, so a caller that is not answering a real
// request (the window's anchor readout) can keep a slot of its own.
//
// `surface`, when passed, is filled only by the sim path: the database has no
// idea whether the ground it describes is a platform.  It is diagnostic for
// now; see terrain::probe::Surface.
static bool terrainAt(unsigned key,
                      double latDeg, double lonDeg, double altMeters,
                      double& hot,
                      terrain::probe::Surface* surface = nullptr) {
    if (terrain::probe::elevation(key, latDeg, lonDeg, altMeters, hot, surface)) {
        return true;
    }
    if (surface) *surface = terrain::probe::Surface{};
    bool ok = false;
    hot = terrain::db::elevation(latDeg, lonDeg, ok);
    return ok;
}

// The answer to one HAT/HOT request. `valid` follows the ICD: it is cleared
// whenever the IG cannot return the requested datum, which here means "we have
// no terrain to report" (a request for another entity, no anchor at all for an
// entity-relative request, or no source covering the point). Answering
// Valid = 0 makes the HOST ignore the packet -- far better than a fabricated
// HOT of 0, which the HOST would follow straight into the ground.
struct HatHotAnswer {
    bool   valid = false;
    double hat   = 0.0;  // metres above the terrain surface
    double hot   = 0.0;  // metres MSL
};

static HatHotAnswer answerHatHot(const cigi::HatHotRequest& req,
                                 const HostPose& host) {
    HatHotAnswer ans;

    // Work out the point whose terrain the HOST is asking about, and the
    // altitude of the test point itself.
    double testAlt = 0.0;
    double lat     = 0.0;
    double lon     = 0.0;

    if (req.coordSystem == cigi::kHatHotEntity) {
        if (req.entityId != 0) {
            return ans;  // not the aircraft the HOST is flying
        }
        // Anchor to the aircraft the HOST is flying. Its Entity Control is the
        // authoritative pose for a request that names offsets from that
        // aircraft, and until one has arrived there is no anchor to answer from
        // at all. This arrives with the HOST's first frame.
        if (!host.seen) {
            return ans;
        }
        testAlt = host.altMeters - req.z;
        lat     = host.latDeg;
        lon     = host.lonDeg;
        // Entity axes are local NED: +Z points out of the bottom, so a positive
        // Z offset sits *below* the reference point. The HOST's entity-relative
        // probes sit within ~10 m of the aircraft (the reference capture's six
        // points reach 10.4 m back and 3 m forward). Terrain data is far
        // coarser than that, so the terrain under the aircraft answers every
        // one of them, and the x/y offsets are folded in as a result.
    } else {
        testAlt = req.z;  // geodetic requests carry the altitude directly
        lat = req.x;
        lon = req.y;
    }

    // The chain below answers from the sim's own terrain engine first, through
    // the PDK add-on: it reports the surface Prepar3D will actually put the
    // aircraft on (a runway's flattening included), it stays live for the whole
    // flight, and it answers a geodetic request anywhere rather than only under
    // the aircraft. The offline database is the fallback for when the add-on is
    // not loaded. See TerrainProbe.h.
    bool   ok  = false;
    double hot = 0.0;

    ok = terrainAt(static_cast<unsigned>(req.hatHotId), lat, lon, testAlt, hot);
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
// The terrain under the anchored aircraft, for the window
//-----------------------------------------------------------------------------
// The ground *the answers use*: the height at the aircraft the entity-relative
// requests are anchored to, from the same chain answerHatHot() takes. The
// transmit loop refreshes it every frame, so the window has a live reading of
// the terrain actually being answered.
//
// A reserved probe key keeps this read out of the slots the real HAT/HOT ids
// use; the HOST's ids are one byte wide, so 0xFFFFFFFF can never collide.
constexpr unsigned kAnchorTerrainKey = 0xFFFFFFFFu;

struct AnchorTerrain {
    bool   valid     = false;  // a source could answer for the anchor
    double hotMeters = 0.0;    // ground height there, metres MSL
    // The sim's own classification of that ground, when the probe answered from
    // an add-on that reports one: `onPlatform` is what says the answer is an
    // object's deck rather than terrain (a vessel at sea). Diagnostic only, and
    // left default when the database supplied the height.
    terrain::probe::Surface surface;
};
static std::mutex    g_anchorTerrainMtx;
static AnchorTerrain g_anchorTerrain;

static void noteAnchorTerrain(bool valid, double hotMeters,
                              const terrain::probe::Surface& surface) {
    std::lock_guard<std::mutex> lk(g_anchorTerrainMtx);
    g_anchorTerrain.valid     = valid;
    g_anchorTerrain.hotMeters = hotMeters;
    g_anchorTerrain.surface   = surface;
}

static AnchorTerrain anchorTerrain() {
    std::lock_guard<std::mutex> lk(g_anchorTerrainMtx);
    return g_anchorTerrain;
}

//-----------------------------------------------------------------------------
// Window callbacks
//
// All of them run on the window's thread. The window owns no state of its own,
// so it polls the statistics through uiStatus() and pokes the atomics the
// worker threads already watch.
//-----------------------------------------------------------------------------
// A short phrase for the window's status block saying which tick the sim's
// terrain probe is answering from. Empty while Prepar3D's own tick is doing the
// work, so the line only appears when there is something to explain. Diagnostic
// only: no decision anywhere reads this.
static std::string probeNote(const terrain::probe::Health& p) {
    if (!p.mappingOpen) return "add-on not loaded";
    if (!p.alive)       return "loaded, but it has no terrain service";

    // Prepar3D stops signalling its per-frame event while the sim is paused --
    // the frame loop stops with it on v6 -- so the probe keeps a thread of its
    // own for exactly that case. Saying which tick is answering is the
    // difference between the sim answering from its own scenery and the sim
    // having stopped while the HOST is still asking.
    if (p.ownThread) {
        return p.simPaused
            ? "sim paused; the probe is answering from its own thread"
            : "the sim's own ticks have stopped; the probe is answering from its own thread";
    }
    if (p.ticking) return std::string();   // Prepar3D's own tick; nothing to explain

    // Nothing is advancing at all, which now takes all of the ticks failing: the
    // frame event, the object's simulation and the probe's thread.  The counters
    // say which of them Prepar3D was ever willing to deliver.
    const bool paused = p.simPaused;

    char buf[256];
    if (p.frameTicks == 0) {
        if (p.frameFails > 0) {
            std::snprintf(buf, sizeof(buf),
                          "tick stopped: no per-frame event (%u set-up failures), and the"
                          " probe's own thread is not running either%s",
                          p.frameFails, paused ? ", sim paused" : "");
        } else {
            std::snprintf(buf, sizeof(buf),
                          "tick stopped: no per-frame event arrived, and the probe's own"
                          " thread is not running either%s",
                          paused ? ", sim paused" : "");
        }
        return buf;
    }

    std::snprintf(buf, sizeof(buf),
                  "tick stopped: frames %u, object mode 0x%X, creates %u, removes %u, and the"
                  " probe's own thread is not running either%s",
                  p.frameTicks, p.simMode, p.objectCreates, p.objectRemoves,
                  paused ? " (sim paused)" : "");
    return buf;
}

// A short phrase for the window's status block saying why the entity writer in
// the local Prepar3D (SimLink.h) is not rendering, when there is something to
// say.  Empty while it is working, so the line only appears when it explains
// itself.  Diagnostic only: no decision anywhere reads this.
static std::string simLinkNote(const simlink::Health& w, bool enabled) {
    if (!enabled)       return "disabled with --no-sim-link";
    if (!w.mappingOpen) return "add-on not loaded";
    if (w.noTitle)      return "loaded, but PdkSimLink.ini has no title to create from";
    if (!w.hasManager)  return "loaded, but it has no sim-object manager";
    if (!w.alive || !w.ticking) return "loaded, but its tick is not running";
    if (w.objectsCreated == 0 && w.createFails > 0) {
        return "the sim refused every object it tried to create";
    }
    if (w.objectsCreated > 0 && w.applied == 0 && w.setFails > 0) {
        return "objects exist, but the sim refuses every move";
    }
    return std::string();   // rendering; nothing to explain
}

static gui::Status uiStatus() {
    gui::Status s;
    s.sending           = g_sending.load();
    s.respondingHatHot  = g_respondHatHot.load();
    s.flowControl       = g_flowControl.load();
    s.datagrams         = g_stats.datagrams.load();
    s.entityUpdates     = g_stats.entityControl.load();
    s.cloudPatterns     = g_stats.cloudPatterns.load();
    s.sofFrames         = g_stats.sofFrames.load();
    s.targetFps         = g_targetFps.load();
    s.hostIgMode        = g_stats.hostIgMode.load();
    s.lastHostFrame     = g_stats.lastHostFrame.load();

    // Which aircraft the entity-relative answers are anchored to, and where it
    // is. A stale anchor is invisible on the wire -- the requests are the same
    // offsets wherever the aircraft is -- so the window has to say it, or an
    // anchor stuck over the departure airfield looks like nothing at all.
    const HostPose anchor = hostPose();
    s.anchorFromHost  = anchor.seen;
    s.anchorLatDeg    = anchor.latDeg;
    s.anchorLonDeg    = anchor.lonDeg;
    s.anchorAltMeters = anchor.altMeters;

    // The ground the answers use at that anchor, refreshed by the transmit loop
    // (see anchorTerrain()).
    const AnchorTerrain ground = anchorTerrain();
    s.anchorGroundValid  = ground.valid;
    s.anchorGroundMeters = ground.hotMeters;
    s.groundSurfaceKnown = ground.surface.reported;
    s.groundOnPlatform   = ground.surface.onPlatform;
    s.groundCategory     = ground.surface.category;

    const terrain::db::Status tdb = terrain::db::status();
    s.terrainTiles  = tdb.tiles;
    s.terrainLoaded = tdb.loaded;
    s.terrainBytes  = tdb.bytes;
    s.terrainDir    = tdb.dir;
    s.terrainError  = tdb.error;

    // The add-on in the local Prepar3D, when it is loaded and ticking: that is
    // what answers HAT/HOT in preference to everything below it. The status is
    // read once and both the yes/no and the reason come from it, so the window
    // can say why the probe is not the source rather than only that it is not.
    const terrain::probe::Health probe = terrain::probe::health();
    s.terrainProbe     = probe.alive && probe.ticking;
    s.terrainProbeNote = probeNote(probe);

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
    s.relaySpoofed   = fan.spoofed;
    s.relaySource    = fan.source;
    s.relayError     = fan.error;

    // The writer that puts the HOST's entities into the local Prepar3D
    // (SimLink.h).  Like the terrain probe it is optional -- an IG PC with no
    // add-on still runs the link -- so its state is reported, not gated.  The
    // line counts as "on" only when the note is empty, i.e. when the add-on is
    // loaded, ticking and actually creating objects; anything else has a reason
    // for the note to carry.
    const bool wanted = g_simLinkEnabled.load();
    const simlink::Health writer = simlink::health();
    s.simLinkNote    = simLinkNote(writer, wanted);
    s.simLink        = wanted && s.simLinkNote.empty();
    s.simLinkObjects = writer.objectsCreated;
    s.simLinkApplied = writer.applied;
    s.simLinkEye     = writer.viewCarried;

    // The HOST's world beyond the entities: the clock and the weather, resolved
    // with the operator's own choices, and what the add-on in the sim made of
    // them.  The two preset indices are carried as well as the text, so the
    // window's combo boxes can show the selection they represent; the text says
    // what is actually being applied, which for "Host" is the HOST's own clock
    // rather than a preset.
    s.envCarried    = writer.envCarried;
    s.envManualTime = writer.envManualTime;
    s.envManualWeather = writer.envManualWeather;
    s.envWeatherPattern = writer.envWeatherPattern;
    s.envApplies    = writer.envApplies;
    s.envLastError  = writer.envLastError;

    const int timeSel    = g_timePreset.load();
    const int weatherSel = g_weatherPreset.load();
    s.envTimePreset    = timeSel;
    s.envWeatherPreset = weatherSel;

    if (timeSel == static_cast<int>(cigienv::TimePreset::Host)) {
        std::lock_guard<std::mutex> lk(g_hostEnvMtx);
        if (g_hostEnv.haveTime) {
            char b[40];
            std::snprintf(b, sizeof(b), "%02u:%02u (HOST)",
                          g_hostEnv.hour, g_hostEnv.minute);
            s.envTimeText = b;
        } else {
            s.envTimeText = "HOST (no clock yet)";
        }
    } else {
        cigienv::TimeOfDay tod{};
        char b[40];
        if (cigienv::timePresetClock(
                static_cast<cigienv::TimePreset>(timeSel), tod)) {
            std::snprintf(b, sizeof(b), "%02d:%02d (manual)", tod.hour, tod.minute);
        } else {
            std::snprintf(b, sizeof(b), "(manual)");
        }
        s.envTimeText = b;
    }
    s.envWeatherText =
        weatherSel == static_cast<int>(cigienv::WeatherPreset::Host)
            ? "HOST"
            : cigienv::weatherPresetName(
                  static_cast<cigienv::WeatherPreset>(weatherSel));

    // When the sky being applied is the HOST's *cloud pattern* -- a Component
    // Control on the cloud entity, which is the only way the reference HOST ever
    // describes one -- say which pattern, so a capture whose sky arrives that
    // way does not look like a HOST that sent nothing.  The pattern's index and
    // its "off" state are read out of the accumulated world, like the clock
    // above, because they are the HOST's rather than the resolved row's.
    if (weatherSel == static_cast<int>(cigienv::WeatherPreset::Host) &&
        writer.envWeatherPattern) {
        std::lock_guard<std::mutex> lk(g_hostEnvMtx);
        char b[40];
        if (g_hostEnv.patternIndex == 0) {
            std::snprintf(b, sizeof(b), "HOST (cloud pattern off)");
        } else {
            std::snprintf(b, sizeof(b), "HOST cloud pattern %u",
                          g_hostEnv.patternIndex);
        }
        s.envWeatherText = b;
    }

    // The sky the sim itself reports, from the add-on's read-back of its weather
    // system (SimLinkIpc.h, kVersion 5).  A METAR that names a cloud is a
    // request; this is the cloud that exists, and it is the one reading that can
    // tell a pattern which reached the sim from one that only reached the
    // region.
    if (writer.envSimCloudLayers == 0) {
        s.envSimSkyText = "no cloud";
    } else {
        char b[96];
        std::snprintf(b, sizeof(b), "%u layer%s %s %.0f-%.0f m %s",
                      writer.envSimCloudLayers,
                      writer.envSimCloudLayers == 1 ? "" : "s",
                      cigienv::p3dCloudTypeName(writer.envSimCloudType),
                      writer.envSimCloudBaseM, writer.envSimCloudTopsM,
                      cigienv::p3dCloudCoverName(writer.envSimCloudCover));
        s.envSimSkyText = b;
    }

    // What the sim itself says the time of day is, from the add-on's read-back
    // of IGlobalData::GetGeneralTimeOfDay() after the clock was applied.  It is
    // the only evidence an outside observer has that a commanded 12:00 became
    // *day* rather than merely being accepted.
    switch (writer.envTimeOfDay) {
    case 0:  s.envSimTimeText = "dawn";  break;
    case 1:  s.envSimTimeText = "day";   break;
    case 2:  s.envSimTimeText = "dusk";  break;
    case 3:  s.envSimTimeText = "night"; break;
    default: s.envSimTimeText = "not read back"; break;
    }

    return s;
}

static void uiSetSending(bool on) { g_sending.store(on); }
static void uiRequestQuit()       { g_running.store(false); }

// Flow control is read afresh every frame by the transmit loop, so switching it
// needs no other bookkeeping: the loop's pacer state is kept across the change
// (a held-back SOF simply becomes due again).
static void uiSetFlowControl(bool on) { g_flowControl.store(on); }

// Switching HAT/HOT answering on and off starts and stops the terrain probe
// with it: there is no point querying the sim while the answer goes unused. The
// probe reconnects on its own, so toggling is safe in any order (including
// before Prepar3D is running, where it just retries).
static void uiSetRespondHatHot(bool on) {
    if (g_respondHatHot.exchange(on) == on) {
        return;  // no change
    }
    if (on) {
        terrain::probe::start();
    } else {
        terrain::probe::stop();
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

// The operator's time-of-day and weather presets.  Both simply re-resolve the
// HOST's world with the new choice on top and republish; the publisher's
// on-change rule makes the apply land once, and switching back to "Host" just
// resolves to the HOST's own values again -- which then differ from the preset,
// so the change is applied and the clock/weather go back to being the HOST's.
static void uiSetTimePreset(int preset) {
    if (preset < 0 || preset >= static_cast<int>(cigienv::TimePreset::Count)) {
        return;
    }
    g_timePreset.store(preset);
    publishWorld();
}

static void uiSetWeatherPreset(int preset) {
    if (preset < 0 || preset >= static_cast<int>(cigienv::WeatherPreset::Count)) {
        return;
    }
    g_weatherPreset.store(preset);
    publishWorld();
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
    bindAddr.sin_port        = htons(cfg.igPort);
    bindAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(rx, reinterpret_cast<sockaddr*>(&bindAddr), sizeof(bindAddr)) == SOCKET_ERROR) {
        const int err = WSAGetLastError();
        closesocket(rx); WSACleanup();
        return fail("error: bind(0.0.0.0:%u) failed (%d)\n"
                    "  (is another program already using UDP port %u?)\n",
                    cfg.igPort, err, cfg.igPort);
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
    if (!resolveV4(cfg.hostIp.c_str(), cfg.hostPort, hostAddr)) {
        closesocket(rx); closesocket(tx); WSACleanup();
        return fail("error: invalid host address '%s'\n", cfg.hostIp.c_str());
    }

    // Seed the live state; from here on the window owns it. HAT/HOT answering,
    // the rate and flow control are the window's controls, seeded from the
    // command line; the stream state is seeded too, so that --auto-start (the
    // default) has the stream on before the window is ever created.
    g_respondHatHot.store(cfg.respondHatHot);
    g_targetFps.store(cfg.fps);
    g_flowControl.store(cfg.flowControl);
    g_sending.store(cfg.autoStart);
    g_simLinkEnabled.store(cfg.simLink);

    // The IG does not choose its own mode: it reports back whatever the HOST
    // last commanded. --ig-mode therefore only seeds the value used until the
    // first IG Control arrives, after which the receive thread takes over.
    g_stats.hostIgMode.store(static_cast<uint8_t>(cfg.igMode & 0x03u));

    // The terrain probe runs while HAT/HOT answering is enabled -- which it is
    // by default, since a wrong or missing HOT flies the aircraft into the
    // ground. The window's check box turns it on and off at runtime.
    if (g_respondHatHot.load()) {
        terrain::probe::start();
    }

    // The sim link (SimLink.h): the HOST's entities, published for the
    // PdkSimLink add-on in the local Prepar3D to render.  Its region is created
    // by the add-on, so there is nothing to create here; opening now is just the
    // first attempt, and publish() re-tries whenever the sim is not up yet.
    if (cfg.simLink) {
        simlink::start();
    }

    // ---- The offline terrain database (SRTM tiles) ------------------------
    // The HAT/HOT answer may be taken from here when the sim's own probe cannot
    // answer. Indexing the folder is cheap; each tile is read from disk the
    // first time it is asked about. Optional and never fatal -- no tiles just
    // means an unanswered request when the probe is not loaded either.
    terrain::db::setVerticalOffset(cfg.terrainOffset);
    if (cfg.terrainDir.empty()) {
        terrain::db::load();
    } else {
        terrain::db::loadFromDirectory(cfg.terrainDir);
    }

    // ---- Fan the HOST's stream out to the extra IG PCs (client.txt) ------
    // Optional and never fatal: the client list lives next to the executable,
    // and with no client.txt this does nothing at all. See Relay.h.
    relay::start(cfg.hostIp.c_str(), cfg.hostPort, cfg.igIp.c_str(), cfg.igPort,
                 cfg.relaySpoof);

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
                    case cigi::MsgEntityControl: {
                        g_stats.entityControl.fetch_add(1);
                        // Within one datagram the HOST sends one Entity Control
                        // per entity. Entity 0 is the aircraft the HOST's own
                        // flight model is flying, and the one its entity-relative
                        // HAT/HOT requests are about: keep its pose -- that, not
                        // the sim link, is what those answers are anchored to.
                        // Every entity, entity 0 included, is also published for
                        // the sim to render (SimLink.h).
                        cigi::EntityControl ent;
                        if (cigi::parseEntityControl(msg, msgSize, ent)) {
                            if (ent.entityId == 0) noteHostPose(ent);
                            if (g_simLinkEnabled.load() && cigi::poseUsable(ent)) {
                                simlink::publish(ent);
                            }
                        }
                        break;
                    }
                    case cigi::MsgViewControl: {
                        g_stats.viewControl.fetch_add(1);
                        // Where the HOST wants its own eye. The IG is the only
                        // party that can put the picture there, and it has to be
                        // told: the entity objects the sim link creates are not
                        // the sim's user aircraft, so the sim's view stays where
                        // the user left it however well they move. Published for
                        // the PdkSimLink add-on, which flies the camera from it
                        // (SimLink.h, SimLinkIpc.h).
                        cigi::ViewControl v;
                        if (g_simLinkEnabled.load() &&
                            cigi::parseViewControl(msg, msgSize, v)) {
                            simlink::publishView(v);
                        }
                        break;
                    }
                    case cigi::MsgCelestialSphere: {
                        g_stats.celestialSphere.fetch_add(1);
                        // The HOST's wall clock and date (id 9).  Folded into the
                        // accumulated world and resolved/published with the rest;
                        // the operator's own time-of-day preset, if any, is
                        // applied on top in resolve().
                        cigi::CelestialSphere cs;
                        if (cigi::parseCelestialSphere(msg, msgSize, cs)) {
                            {
                                std::lock_guard<std::mutex> lk(g_hostEnvMtx);
                                cigienv::applyCelestialSphere(g_hostEnv, cs);
                            }
                            publishWorld();
                        }
                        break;
                    }
                    case cigi::MsgAtmosphere: {
                        g_stats.atmosphere.fetch_add(1);
                        // The air the aircraft flies in (id 10): temperature,
                        // visibility, wind, pressure, humidity.  The captures
                        // carry it in their warm-up frames; publishWorld() drops
                        // the repeats.
                        cigi::Atmosphere a;
                        if (cigi::parseAtmosphere(msg, msgSize, a)) {
                            {
                                std::lock_guard<std::mutex> lk(g_hostEnvMtx);
                                cigienv::applyAtmosphere(g_hostEnv, a);
                            }
                            publishWorld();
                        }
                        break;
                    }
                    case cigi::MsgWeatherControl: {
                        g_stats.weatherControl.fetch_add(1);
                        // Cloud and precipitation (id 12), which no capture in
                        // this deployment carries -- the reference HOST commanded
                        // a whole environment with the warm-up Atmosphere Control
                        // alone.  Built to the ICD and contract-tested; the
                        // operator's weather presets are what this rig will
                        // actually exercise.
                        cigi::WeatherControl w;
                        if (cigi::parseWeatherControl(msg, msgSize, w)) {
                            {
                                std::lock_guard<std::mutex> lk(g_hostEnvMtx);
                                cigienv::applyWeather(g_hostEnv, w);
                            }
                            publishWorld();
                        }
                        break;
                    }
                    case cigi::MsgComponentControl: {
                        // A component on an entity (id 4).  The only one the
                        // reference HOST sends is the cloud pattern on the cloud
                        // entity it creates -- component 0x1001, class Entity --
                        // and it is that capture's *only* description of the
                        // sky: the real HOST never sends a Weather Control
                        // (Environment.h has the evidence and the mapping).
                        cigi::ComponentControl cc;
                        if (cigi::parseComponentControl(msg, msgSize, cc) &&
                            cigienv::isCloudPattern(cc)) {
                            g_stats.cloudPatterns.fetch_add(1);
                            {
                                std::lock_guard<std::mutex> lk(g_hostEnvMtx);
                                cigienv::applyCloudPattern(g_hostEnv, cc);
                            }
                            publishWorld();
                        }
                        break;
                    }
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

                    const HostPose host = hostPose();

                    // The ground the answers use, at the anchor the requests are
                    // taken from. Recomputed every frame so the window can show
                    // the terrain actually being answered. No anchor yet (the
                    // first Entity Control has not arrived) means no ground to
                    // show, and no answer either.
                    if (host.seen) {
                        double hot = 0.0;
                        terrain::probe::Surface surface;
                        const bool ok = terrainAt(kAnchorTerrainKey,
                                                  host.latDeg, host.lonDeg,
                                                  host.altMeters, hot, &surface);
                        noteAnchorTerrain(ok, hot, surface);
                    } else {
                        noteAnchorTerrain(false, 0.0,
                                          terrain::probe::Surface{});
                    }

                    const uint8_t lsn = static_cast<uint8_t>(
                        g_stats.lastHostFrame.load() & 0x0Fu);
                    int answered = 0;

                    for (size_t i = 0;
                         i < hatHotActive.size() && answered < kMaxResponsesPerFrame; ) {
                        ActiveHatHotRequest& a = hatHotActive[i];
                        const bool due = a.req.updatePeriod == 0 ||
                                         a.framesSince % a.req.updatePeriod == 0;
                        if (!due) {
                            // The period clock has to keep running while a
                            // request waits its turn, or it never comes due a
                            // second time and a HOST that leans on the update
                            // period instead of re-sending gets one answer.
                            ++a.framesSince;
                            ++i;
                            continue;
                        }

                        const HatHotAnswer ans = answerHatHot(a.req, host);
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
    hooks.setTimePreset    = uiSetTimePreset;
    hooks.setWeatherPreset = uiSetWeatherPreset;
    hooks.requestQuit      = uiRequestQuit;

    gui::Link link;
    link.hostIp   = cfg.hostIp.c_str();
    link.hostPort = cfg.hostPort;
    link.igIp     = cfg.igIp.c_str();
    link.igPort   = cfg.igPort;

    const int exitCode = gui::run(hooks, link);

    // ---- Shutdown ---------------------------------------------------------
    g_running.store(false);  // in case run() returned without EXIT
    terrain::probe::stop();  // tells the add-on to stop querying for us
    closesocket(rx);
    closesocket(tx);
    if (rxThread.joinable()) rxThread.join();
    if (txThread.joinable()) txThread.join();
    // After the joins: the receive thread is the only caller of publish(), and
    // retiring the rows tells the add-on to take the objects back out.
    simlink::stop();
    relay::stop();           // only now: no receive thread is injecting frames
    timeEndPeriod(1);
    WSACleanup();
    return exitCode;
}
