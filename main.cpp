//=============================================================================
//  CigiStarter — minimal CIGI 3.3 Image Generator (IG) for a new IG PC
//
//  Role
//    Plays the "Master IG" side of a CIGI link so the HOST begins streaming
//    flight data. The HOST stays silent until the IG transmits Start-of-Frame
//    (SOF) messages, so this program continuously sends SOF at a configurable
//    rate and (optionally) answers HAT/HOT requests.
//
//  Transport (from the reference pcapng capture)
//    HOST 192.168.70.2  -> IG port 8000   (downstream control/flight data)
//    IG   -> HOST 192.168.70.2 : 8001     (upstream SOF / responses)
//
//  Dependencies
//    Windows sockets only (Ws2_32.lib). No third-party libraries.
//
//  Usage
//    CigiStarter.exe [--host-ip 192.168.70.2] [--host-port 8001]
//                    [--bind-ip 0.0.0.0] [--bind-port 8000]
//                    [--fps 100] [--ig-mode 0] [--respond-hathot] [--help]
//=============================================================================
#include "cigi.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

//-----------------------------------------------------------------------------
// Run configuration
//-----------------------------------------------------------------------------
struct Config {
    std::string hostIp   = "192.168.70.2";
    uint16_t    hostPort = 8001;   // port the HOST listens on (IG -> HOST)
    std::string bindIp   = "0.0.0.0";
    uint16_t    bindPort = 8000;   // port the HOST sends to (HOST -> IG)
    int         fps      = 100;    // SOF transmission rate
    uint8_t     igMode   = 0;      // 0 = Standby (0x30), 1 = Operate (0x31)
    bool        respondHatHot = false;
};

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
};

// Hat/Hot responses are queued by the receive thread and flushed by the SOF
// thread so they ride along on the upstream stream (as in the capture).
struct HatHotQueue {
    std::mutex   mtx;
    std::vector<uint16_t> ids;
};

static std::atomic<bool> g_running{true};

//-----------------------------------------------------------------------------
// Ctrl+C / console-close handler
//-----------------------------------------------------------------------------
static BOOL WINAPI consoleHandler(DWORD) {
    g_running.store(false);
    return TRUE;
}

//-----------------------------------------------------------------------------
// Print usage and exit
//-----------------------------------------------------------------------------
static void printUsage(const char* argv0) {
    std::printf(
        "CigiStarter - minimal CIGI 3.3 Image Generator\n\n"
        "Usage: %s [options]\n\n"
        "Options:\n"
        "  --host-ip <ip>      HOST address to send SOF to       (default 192.168.70.2)\n"
        "  --host-port <port>  HOST upstream port (IG -> HOST)   (default 8001)\n"
        "  --bind-ip <ip>      local address to bind for RX      (default 0.0.0.0)\n"
        "  --bind-port <port>  local port to bind for RX         (default 8000)\n"
        "  --fps <rate>        SOF transmit rate in Hz           (default 100)\n"
        "  --ig-mode <0|1>     SOF IG mode 0=Standby 1=Operate   (default 0)\n"
        "  --respond-hathot    answer HAT/HOT requests           (default off)\n"
        "  --help              show this help\n",
        argv0);
}

