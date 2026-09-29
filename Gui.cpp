//=============================================================================
//  Gui.cpp — plain Win32 implementation of the CigiStarter window
//
//  See Gui.h for the rationale. The layout is a fixed-size window (no resize):
//
//     +--------------------------------------------------------------------+
//     |  HOST 192.168.70.2:8001   ->  SOF destination                       |
//     |   ... live status block (read-only edit): the link, the stream, the |
//     |       client.txt fan-out, the terrain sample and the world being     |
//     |       applied (time of day, weather) ...                            |
//     |                                                                     |
//     |  [x] Answer HAT/HOT  SOF rate: [ 150 ] Hz [Apply]                  |
//     |  [x] Pace SOF to HOST replies (flow control)                        |
//     |  Time of day: [ Host        v ]  Weather: [ Host        v ]        |
//     |                                                                     |
//     |  [ START ]  [ STOP ]                                        [ EXIT ] |
//     +--------------------------------------------------------------------+
//
//  The settings rows are live: the check boxes switch HAT/HOT answering (and
//  with it the terrain probe) and the flow control, the rate box retunes the
//  SOF stream, and the two drop-downs pick the time of day and the weather, all
//  without stopping anything first. The rate box applies on Enter, on Apply, and
//  when it loses focus, so a typed value can never be silently ignored.
//
//  There is no X button: SC_CLOSE is deleted from the system menu at creation
//  and WM_CLOSE is ignored, so DestroyWindow() is only ever reached from the
//  EXIT handler.
//=============================================================================
#include "Gui.h"
#include "Environment.h"    // the time-of-day/weather preset names for the combo boxes
#include "TerrainProbe.h"   // terrain::probe::surface::kCategory*, for the readout

#include <windows.h>

#include <cstdio>
#include <cwchar>
#include <initializer_list>

