//=============================================================================
//  Gui.h — the operator window for CigiStarter
//
//  Why this exists
//    CigiStarter used to be a console program driven by S / X / Q keystrokes.
//    The IG PC is operated by hand at a deployment site, so it gets a small
//    window instead: the link settings, a live status block, and START / STOP
//    / EXIT.
//
//  What the operator can change at runtime
//    - START / STOP of the SOF stream,
//    - whether HAT/HOT requests are answered (which starts/stops the FSUIPC
//      terrain poller along with it),
//    - whether the SOF stream is paced to the HOST's replies (flow control),
//    - the SOF transmit rate, typed as a whole number of Hz.
//    The command-line options only seed these controls; whatever the window
//    shows is what the program is doing.
//
//  Exit is deliberate
//    EXIT is the ONLY way out. The window's X button and Alt+F4 are swallowed
//    (SC_CLOSE is removed from the system menu and WM_CLOSE is ignored), so a
//    stray click cannot take the IG off the link mid-run -- the same rule the
//    CigiP3dBridge GUI follows.
//
//  Deliberately plain Win32 -- no resource file, no MFC/ATL/Qt -- so the
//  program still links against nothing but the Windows system libraries it
//  already uses. The window owns no simulation state: it polls the caller's
//  statistics and calls back to start, stop, retune and quit.
//=============================================================================
#pragma once

#include <cstdint>
#include <string>

namespace gui {

// SOF rate limits, in Hz. Shared with the command-line parser so `--fps` and
// the rate box in the window accept exactly the same range.
constexpr int kMinFps = 1;
constexpr int kMaxFps = 100000;

// A snapshot of everything the window displays. The caller fills this in from
// its own statistics and the FSUIPC terrain poller. Counters are passed
// cumulative; the window works out the per-second rates itself.
struct Status {
    bool     sending           = false;  // SOF stream running?
    bool     respondingHatHot  = false;  // answering HAT/HOT requests?
    bool     flowControl       = true;   // pacing SOF to the HOST's replies?
    uint64_t datagrams         = 0;      // downstream datagrams received
    uint64_t entityUpdates     = 0;      // Entity Control packets seen
    uint32_t sofFrames         = 0;      // SOF frames transmitted
    int      targetFps         = 60;     // rate the transmit loop is pacing to
    uint8_t  hostIgMode        = 0;      // mode the HOST last commanded
    uint32_t lastHostFrame     = 0;      // HOST frame counter
    bool     terrainConnected  = false;  // FSUIPC link up?
    double   groundAltMeters   = 0.0;    // terrain under the aircraft
    double   aircraftAltMeters = 0.0;    // aircraft altitude
    // The fan-out to the extra IG PCs (Relay.h). The relay is read once, at
    // startup, so everything here is a fixed description of what it is doing
    // apart from the two counters and the `ready` count.
    bool        relayFileFound = false;  // client.txt was there to be read
    int         relayClients   = 0;      // usable entries in it
    int         relayReady     = 0;      // entries that answer (fed right now)
    int         relaySkipped   = 0;      // client.txt lines that were unusable
    uint64_t    relayForwarded = 0;      // datagrams copied to a client
    uint64_t    relayDropped   = 0;      // datagrams no client could receive
    std::string relayDevice;             // adapter the copies leave by
    std::string relayError;              // why the relay is off ("" = running)
};

// Fixed deployment information, shown above the live status.
struct Link {
    const char* hostIp   = "";
    uint16_t    hostPort = 0;
    const char* igIp     = "";
    uint16_t    igPort   = 0;
};

// The callbacks the window drives. All are invoked on the GUI thread.
struct Hooks {
    Status (*status)();              // polled several times per second
    void   (*setSending)(bool);      // START / STOP
    void   (*setRespondHatHot)(bool);// HAT/HOT check box
    void   (*setFlowControl)(bool);  // flow-control check box
    void   (*setFps)(int);           // rate box, already range-checked
    void   (*requestQuit)();         // EXIT
};

// Create the window and block in its message loop until EXIT is clicked.
// Returns the process exit code (0 = clean exit).
int run(const Hooks& hooks, const Link& link);

} // namespace gui