//-----------------------------------------------------------------------------
// Minimal command-line parser
//-----------------------------------------------------------------------------
static bool parseArgs(int argc, char** argv, Config& cfg) {
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "error: %s requires a value\n", name);
                return nullptr;
            }
            return argv[++i];
        };
        if (a == "--help" || a == "-h") {
            printUsage(argv[0]);
            return false;
        } else if (a == "--host-ip") {
            const char* v = next("--host-ip"); if (!v) return false;
            cfg.hostIp = v;
        } else if (a == "--host-port") {
            const char* v = next("--host-port"); if (!v) return false;
            cfg.hostPort = static_cast<uint16_t>(std::atoi(v));
        } else if (a == "--bind-ip") {
            const char* v = next("--bind-ip"); if (!v) return false;
            cfg.bindIp = v;
        } else if (a == "--bind-port") {
            const char* v = next("--bind-port"); if (!v) return false;
            cfg.bindPort = static_cast<uint16_t>(std::atoi(v));
        } else if (a == "--fps") {
            const char* v = next("--fps"); if (!v) return false;
            cfg.fps = std::atoi(v);
        } else if (a == "--ig-mode") {
            const char* v = next("--ig-mode"); if (!v) return false;
            cfg.igMode = static_cast<uint8_t>(std::atoi(v));
        } else if (a == "--respond-hathot") {
            cfg.respondHatHot = true;
        } else {
            std::fprintf(stderr, "error: unknown option '%s'\n", a.c_str());
            printUsage(argv[0]);
            return false;
        }
    }
    if (cfg.fps <= 0 || cfg.fps > 100000) {
        std::fprintf(stderr, "error: --fps must be in [1, 100000]\n");
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
// Resolve an IPv4 address string into a sockaddr_in (returns false on error)
//-----------------------------------------------------------------------------
static bool resolveV4(const std::string& ip, uint16_t port, sockaddr_in& out) {
    std::memset(&out, 0, sizeof(out));
    out.sin_family = AF_INET;
    out.sin_port = htons(port);
    if (ip.empty() || ip == "0.0.0.0") {
        out.sin_addr.s_addr = INADDR_ANY;
        return true;
    }
    if (inet_pton(AF_INET, ip.c_str(), &out.sin_addr) == 1) {
        return true;
    }
    std::fprintf(stderr, "error: invalid IPv4 address '%s'\n", ip.c_str());
    return false;
}

int main(int argc, char** argv) {
    Config cfg;
    if (!parseArgs(argc, argv, cfg)) {
        return 1;
    }

    // Handle Ctrl+C / console close so we shut down sockets and threads cleanly.
    SetConsoleCtrlHandler(consoleHandler, TRUE);

    // Initialise Winsock.
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::fprintf(stderr, "error: WSAStartup failed (%d)\n", WSAGetLastError());
        return 1;
    }

    // ---- Receive socket: bound to the port the HOST streams to. ----------
    SOCKET rx = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (rx == INVALID_SOCKET) {
        std::fprintf(stderr, "error: socket() failed (%d)\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    sockaddr_in bindAddr;
    if (!resolveV4(cfg.bindIp, cfg.bindPort, bindAddr)) {
        closesocket(rx); WSACleanup();
        return 1;
    }
    if (bind(rx, reinterpret_cast<sockaddr*>(&bindAddr), sizeof(bindAddr)) == SOCKET_ERROR) {
        std::fprintf(stderr, "error: bind(%s:%u) failed (%d)\n",
                     cfg.bindIp.c_str(), cfg.bindPort, WSAGetLastError());
        closesocket(rx); WSACleanup();
        return 1;
    }
    // Short receive timeout so the loop can observe g_running on shutdown.
    DWORD rxTimeout = 500;
    setsockopt(rx, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&rxTimeout),
               sizeof(rxTimeout));

    // ---- Send socket: transmits SOF to the HOST. -------------------------
    SOCKET tx = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (tx == INVALID_SOCKET) {
        std::fprintf(stderr, "error: socket() failed (%d)\n", WSAGetLastError());
        closesocket(rx); WSACleanup();
        return 1;
    }
    sockaddr_in hostAddr;
    if (!resolveV4(cfg.hostIp, cfg.hostPort, hostAddr)) {
        closesocket(rx); closesocket(tx); WSACleanup();
        return 1;
    }

    const uint8_t igModeByte = cfg.igMode ? cigi::kSofIgModeOperate
                                          : cigi::kSofIgModeStandby;

    std::printf(
        "CigiStarter\n"
        "  HOST   : %s:%u  (upstream SOF destination)\n"
        "  RX     : %s:%u  (downstream flight data)\n"
        "  SOF    : %d Hz, IG mode byte 0x%02X\n"
        "  HAT/HOT: %s\n"
        "Press Ctrl+C to stop.\n\n",
        cfg.hostIp.c_str(), cfg.hostPort,
        cfg.bindIp.c_str(), cfg.bindPort,
        cfg.fps, igModeByte,
        cfg.respondHatHot ? "responding" : "ignored");

    Stats stats;
    HatHotQueue hatHot;

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

            stats.datagrams.fetch_add(1);
            stats.bytes.fetch_add(static_cast<uint64_t>(n));

            cigi::forEachMessage(buf.data(), static_cast<size_t>(n),
                                 [&](uint8_t id, const uint8_t* msg, size_t) {
                stats.messages.fetch_add(1);
                switch (id) {
                    case cigi::MsgIGControl:
                        stats.igControl.fetch_add(1);
                        // IGCtrl layout (16 bytes): [id][size][ver][db][mode]
                        // [pad][byte-swap u16][frame-ctr u32][timestamp u32].
                        // The host frame counter is a uint32 LE at full-message
                        // offset 8; echo it back in the SOF's last-host-frame.
                        if (msg[0] == cigi::MsgIGControl && msg[1] >= 12) {
                            const uint32_t fc = static_cast<uint32_t>(msg[8]) |
                                                (static_cast<uint32_t>(msg[9]) << 8) |
                                                (static_cast<uint32_t>(msg[10]) << 16) |
                                                (static_cast<uint32_t>(msg[11]) << 24);
                            stats.lastHostFrame.store(fc);
                        }
                        break;
                    case cigi::MsgEntityControl:
                        stats.entityControl.fetch_add(1);
                        break;
                    case cigi::MsgHATHOTRequest:
                        stats.hatHotReq.fetch_add(1);
                        if (cfg.respondHatHot) {
                            // hat_hot_id is a uint16 LE at full-message offset 2.
                            uint16_t hid = static_cast<uint16_t>(
                                msg[2] | (msg[3] << 8));
                            std::lock_guard<std::mutex> lk(hatHot.mtx);
                            if (hatHot.ids.size() < 64) {
                                hatHot.ids.push_back(hid);
                            }
                        }
                        break;
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
        const auto period = std::chrono::microseconds(1000000 / cfg.fps);
        auto next = std::chrono::steady_clock::now();

        while (g_running.load()) {
            const auto now = std::chrono::steady_clock::now();
            const uint32_t ts = static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(now - started).count());

            cigi::packStartOfFrame(sof, frame, ts, stats.lastHostFrame.load(),
                                   igModeByte);

            // Flush any pending HAT/HOT responses by appending them to the SOF
            // datagram, mirroring the reference capture's upstream framing.
            if (cfg.respondHatHot) {
                std::vector<uint16_t> ids;
                {
                    std::lock_guard<std::mutex> lk(hatHot.mtx);
                    ids.swap(hatHot.ids);
                }
                if (!ids.empty()) {
                    uint8_t pkt[24 + 40 * 64];
                    std::memcpy(pkt, sof, 24);
                    size_t used = 24;
                    for (uint16_t hid : ids) {
                        cigi::packHatHotExtResponse(pkt + used, hid, 1, 0.0, 0.0);
                        used += cigi::kHatHotExtRespSize;
                    }
                    sendto(tx, reinterpret_cast<const char*>(pkt),
                           static_cast<int>(used), 0,
                           reinterpret_cast<sockaddr*>(&hostAddr), sizeof(hostAddr));
                } else {
                    sendto(tx, reinterpret_cast<const char*>(sof), 24, 0,
                           reinterpret_cast<sockaddr*>(&hostAddr), sizeof(hostAddr));
                }
            } else {
                sendto(tx, reinterpret_cast<const char*>(sof), 24, 0,
                       reinterpret_cast<sockaddr*>(&hostAddr), sizeof(hostAddr));
            }

            ++frame;
            next += period;
            std::this_thread::sleep_until(next);
        }
    });

    // ---- Status line loop (main thread). ---------------------------------
    uint64_t lastDatagrams = 0;
    while (g_running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        const uint64_t d = stats.datagrams.load();
        const uint64_t e = stats.entityControl.load();
        std::printf("[%u datagrams/s | total %llu | entities %llu | host frame %u]\n",
                    static_cast<unsigned>(d - lastDatagrams),
                    static_cast<unsigned long long>(d),
                    static_cast<unsigned long long>(e),
                    stats.lastHostFrame.load());
        lastDatagrams = d;
    }

    std::printf("\nShutting down...\n");
    closesocket(rx);
    closesocket(tx);
    if (rxThread.joinable()) rxThread.join();
    if (txThread.joinable()) txThread.join();
    WSACleanup();
    std::printf("Stopped.\n");
    return 0;
}