namespace gui {
namespace {

// ---- Layout (client-area pixels) -----------------------------------------
constexpr int kMargin    = 12;
constexpr int kClientW   = 660;
constexpr int kStatusW   = kClientW - 2 * kMargin;
// Tall enough for the whole status block plus a couple of wrapped lines: the
// relay line grows with the counters, the terrain block gains a "Probe" line
// when the add-on is not answering, the SimLink line explains itself when the
// entity writer is not answering, the world and sky lines say what clock and
// weather are being applied and what the sim made of them, and a line that wraps
// (the terrain database summary carries a folder path) must not push a reading
// out of sight.
constexpr int kStatusH   = 418;
constexpr int kSettingsY = kMargin + kStatusH + kMargin;
constexpr int kRowH      = 24;
// The flow-control check box gets a row of its own under the HAT/HOT row: there
// is no width left beside it, and a squeezed label would be worse than a taller
// window.
constexpr int kFlagsY    = kSettingsY + kRowH;
// The time-of-day and weather combo boxes get a row below the check boxes: each
// is a labelled drop-down, and neither fits beside the rate box above.
constexpr int kEnvY      = kFlagsY + kRowH;
constexpr int kButtonY   = kEnvY + kRowH + kMargin;
constexpr int kButtonW   = 110;
constexpr int kButtonH   = 32;
constexpr int kClientH   = kButtonY + kButtonH + kMargin;

// ---- Units ---------------------------------------------------------------
// Prepar3D displays its own altitudes in feet, so the operator-facing terrain
// line does too. Everything behind it stays metres: CIGI defines the HAT/HOT
// altitudes in metres and the HOST's flight model consumes them that way, so
// this is a conversion at the point of display only.
constexpr double kFeetPerMetre = 3.280839895013123;

// ---- Control identifiers -------------------------------------------------
constexpr int kStatusId    = 1000;
constexpr int kStartId     = 1001;
constexpr int kStopId      = 1002;
constexpr int kExitId      = 1003;
constexpr int kRespondId   = 1004;
constexpr int kFpsEditId   = 1005;
constexpr int kFpsApplyId  = 1006;
constexpr int kFpsLabelId  = 1007;
constexpr int kFpsUnitId   = 1008;
constexpr int kFlowId      = 1009;
constexpr int kTimeLabelId = 1010;
constexpr int kTimeComboId = 1011;
constexpr int kWeatherLabelId = 1012;
constexpr int kWeatherComboId = 1013;

// How often the status block is rewritten (ms).
constexpr UINT_PTR kRefreshTimer = 1;
constexpr UINT     kRefreshMs    = 250;

constexpr wchar_t kClassName[] = L"CigiStarterWindow";

// ---- Window state --------------------------------------------------------
const Hooks* g_hooks = nullptr;
const Link*  g_link  = nullptr;

HWND  g_hwnd    = nullptr;
HWND  g_status  = nullptr;
HWND  g_start   = nullptr;
HWND  g_stop    = nullptr;
HWND  g_respond = nullptr;
HWND  g_flow    = nullptr;
HWND  g_fpsEdit = nullptr;
HWND  g_timeCombo    = nullptr;
HWND  g_weatherCombo = nullptr;
HFONT g_mono    = nullptr;

// Rate sampling: the counters are cumulative, so the window keeps its own
// previous sample and the time it was taken. The SOF counter is 32-bit and may
// wrap, so its delta is taken in 32-bit arithmetic.
uint64_t g_lastDatagrams  = 0;
uint32_t g_lastSofFrames  = 0;
uint64_t g_lastSampleTick = 0;
uint64_t g_dataRate       = 0;
uint64_t g_sofRate        = 0;

const wchar_t* IgModeName(unsigned mode) {
    switch (mode) {
        case 0:  return L"Standby";
        case 1:  return L"Operate";
        default: return L"other";
    }
}

// A latitude or longitude the way Prepar3D shows its own -- its `Seconds`
// LatLonFormat: hemisphere, then degrees, minutes and whole seconds with two
// fractional digits. The signed decimal degrees this replaced are exact and
// unreadable beside the sim's readout, which is the one thing the `Anchor` line
// exists to be compared with. Rounding the seconds can carry a whole minute, and
// that a whole degree, so the carry is resolved before printing.
void FormatLatLon(double deg, wchar_t north, wchar_t south,
                  wchar_t* out, size_t outCount) {
    const wchar_t hemi = deg < 0.0 ? south : north;
    const double  abs  = deg < 0.0 ? -deg : deg;

    int    d      = static_cast<int>(abs);
    double remMin = (abs - d) * 60.0;
    int    m      = static_cast<int>(remMin);
    double s      = (remMin - m) * 60.0;
    if (s >= 59.995) {
        s = 0.0;
        if (++m >= 60) {
            m = 0;
            ++d;
        }
    }
    std::swprintf(out, outCount, L"%c%d\x00B0%d'%.2f\"", hemi, d, m, s);
}

// Control identifiers ride in the HMENU slot of CreateWindowExW(), so the cast
// has to go through INT_PTR to keep it pointer-sized on x64.
inline HMENU ControlId(int id) {
    return reinterpret_cast<HMENU>(static_cast<INT_PTR>(id));
}

void SetFpsText(int fps) {
    if (!g_fpsEdit) {
        return;
    }
    wchar_t buf[16];
    std::swprintf(buf, ARRAYSIZE(buf), L"%d", fps);
    SetWindowTextW(g_fpsEdit, buf);
}

// Read the rate box and apply it. Invalid input is never adopted: the box is
// left alone when it merely loses focus (the operator is clicking elsewhere --
// a modal box popping up on an unrelated click would be hostile) and is snapped
// back with a warning when Apply or Enter asks for it, so the rate can never be
// silently wrong. `quiet` selects between those two behaviours.
void ApplyFps(bool quiet) {
    if (!g_fpsEdit || !g_hooks || !g_hooks->setFps || !g_hooks->status) {
        return;
    }

    wchar_t buf[32] = L"";
    GetWindowTextW(g_fpsEdit, buf, ARRAYSIZE(buf));

    // Digits only: atoi() would happily turn "1x" into 1, which is not what was
    // typed. The accumulator is capped before each multiply so that a long run
    // of digits cannot overflow it.
    bool ok    = buf[0] != L'\0';
    long value = 0;
    for (const wchar_t* p = buf; ok && *p; ++p) {
        if (*p < L'0' || *p > L'9') {
            ok = false;
            break;
        }
        if (value > 1000000) {
            ok = false;
            break;
        }
        value = value * 10 + (*p - L'0');
    }
    // Both ends of the range, so "0" is rejected rather than quietly doing
    // nothing (main() refuses it, which would look like the box was ignored).
    if (ok && (value < kMinFps || value > kMaxFps)) {
        ok = false;
    }

    if (!ok) {
        if (quiet) {
            return;  // leave the text for the operator to fix
        }
        wchar_t msg[192];
        std::swprintf(msg, ARRAYSIZE(msg),
                      L"Enter the SOF rate as a whole number of Hz,\n"
                      L"between %d and %d.",
                      kMinFps, kMaxFps);
        MessageBoxW(g_hwnd, msg, L"CigiStarter", MB_OK | MB_ICONWARNING);
        SetFpsText(g_hooks->status().targetFps);  // put the live value back
        SetFocus(g_fpsEdit);
        return;
    }

    g_hooks->setFps(static_cast<int>(value));
    SetFpsText(g_hooks->status().targetFps);  // show what was actually adopted
}

// ---- Preset drop-downs ---------------------------------------------------
// The two drop-downs carry the enum's own names, so the window and the code
// that resolves the preset cannot disagree about what "Storm" means.  They are
// populated once, at creation, and only their selection is ever moved.

void PopulateTimeCombo(HWND combo) {
    for (int i = 0; i < static_cast<int>(cigienv::TimePreset::Count); ++i) {
        wchar_t w[64];
        std::swprintf(w, ARRAYSIZE(w), L"%hs",
                      cigienv::timePresetName(static_cast<cigienv::TimePreset>(i)));
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(w));
    }
}

void PopulateWeatherCombo(HWND combo) {
    for (int i = 0; i < static_cast<int>(cigienv::WeatherPreset::Count); ++i) {
        wchar_t w[64];
        std::swprintf(w, ARRAYSIZE(w), L"%hs",
                      cigienv::weatherPresetName(
                          static_cast<cigienv::WeatherPreset>(i)));
        SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(w));
    }
}

