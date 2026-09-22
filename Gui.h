//=============================================================================
//  Gui.h — the operator window for CigiStarter
//
//  Why this exists
//    CigiStarter used to be a console program driven by S / X / Q keystrokes.
//    The IG PC is operated by hand at a deployment site, so it gets a small
//    window instead: START and STOP for the SOF stream, a live status block,
//    and EXIT.
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
//  statistics and calls back to start, stop and quit.
//=============================================================================
#pragma once

#include <cstdint>

namespace gui {

// A snapshot of everything the window displays. The caller fills this in from
// its own statistics and the FSUIPC terrain poller. Counters are passed
// cumulative; the window works out the per-second rate itself.
struct Status {
    bool     sending          = false;  // SOF stream running?
    bool     respondingHatHot = false;  // --respond-hathot given?
    uint64_t datagrams        = 0;      // downstream datagrams received
    uint64_t entityUpdates    = 0;      // Entity Control packets seen
    uint32_t sofFrames        = 0;      // SOF frames transmitted
    uint8_t  hostIgMode       = 0;      // mode the HOST last commanded
    uint32_t lastHostFrame    = 0;      // HOST frame counter
    bool     terrainConnected = false;  // FSUIPC link up?
    double   groundAltMeters  = 0.0;    // terrain under the aircraft
    double   aircraftAltMeters = 0.0;   // aircraft altitude
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
    Status (*status)();          // polled several times per second
    void   (*setSending)(bool);  // START / STOP
    void   (*requestQuit)();     // EXIT
};

// Create the window and block in its message loop until EXIT is clicked.
// Returns the process exit code (0 = clean exit).
int run(const Hooks& hooks, const Link& link);

} // namespace gui
