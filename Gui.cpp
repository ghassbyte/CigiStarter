//=============================================================================
//  Gui.cpp — plain Win32 implementation of the CigiStarter window
//
//  See Gui.h for the rationale. The layout is a fixed-size window (no resize):
//
//     +--------------------------------------------------------------------+
//     |  HOST 192.168.70.2:8001   ->  SOF destination                       |
//     |   ... live status block (read-only edit): the link, the stream, the |
//     |       client.txt fan-out and the terrain sample ...                 |
//     |                                                                     |
//     |  [x] Answer HAT/HOT (terrain from FSUIPC)  SOF rate: [ 60 ] Hz [Apply] |
//     |  [x] Pace SOF to HOST replies (flow control)                        |
//     |                                                                     |
//     |  [ START ]  [ STOP ]                                        [ EXIT ] |
//     +--------------------------------------------------------------------+
//
//  The settings rows are live: the check boxes switch HAT/HOT answering (and
//  with it the FSUIPC poller) and the flow control, and the rate box retunes the
//  SOF stream, all without stopping anything first. The rate box applies on
//  Enter, on Apply, and when it loses focus, so a typed value can never be
//  silently ignored.
//
//  There is no X button: SC_CLOSE is deleted from the system menu at creation
//  and WM_CLOSE is ignored, so DestroyWindow() is only ever reached from the
//  EXIT handler.
//=============================================================================
#include "Gui.h"

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
// relay line grows with the counters, and a line that wraps must not push the
// terrain reading out of sight.
constexpr int kStatusH   = 288;
constexpr int kSettingsY = kMargin + kStatusH + kMargin;
constexpr int kRowH      = 24;
// The flow-control check box gets a row of its own under the HAT/HOT row: there
// is no width left beside it, and a squeezed label would be worse than a taller
// window.
constexpr int kFlagsY    = kSettingsY + kRowH;
constexpr int kButtonY   = kFlagsY + kRowH + kMargin;
constexpr int kButtonW   = 110;
constexpr int kButtonH   = 32;
constexpr int kClientH   = kButtonY + kButtonH + kMargin;

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
    wchar_t terrain[192];
    if (!s.respondingHatHot) {
        std::swprintf(terrain, ARRAYSIZE(terrain),
                      L"  HAT/HOT   : not answered (switched off)");
    } else if (s.terrainConnected) {
        std::swprintf(terrain, ARRAYSIZE(terrain),
                      L"  HAT/HOT   : answering from FSUIPC\r\n"
                      L"  Terrain   : %.1f m under the aircraft, %.1f m aircraft",
                      s.groundAltMeters, s.aircraftAltMeters);
    } else {
        std::swprintf(terrain, ARRAYSIZE(terrain),
                      L"  HAT/HOT   : switched on, but no FSUIPC link to Prepar3D");
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

    wchar_t relayNic[256];
    if (s.relayDevice.empty()) {
        std::swprintf(relayNic, ARRAYSIZE(relayNic), L"  Relay NIC : none");
    } else {
        std::swprintf(relayNic, ARRAYSIZE(relayNic), L"  Relay NIC : %hs",
                      s.relayDevice.c_str());
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

    wchar_t text[2048];
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
        relay, relayNic, terrain);

    SetWindowTextW(g_status, text);

    // Reflect the stream state on the buttons instead of letting the operator
    // click START twice.
    EnableWindow(g_start, s.sending ? FALSE : TRUE);
    EnableWindow(g_stop,  s.sending ? TRUE : FALSE);
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
                                    L"Answer HAT/HOT (terrain from FSUIPC)",
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

        if (!g_status || !g_start || !g_stop || !exitBtn || !g_respond ||
            !fpsLabel || !g_fpsEdit || !fpsUnit || !fpsApply || !g_flow) {
            return -1;  // abort window creation
        }

        // Monospace for the status block, the standard dialog font elsewhere.
        SendMessageW(g_status, WM_SETFONT, reinterpret_cast<WPARAM>(g_mono), TRUE);
        for (HWND c : { g_start, g_stop, exitBtn, g_respond, g_flow, g_fpsEdit,
                        fpsLabel, fpsUnit, fpsApply }) {
            SendMessageW(c, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        }

        // Seed the controls from the command line: they are the live state from
        // here on, so what the window shows is what the program is doing.
        if (g_hooks && g_hooks->status) {
            const Status s = g_hooks->status();
            SendMessageW(g_respond, BM_SETCHECK,
                         s.respondingHatHot ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessageW(g_flow, BM_SETCHECK,
                         s.flowControl ? BST_CHECKED : BST_UNCHECKED, 0);
            SetFpsText(s.targetFps);
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
            g_fpsEdit = nullptr;
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