// Show the live selection on a drop-down without fighting the operator: while
// the list is open, or the control has the keyboard focus, the window leaves the
// selection alone rather than resetting it out from under a click or a keypress.
void SyncCombo(HWND combo, int index) {
    if (!combo || index < 0) return;
    if (GetFocus() == combo) return;
    if (SendMessageW(combo, CB_GETDROPPEDSTATE, 0, 0) != 0) return;
    const int cur = static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
    if (cur != index) {
        SendMessageW(combo, CB_SETCURSEL, static_cast<WPARAM>(index), 0);
    }
}

// Rebuild the status block from the caller's snapshot and sync the controls.
// The rate box is deliberately not rewritten here -- that would fight whoever
// is typing in it.
void Refresh() {
    if (!g_hooks || !g_hooks->status || !g_status) {
        return;
    }
    const Status s = g_hooks->status();

    const uint64_t now = GetTickCount64();
    if (g_lastSampleTick == 0) {
        g_lastSampleTick = now;
        g_lastDatagrams  = s.datagrams;
        g_lastSofFrames  = s.sofFrames;
    } else if (now - g_lastSampleTick >= 1000) {
        const uint64_t dt = now - g_lastSampleTick;
        g_dataRate = (s.datagrams - g_lastDatagrams) * 1000u / dt;
        g_sofRate  = static_cast<uint32_t>(s.sofFrames - g_lastSofFrames) *
                     1000u / dt;
        g_lastDatagrams  = s.datagrams;
        g_lastSofFrames  = s.sofFrames;
        g_lastSampleTick = now;
    }

    // The terrain lines have their own branches because they can be missing.
    // The sim's own terrain probe answers HAT/HOT when it is loaded, because it
    // reports the surface the sim will actually fly the aircraft onto; the
    // offline database is the fallback for a PC whose sim is not answering.
    wchar_t terrain[1536];
    if (!s.respondingHatHot) {
        std::swprintf(terrain, ARRAYSIZE(terrain),
                      L"  HAT/HOT   : not answered (switched off)");
    } else {
        wchar_t dbline[192];
        if (s.terrainTiles > 0) {
            std::swprintf(dbline, ARRAYSIZE(dbline),
                          L"  TerrainDB : %d tile(s), %d in memory, %llu MB",
                          s.terrainTiles, s.terrainLoaded,
                          static_cast<unsigned long long>(s.terrainBytes >> 20));
        } else {
            std::swprintf(dbline, ARRAYSIZE(dbline), L"  TerrainDB : none -- %hs",
                          s.terrainError.c_str());
        }
        const wchar_t* source =
            s.terrainProbe       ? L"answering from the sim's own terrain"
          : (s.terrainTiles > 0) ? L"answering from the terrain database"
                                 : L"switched on, but nothing to answer from";

        // Which tick the probe is answering from, and why it is not, when there
        // is something to say: without this the operator sees only that the
        // answer fell back -- never whether the add-on is missing, has stopped
        // ticking, has had its tick left on an object the sim no longer
        // simulates (the reposition failure), or has fallen back to its own
        // thread because Prepar3D stopped ticking (a pause).
        wchar_t probeLine[320] = L"";
        if (s.respondingHatHot && !s.terrainProbeNote.empty()) {
            std::swprintf(probeLine, ARRAYSIZE(probeLine),
                          L"\r\n  Probe     : %hs", s.terrainProbeNote.c_str());
        }

        // The aircraft the entity-relative answers are anchored to: the HOST's
        // own Entity Control, the pose its flight model is flying. Shown because
        // a stale anchor is otherwise invisible -- the requests are the same
        // offsets wherever it is -- and because those requests go unanswered
        // until one arrives.
        wchar_t anchorLine[256] = L"";
        if (!s.anchorFromHost) {
            std::swprintf(anchorLine, ARRAYSIZE(anchorLine),
                          L"\r\n  Anchor    : none yet -- no Entity Control "
                          L"from the HOST");
        } else {
            wchar_t anchorLat[32];
            wchar_t anchorLon[32];
            FormatLatLon(s.anchorLatDeg, L'N', L'S', anchorLat,
                         ARRAYSIZE(anchorLat));
            FormatLatLon(s.anchorLonDeg, L'E', L'W', anchorLon,
                         ARRAYSIZE(anchorLon));
            std::swprintf(anchorLine, ARRAYSIZE(anchorLine),
                          L"\r\n  Anchor    : HOST Entity Control %s %s, %.0f ft",
                          anchorLat, anchorLon,
                          s.anchorAltMeters * kFeetPerMetre);
        }

        // The ground the answers actually use: the terrain at the aircraft they
        // are anchored to, from the same chain the answer is taken from. This is
        // the number to watch.
        // What kind of surface the sim called that ground, when it said. Over a
        // vessel at sea the interesting reading is a *platform*: the deck, which
        // Prepar3D's collision will rest an aircraft on while the terrain under
        // it is still sea level. Silently absent for a build or a point the
        // surface was not reported for, so the line reads exactly as before.
        wchar_t surfaceNote[64] = L"";
        if (s.anchorGroundValid && s.groundSurfaceKnown) {
            const wchar_t* cat = L"surface";
            switch (s.groundCategory) {
            case terrain::probe::surface::kCategoryHard:  cat = L"hard";  break;
            case terrain::probe::surface::kCategorySoft:  cat = L"soft";  break;
            case terrain::probe::surface::kCategoryWater: cat = L"water"; break;
            default: break;
            }
            if (s.groundOnPlatform) {
                std::swprintf(surfaceNote, ARRAYSIZE(surfaceNote),
                              L" (%s, platform)", cat);
            } else {
                std::swprintf(surfaceNote, ARRAYSIZE(surfaceNote), L" (%s)", cat);
            }
        }

        wchar_t answerLine[176] = L"";
        if (s.anchorGroundValid) {
            std::swprintf(answerLine, ARRAYSIZE(answerLine),
                          L"\r\n  Terrain   : %.0f ft ground under the HOST aircraft%s",
                          s.anchorGroundMeters * kFeetPerMetre,
                          surfaceNote);
        }

        std::swprintf(terrain, ARRAYSIZE(terrain),
                      L"%s\r\n"
                      L"  HAT/HOT   : %s%s%s%s",
                      dbline, source, probeLine, anchorLine,
                      answerLine);
    }

    // The relay block, likewise: off is a normal state, and each way of being
    // off says which one it is, because the operator's next move differs.
    wchar_t relay[384];
    if (!s.relayError.empty()) {
        std::swprintf(relay, ARRAYSIZE(relay), L"  Relay     : off (%hs)",
                      s.relayError.c_str());
    } else if (!s.relayFileFound) {
        std::swprintf(relay, ARRAYSIZE(relay),
                      L"  Relay     : off (no client.txt next to the program)");
    } else if (s.relayClients == 0) {
        std::swprintf(relay, ARRAYSIZE(relay),
                      L"  Relay     : off (client.txt lists no usable address)");
    } else {
        int used = std::swprintf(
            relay, ARRAYSIZE(relay),
            L"  Relay     : %d client%s, %d reachable, %llu datagrams forwarded",
            s.relayClients, s.relayClients == 1 ? L"" : L"s", s.relayReady,
            static_cast<unsigned long long>(s.relayForwarded));
        const int room = static_cast<int>(ARRAYSIZE(relay)) - used;
        if (used > 0 && room > 0 && s.relayDropped > 0) {
            used += std::swprintf(relay + used, static_cast<size_t>(room),
                                  L", %llu dropped",
                                  static_cast<unsigned long long>(s.relayDropped));
        }
        const int left = static_cast<int>(ARRAYSIZE(relay)) - used;
        if (used > 0 && left > 0 && s.relaySkipped > 0) {
            std::swprintf(relay + used, static_cast<size_t>(left),
                          L", %d client.txt line%s ignored", s.relaySkipped,
                          s.relaySkipped == 1 ? L"" : L"s");
        }
    }

    wchar_t relayNic[320];
    if (s.relayDevice.empty()) {
        std::swprintf(relayNic, ARRAYSIZE(relayNic), L"  Relay NIC : none");
    } else if (s.relaySpoofed) {
        std::swprintf(relayNic, ARRAYSIZE(relayNic), L"  Relay NIC : %hs",
                      s.relayDevice.c_str());
    } else {
        // Only the non-default is called out: the copies normally claim the
        // HOST, and a note saying so every time would be noise.
        std::swprintf(relayNic, ARRAYSIZE(relayNic),
                      L"  Relay NIC : %hs (copies as this IG %hs)",
                      s.relayDevice.c_str(), s.relaySource.c_str());
    }

    // The entity writer in the local Prepar3D (SimLink.h). Unlike the terrain
    // probe this is the rendering path, not a fallback, so "off" means the sim
    // is not showing the HOST's world at all -- which is why the line is always
    // there, with the reason when it is. `simLinkNote` is empty only while it is
    // working.
    wchar_t simlink[256];
    if (!s.simLink) {
        std::swprintf(simlink, ARRAYSIZE(simlink), L"  SimLink   : off -- %hs",
                      s.simLinkNote.empty() ? "not answering"
                                            : s.simLinkNote.c_str());
    } else {
        std::swprintf(simlink, ARRAYSIZE(simlink),
                      L"  SimLink   : %u entit%s applied, %u object%s in the sim%s",
                      s.simLinkApplied, s.simLinkApplied == 1 ? L"y" : L"ies",
                      s.simLinkObjects, s.simLinkObjects == 1 ? L"" : L"s",
                      s.simLinkEye ? L", eye from the HOST" : L"");
    }

    // The world beyond the entities, on the same channel: the clock and the
    // weather that are being applied, and what the add-on in the sim made of
    // them.  `simLinkNote` above already says when the world cannot be applied
    // at all, so this line only reports what was published and what the sim's
    // own time-of-day read-back says about the clock.
    wchar_t world[256];
    if (!s.envCarried) {
        std::swprintf(world, ARRAYSIZE(world),
                      L"  World     : nothing published yet");
    } else {
        const int used = std::swprintf(
            world, ARRAYSIZE(world),
            L"  World     : time %hs, weather %hs; sim says %hs, %u applied",
            s.envTimeText.c_str(), s.envWeatherText.c_str(),
            s.envSimTimeText.c_str(), s.envApplies);
        // A clock or weather call the sim refused is otherwise invisible: the
        // world would simply not move.  The last HRESULT is the same word the
        // region carries for the test to read.
        if (s.envLastError != 0 && used > 0 &&
            static_cast<size_t>(used) < ARRAYSIZE(world)) {
            std::swprintf(world + used, ARRAYSIZE(world) - used,
                          L", last error 0x%08X",
                          static_cast<unsigned>(s.envLastError));
        }
    }

    // The sky, in two halves: what the IG is applying (the world line above) and
    // what the sim says it ended up with.  The second half is the whole point of
    // the line -- a METAR naming a cloud is a request, and a cloud pattern is a
    // request phrased differently again -- so a cloud that only reached the
    // shared region shows up here as "no cloud".  The pattern count says whether
    // the HOST described its sky that way at all, which is otherwise invisible:
    // a capture with cloud patterns and a capture with none both read "HOST".
    wchar_t sky[256];
    if (!s.envCarried) {
        std::swprintf(sky, ARRAYSIZE(sky), L"  Sky       : nothing published yet");
    } else {
        const int used = std::swprintf(
            sky, ARRAYSIZE(sky), L"  Sky       : sim says %hs",
            s.envSimSkyText.empty() ? "nothing read back" : s.envSimSkyText.c_str());
        if (used > 0 && static_cast<size_t>(used) < ARRAYSIZE(sky) &&
            s.cloudPatterns > 0) {
            std::swprintf(sky + used, ARRAYSIZE(sky) - used,
                          L"; %llu cloud pattern%s from the HOST",
                          static_cast<unsigned long long>(s.cloudPatterns),
                          s.cloudPatterns == 1 ? L"" : L"s");
        }
    }

    // How the transmit loop is pacing, and -- when flow control is holding
    // frames back -- whether it is the HOST that is setting the pace. The SOF
    // line already shows the rate that was achieved, so a rate well under the
    // one requested means the HOST is the limit.
    wchar_t pacing[64];
    if (!s.flowControl) {
        std::swprintf(pacing, ARRAYSIZE(pacing), L"free-running");
    } else if (s.sending &&
               g_sofRate * 10 < static_cast<uint64_t>(s.targetFps) * 9) {
        std::swprintf(pacing, ARRAYSIZE(pacing),
                      L"paced to the HOST, which is the limit");
    } else {
        std::swprintf(pacing, ARRAYSIZE(pacing), L"paced to the HOST");
    }

    wchar_t text[4096];
    std::swprintf(text, ARRAYSIZE(text),
        L"  HOST %hs:%u   ->  SOF destination\r\n"
        L"  IG   :%u   (this IG is %hs)\r\n"
        L"\r\n"
        L"  Stream    : %s (%s)\r\n"
        L"  SOF rate  : %d Hz requested, %llu/s measured\r\n"
        L"  SOF frames: %u\r\n"
        L"  Received  : %llu datagrams (%llu/s), %llu entity updates\r\n"
        L"  IG mode   : %u (%s)  <- commanded by the HOST\r\n"
        L"  HOST frame: %u\r\n"
        L"\r\n"
        L"%s\r\n"
        L"%s\r\n"
        L"%s\r\n"
        L"%s\r\n"
        L"%s\r\n"
        L"\r\n"
        L"%s\r\n",
        g_link->hostIp, static_cast<unsigned>(g_link->hostPort),
        static_cast<unsigned>(g_link->igPort), g_link->igIp,
        s.sending ? L"RUNNING" : L"STOPPED", pacing,
        s.targetFps,
        static_cast<unsigned long long>(g_sofRate),
        static_cast<unsigned>(s.sofFrames),
        static_cast<unsigned long long>(s.datagrams),
        static_cast<unsigned long long>(g_dataRate),
        static_cast<unsigned long long>(s.entityUpdates),
        static_cast<unsigned>(s.hostIgMode), IgModeName(s.hostIgMode),
        static_cast<unsigned>(s.lastHostFrame),
        relay, relayNic, simlink, world, sky, terrain);

    SetWindowTextW(g_status, text);

    // Reflect the stream state on the buttons instead of letting the operator
    // click START twice.
    EnableWindow(g_start, s.sending ? FALSE : TRUE);
    EnableWindow(g_stop,  s.sending ? TRUE : FALSE);

    // And the preset drop-downs on the live selections.  This is what makes a
    // command-line/default or a mid-run change on the other side show up here;
    // the guard inside leaves the control alone while it is being used.
    SyncCombo(g_timeCombo, s.envTimePreset);
    SyncCombo(g_weatherCombo, s.envWeatherPreset);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        g_hwnd = hwnd;
        HFONT uiFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        g_mono = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             FIXED_PITCH | FF_MODERN, L"Consolas");

        g_status = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                   WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                                       ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                                   kMargin, kMargin, kStatusW, kStatusH,
                                   hwnd, ControlId(kStatusId), nullptr, nullptr);
        g_start = CreateWindowExW(0, L"BUTTON", L"START",
                                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON |
                                      WS_TABSTOP,
                                  kMargin, kButtonY, kButtonW, kButtonH,
                                  hwnd, ControlId(kStartId), nullptr, nullptr);
        g_stop = CreateWindowExW(0, L"BUTTON", L"STOP",
                                 WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                                 kMargin + kButtonW + 10, kButtonY, kButtonW, kButtonH,
                                 hwnd, ControlId(kStopId), nullptr, nullptr);
        HWND exitBtn = CreateWindowExW(0, L"BUTTON", L"EXIT",
                                       WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                                       kClientW - kMargin - kButtonW, kButtonY,
                                       kButtonW, kButtonH,
                                       hwnd, ControlId(kExitId), nullptr, nullptr);

        // ---- Settings row ------------------------------------------------
        g_respond = CreateWindowExW(0, L"BUTTON",
                                    L"Answer HAT/HOT",
                                    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                                    kMargin, kSettingsY, 320, kRowH,
                                    hwnd, ControlId(kRespondId), nullptr, nullptr);
        HWND fpsLabel = CreateWindowExW(0, L"STATIC", L"SOF rate:",
                                        WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                        420, kSettingsY + 3, 74, 18,
                                        hwnd, ControlId(kFpsLabelId), nullptr, nullptr);
        g_fpsEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                    WS_CHILD | WS_VISIBLE | ES_NUMBER |
                                        ES_CENTER | WS_TABSTOP,
                                    500, kSettingsY, 56, kRowH,
                                    hwnd, ControlId(kFpsEditId), nullptr, nullptr);
        HWND fpsUnit = CreateWindowExW(0, L"STATIC", L"Hz",
                                       WS_CHILD | WS_VISIBLE | SS_LEFT,
                                       562, kSettingsY + 3, 18, 18,
                                       hwnd, ControlId(kFpsUnitId), nullptr, nullptr);
        HWND fpsApply = CreateWindowExW(0, L"BUTTON", L"Apply",
                                        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                                        kClientW - kMargin - 64, kSettingsY, 64, kRowH,
                                        hwnd, ControlId(kFpsApplyId), nullptr, nullptr);
        g_flow = CreateWindowExW(0, L"BUTTON",
                                 L"Pace SOF to HOST replies (flow control)",
                                 WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                                 kMargin, kFlagsY, 320, kRowH,
                                 hwnd, ControlId(kFlowId), nullptr, nullptr);

        // ---- Time of day / weather row -----------------------------------
        // Two labelled drop-downs. Each lists "Host" first and then the presets,
        // so the untouched state is "follow the HOST" and a manual choice is the
        // deliberate act. The heights are the dropped-down extents, sized to
        // show every entry without a scroll.
        HWND timeLabel = CreateWindowExW(0, L"STATIC", L"Time of day:",
                                         WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                         kMargin, kEnvY + 3, 80, 18,
                                         hwnd, ControlId(kTimeLabelId),
                                         nullptr, nullptr);
        g_timeCombo = CreateWindowExW(0, L"COMBOBOX", L"",
                                      WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                                          CBS_DROPDOWNLIST | WS_VSCROLL,
                                      kMargin + 84, kEnvY, 160, 140,
                                      hwnd, ControlId(kTimeComboId),
                                      nullptr, nullptr);
        HWND weatherLabel = CreateWindowExW(0, L"STATIC", L"Weather:",
                                            WS_CHILD | WS_VISIBLE | SS_RIGHT,
                                            kMargin + 84 + 160 + 20, kEnvY + 3,
                                            58, 18,
                                            hwnd, ControlId(kWeatherLabelId),
                                            nullptr, nullptr);
        g_weatherCombo = CreateWindowExW(0, L"COMBOBOX", L"",
                                         WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                                             CBS_DROPDOWNLIST | WS_VSCROLL,
                                         kMargin + 84 + 160 + 20 + 58 + 4, kEnvY,
                                         160, 140,
                                         hwnd, ControlId(kWeatherComboId),
                                         nullptr, nullptr);

        if (!g_status || !g_start || !g_stop || !exitBtn || !g_respond ||
            !fpsLabel || !g_fpsEdit || !fpsUnit || !fpsApply || !g_flow ||
            !timeLabel || !g_timeCombo || !weatherLabel || !g_weatherCombo) {
            return -1;  // abort window creation
        }

        // Monospace for the status block, the standard dialog font elsewhere.
        SendMessageW(g_status, WM_SETFONT, reinterpret_cast<WPARAM>(g_mono), TRUE);
        for (HWND c : { g_start, g_stop, exitBtn, g_respond, g_flow, g_fpsEdit,
                        fpsLabel, fpsUnit, fpsApply, timeLabel, g_timeCombo,
                        weatherLabel, g_weatherCombo }) {
            SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        }

        // The drop-downs are populated once and only ever have their selection
        // moved, so the list is stable while the operator scrolls it.
        PopulateTimeCombo(g_timeCombo);
        PopulateWeatherCombo(g_weatherCombo);

        // Seed the controls from the command line: they are the live state from
        // here on, so what the window shows is what the program is doing.
        if (g_hooks && g_hooks->status) {
            const Status s = g_hooks->status();
            SendMessageW(g_respond, BM_SETCHECK,
                         s.respondingHatHot ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageW(g_flow, BM_SETCHECK,
                         s.flowControl ? BST_CHECKED : BST_UNCHECKED, 0);
            SetFpsText(s.targetFps);
            SendMessageW(g_timeCombo, CB_SETCURSEL,
                         static_cast<WPARAM>(s.envTimePreset), 0);
            SendMessageW(g_weatherCombo, CB_SETCURSEL,
                         static_cast<WPARAM>(s.envWeatherPreset), 0);
        }

        // Take the Close item out of the system menu: that is what removes the
        // title-bar X (and the Alt+Space > Close entry).
        if (HMENU sysMenu = GetSystemMenu(hwnd, FALSE)) {
            DeleteMenu(sysMenu, SC_CLOSE, MF_BYCOMMAND);
        }

        SetTimer(hwnd, kRefreshTimer, kRefreshMs, nullptr);
        return 0;
    }

    case WM_TIMER:
        if (wp == kRefreshTimer) {
            Refresh();
            return 0;
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case kStartId:
            if (g_hooks && g_hooks->setSending) g_hooks->setSending(true);
            Refresh();
            return 0;
        case kStopId:
            if (g_hooks && g_hooks->setSending) g_hooks->setSending(false);
            Refresh();
            return 0;
        case kExitId:
            // The one and only exit path. Ask main() to wind the worker
            // threads down, then close the window so run() returns and main()
            // can join them.
            if (g_hooks && g_hooks->requestQuit) g_hooks->requestQuit();
            DestroyWindow(hwnd);
            return 0;
        case kRespondId:
            if (HIWORD(wp) == BN_CLICKED) {
                const bool on =
                    SendMessageW(g_respond, BM_GETCHECK, 0, 0) == BST_CHECKED;
                if (g_hooks && g_hooks->setRespondHatHot) {
                    g_hooks->setRespondHatHot(on);
                }
                Refresh();
            }
            return 0;
        case kFlowId:
            if (HIWORD(wp) == BN_CLICKED) {
                const bool on =
                    SendMessageW(g_flow, BM_GETCHECK, 0, 0) == BST_CHECKED;
                if (g_hooks && g_hooks->setFlowControl) {
                    g_hooks->setFlowControl(on);
                }
                Refresh();
            }
            return 0;
        case kTimeComboId:
            if (HIWORD(wp) == CBN_SELCHANGE) {
                const int sel = static_cast<int>(
                    SendMessageW(g_timeCombo, CB_GETCURSEL, 0, 0));
                if (sel != CB_ERR && g_hooks && g_hooks->setTimePreset) {
                    g_hooks->setTimePreset(sel);
                }
                Refresh();
            }
            return 0;
        case kWeatherComboId:
            if (HIWORD(wp) == CBN_SELCHANGE) {
                const int sel = static_cast<int>(
                    SendMessageW(g_weatherCombo, CB_GETCURSEL, 0, 0));
                if (sel != CB_ERR && g_hooks && g_hooks->setWeatherPreset) {
                    g_hooks->setWeatherPreset(sel);
                }
                Refresh();
            }
            return 0;
        case kFpsApplyId:
            if (HIWORD(wp) == BN_CLICKED) {
                ApplyFps(false);
                Refresh();
            }
            return 0;
        case kFpsEditId:
            // Enter in the rate box, and "clicked somewhere else" both commit.
            if (HIWORD(wp) == EN_KILLFOCUS) {
                ApplyFps(true);
                Refresh();
            }
            return 0;
        case IDOK:
            // IsDialogMessageW() turns Enter into an IDOK command when no
            // button is the default -- and nothing here is one -- so this is
            // how Enter reaches the window. Enter has no other meaning in this
            // window, so it always commits the rate box.
            ApplyFps(false);
            Refresh();
            return 0;
        default:
            break;
        }
        break;

    case WM_SYSCOMMAND:
        // Alt+F4 / system-menu Close. Belt and braces: SC_CLOSE is gone from
        // the menu, but swallow it explicitly anyway.
        if ((wp & 0xFFF0) == SC_CLOSE) {
            return 0;
        }
        break;

    case WM_CLOSE:
        // Deliberately ignored -- EXIT is the only way out.
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, kRefreshTimer);
        if (g_mono) {
            DeleteObject(g_mono);
            g_mono = nullptr;
        }
        g_hwnd = g_status = g_start = g_stop = g_respond = g_flow =
            g_fpsEdit = g_timeCombo = g_weatherCombo = nullptr;
        PostQuitMessage(0);
        return 0;

    default:
        break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

} // namespace

