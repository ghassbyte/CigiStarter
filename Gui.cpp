//=============================================================================
//  Gui.cpp — plain Win32 implementation of the CigiStarter window
//
//  See Gui.h for the rationale. Layout is a fixed-size window (no resize) with
//  a read-only multi-line edit for the status block and three buttons:
//
//     +--------------------------------------------------------------+
//     |  HOST 192.168.70.2:8001  ->  SOF destination                 |
//     |  ... live status ...                                         |
//     |                                                              |
//     |  [ START ]  [ STOP ]                                 [ EXIT ] |
//     +--------------------------------------------------------------+
//
//  There is no X button: SC_CLOSE is deleted from the system menu at creation
//  and WM_CLOSE is ignored, so DestroyWindow() is only ever reached from the
//  EXIT handler.
//=============================================================================
#include "Gui.h"

#include <windows.h>

#include <cstdio>
#include <cwchar>

namespace gui {
namespace {

// ---- Layout (client-area pixels) -----------------------------------------
constexpr int kMargin  = 12;
constexpr int kClientW = 620;
constexpr int kClientH = 320;
constexpr int kButtonW = 110;
constexpr int kButtonH = 32;
constexpr int kButtonY = kClientH - kMargin - kButtonH;
constexpr int kStatusW = kClientW - 2 * kMargin;
constexpr int kStatusH = kButtonY - 2 * kMargin;

// ---- Control identifiers -------------------------------------------------
constexpr int kStatusId = 1000;
constexpr int kStartId  = 1001;
constexpr int kStopId   = 1002;
constexpr int kExitId   = 1003;

// How often the status block is rewritten (ms).
constexpr UINT_PTR kRefreshTimer = 1;
constexpr UINT     kRefreshMs    = 250;

constexpr wchar_t kClassName[] = L"CigiStarterWindow";

// ---- Window state --------------------------------------------------------
const Hooks* g_hooks = nullptr;
const Link*  g_link  = nullptr;

HWND  g_status = nullptr;
HWND  g_start  = nullptr;
HWND  g_stop   = nullptr;
HFONT g_mono   = nullptr;

// Datagram-rate sampling: the counters are cumulative, so the window keeps its
// own previous sample and the time it was taken.
uint64_t g_lastDatagrams  = 0;
uint64_t g_lastSampleTick = 0;
uint64_t g_ratePerSecond  = 0;

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

// Rebuild the status block and sync the button enable state.
void Refresh() {
    if (!g_hooks || !g_hooks->status || !g_status) {
        return;
    }
    const Status s = g_hooks->status();

    const uint64_t now = GetTickCount64();
    if (g_lastSampleTick == 0) {
        g_lastSampleTick = now;
        g_lastDatagrams  = s.datagrams;
    } else if (now - g_lastSampleTick >= 1000) {
        const uint64_t dt = now - g_lastSampleTick;
        g_ratePerSecond = (s.datagrams - g_lastDatagrams) * 1000u / dt;
        g_lastDatagrams  = s.datagrams;
        g_lastSampleTick = now;
    }

    // The terrain lines have their own branches because they can be missing.
    wchar_t terrain[160];
    if (!s.respondingHatHot) {
        std::swprintf(terrain, 160,
                      L"  HAT/HOT   : not answered (--respond-hathot off)");
    } else if (s.terrainConnected) {
        std::swprintf(terrain, 160,
                      L"  HAT/HOT   : responding from FSUIPC\r\n"
                      L"  Terrain   : %.1f m under aircraft, %.1f m aircraft",
                      s.groundAltMeters, s.aircraftAltMeters);
    } else {
        std::swprintf(terrain, 160,
                      L"  HAT/HOT   : responding, but no FSUIPC link to Prepar3D");
    }

    wchar_t text[2048];
    std::swprintf(text, 2048,
        L"  HOST %hs:%u   ->  SOF destination\r\n"
        L"  IG   :%u   (this IG is %hs)\r\n"
        L"\r\n"
        L"  Stream    : %s\r\n"
        L"  SOF frames: %u\r\n"
        L"  Received  : %llu datagrams (%llu/s), %llu entity updates\r\n"
        L"  IG mode   : %u (%s)  <- commanded by the HOST\r\n"
        L"  HOST frame: %u\r\n"
        L"\r\n"
        L"%s\r\n",
        g_link->hostIp, static_cast<unsigned>(g_link->hostPort),
        static_cast<unsigned>(g_link->igPort), g_link->igIp,
        s.sending ? L"RUNNING" : L"STOPPED",
        static_cast<unsigned>(s.sofFrames),
        static_cast<unsigned long long>(s.datagrams),
        static_cast<unsigned long long>(g_ratePerSecond),
        static_cast<unsigned long long>(s.entityUpdates),
        static_cast<unsigned>(s.hostIgMode), IgModeName(s.hostIgMode),
        static_cast<unsigned>(s.lastHostFrame),
        terrain);

    SetWindowTextW(g_status, text);

    // Reflect the stream state on the buttons instead of letting the operator
    // click START twice.
    EnableWindow(g_start, s.sending ? FALSE : TRUE);
    EnableWindow(g_stop,  s.sending ? TRUE : FALSE);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        HFONT uiFont = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        g_mono = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                             CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                             FIXED_PITCH | FF_MODERN, L"Consolas");

        g_status = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                   WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                                       ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
                                   kMargin, kMargin, kStatusW, kStatusH,
                                   hwnd, ControlId(kStatusId),
                                   nullptr, nullptr);
        g_start = CreateWindowExW(0, L"BUTTON", L"START",
                                  WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON |
                                      WS_TABSTOP,
                                  kMargin, kButtonY, kButtonW, kButtonH,
                                  hwnd, ControlId(kStartId),
                                  nullptr, nullptr);
        g_stop = CreateWindowExW(0, L"BUTTON", L"STOP",
                                 WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                                 kMargin + kButtonW + 10, kButtonY, kButtonW, kButtonH,
                                 hwnd, ControlId(kStopId),
                                 nullptr, nullptr);
        HWND exitBtn = CreateWindowExW(0, L"BUTTON", L"EXIT",
                                       WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                                       kClientW - kMargin - kButtonW, kButtonY,
                                       kButtonW, kButtonH,
                                       hwnd, ControlId(kExitId),
                                       nullptr, nullptr);

        if (!g_status || !g_start || !g_stop || !exitBtn) {
            return -1;  // abort window creation
        }
        // Monospace for the status block, the standard dialog font for buttons.
        SendMessageW(g_status, WM_SETFONT, reinterpret_cast<WPARAM>(g_mono), TRUE);
        SendMessageW(g_start,  WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(g_stop,   WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(exitBtn,  WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);

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
        g_status = g_start = g_stop = nullptr;
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
    // maximize box. AdjustWindowRectEx() below must match these flags.
    const DWORD style   = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
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
        // Lets Tab / space drive the buttons.
        if (IsDialogMessageW(hwnd, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}

} // namespace gui