int run(const Hooks& hooks, const Link& link) {
    g_hooks = &hooks;
    g_link  = &link;

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = kClassName;
    if (!RegisterClassExW(&wc)) {
        return 1;
    }

    // Fixed-size window: WS_CAPTION + system menu, but no thick frame and no
    // maximize box. WS_CLIPCHILDREN keeps the parent's background erase out of
    // the children's rectangles -- without it a control can be repainted on top
    // of a stale copy of itself, which shows up as smeared text.
    // AdjustWindowRectEx() below must match these flags.
    const DWORD style   = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                          WS_MINIMIZEBOX | WS_CLIPCHILDREN;
    const DWORD exStyle = WS_EX_CONTROLPARENT;

    RECT r{ 0, 0, kClientW, kClientH };
    AdjustWindowRectEx(&r, style, FALSE, exStyle);

    HWND hwnd = CreateWindowExW(exStyle, kClassName,
                                L"CigiStarter - CIGI 3.3 Image Generator",
                                style, CW_USEDEFAULT, CW_USEDEFAULT,
                                r.right - r.left, r.bottom - r.top,
                                nullptr, nullptr, wc.hInstance, nullptr);
    if (!hwnd) {
        return 1;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    Refresh();

    // If the stream is already running -- the normal case, since the program
    // starts sending on launch -- then the window has nothing to ask for: get
    // it out of the way of whatever else is on the IG desktop. It stays in the
    // taskbar and START / STOP / EXIT keep working as soon as it is restored.
    // Asking the caller rather than assuming keeps the window reacting to the
    // program's state instead of to the command line, which it cannot see.
    if (g_hooks && g_hooks->status && g_hooks->status().sending) {
        ShowWindow(hwnd, SW_MINIMIZE);
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        // Lets Tab / space drive the controls.
        if (IsDialogMessageW(hwnd, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}

} // namespace gui
