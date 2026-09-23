//=============================================================================
//  Relay.cpp — implementation of the client.txt fan-out (see Relay.h)
//
//  Each copy is a whole Ethernet frame, built by hand:
//
//    Ethernet   dst = the client's NIC, src = this IG's NIC, type 0x0800
//    IPv4       src = the HOST,       dst = the client
//    UDP        src = the HOST's port, dst = the client's port
//    payload    the datagram exactly as it arrived from the HOST
//
//  Why the source address is spoofed, and why that forces Npcap
//    The whole point of the relay is that the client IG PCs need no
//    configuration. The accepted way to select the CIGI stream out of an IG
//    PC's own traffic is CigiP3dBridge's live capture filter, and that filter
//    is pinned to the HOST's address: "udp and src host <HOST> and dst port
//    8000". Copies that arrived from *this* IG's address would be ignored by
//    every client, so the copies have to look like they came from the HOST.
//
//    That cannot be done with an ordinary socket, and Windows forbids it on a
//    raw socket too: "UDP datagrams with an invalid source address cannot be
//    sent over raw sockets. The IP source address for any outgoing UDP
//    datagram must exist on a network interface or the datagram is dropped.
//    This change was made to limit the ability of malicious code to create
//    distributed denial-of-service attacks" (Microsoft Learn, "TCP/IP raw
//    sockets"). The only remaining route is a packet-capture driver, so the
//    frame is injected with Npcap's pcap_sendpacket. Npcap is loaded with
//    LoadLibraryW() at run time -- never linked -- so that CigiStarter keeps
//    starting, and keeps working as a plain IG, on a PC without it.
//
//  One frame per client per datagram
//    Unicast, with a destination address resolved per client, so only the IGs
//    listed in client.txt ever see the copies. A datagram too large for one
//    Ethernet payload is dropped instead of fragmented, and counted: the
//    clients' filter matches "dst port 8000", which only ever appears in the
//    first fragment of a fragmented datagram, so fragmenting would hand the
//    clients a truncated CIGI stream. (The reference HOST sends 640..672-byte
//    datagrams, comfortably inside one frame.)
//=============================================================================
#include "Relay.h"

#include <winsock2.h>
#include <ws2tcpip.h>

// <windows.h> must come after <winsock2.h>, never before (see main.cpp).
#include <windows.h>
#include <iphlpapi.h>  // SendARP, GetAdaptersAddresses

#include <pcap.h>      // types only; the entry points are found at run time

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

namespace relay {
namespace {

//-----------------------------------------------------------------------------
// Limits
//-----------------------------------------------------------------------------
constexpr int    kMaxClients     = 32;    // client.txt entries honoured
constexpr size_t kMaxUdpPayload  = 1472;  // 1500 - 20 (IPv4) - 8 (UDP)
constexpr size_t kEthLen         = 14;
constexpr size_t kIpLen          = 20;
constexpr size_t kUdpLen         = 8;
constexpr size_t kMinFrameLen    = 60;    // Ethernet minimum, FCS excluded
constexpr int    kResolveEveryMs = 5000;  // how often the clients are checked
constexpr size_t kMaxConfigBytes = 64 * 1024;

//-----------------------------------------------------------------------------
// State
//
// The client list is fixed once start() has read client.txt; only the `mac` /
// `ready` fields change afterwards, filled in by the resolver thread. Both
// threads agree on the list through g_mtx, which is only ever held for a few
// instructions -- the blocking part of the resolution (SendARP) happens outside
// it, so a client that is switched off can never stall the receive thread.
//-----------------------------------------------------------------------------
struct Client {
    uint32_t ip   = 0;      // host byte order
    uint16_t port = 0;      // host byte order
    uint8_t  mac[6] = {};   // where the frame is addressed (the next hop)
    bool     ready = false; // mac is good: this client answers
};

std::mutex          g_mtx;
std::vector<Client> g_clients;
std::string         g_device;  // adapter description, for the window
std::string         g_error;   // why the relay is off ("" = nothing wrong)
bool                g_fileFound = false;
int                 g_skipped   = 0;

std::atomic<uint64_t> g_forwarded{0};
std::atomic<uint64_t> g_dropped{0};

// Capture side. Written by start() before the worker threads exist, read by the
// receive thread, released by stop() after that thread has been joined.
pcap_t*  g_pcap        = nullptr;
uint8_t  g_srcMac[6]   = {};     // this IG's NIC: the frame's source, because
bool     g_haveSrcMac  = false;  // a switch would otherwise relearn the HOST
uint32_t g_srcIpNet    = 0;      // the HOST address, network order
uint16_t g_srcPort     = 0;      // the HOST port, host order
uint32_t g_localIpNet  = 0;      // this IG's address, network order (SendARP)
uint16_t g_frameId     = 0;      // IPv4 identification, one per datagram

std::atomic<bool> g_resolving{false};
std::thread       g_resolver;

//-----------------------------------------------------------------------------
// Npcap, loaded on demand
//
// Nothing here is linked: an IG PC without Npcap still runs CigiStarter, and
// the relay simply reports that it is off.
//-----------------------------------------------------------------------------
using PFnFindAllDevs = int (*)(pcap_if_t**, char*);
using PFnFreeAllDevs = void (*)(pcap_if_t*);
using PFnOpenLive    = pcap_t* (*)(const char*, int, int, int, char*);
using PFnSendPacket  = int (*)(pcap_t*, const unsigned char*, int);
using PFnClose       = void (*)(pcap_t*);
using PFnGetErr      = char* (*)(pcap_t*);

HMODULE        g_wpcap      = nullptr;
PFnFindAllDevs g_findAllDevs = nullptr;
PFnFreeAllDevs g_freeAllDevs = nullptr;
PFnOpenLive    g_openLive    = nullptr;
PFnSendPacket  g_sendPacket  = nullptr;
PFnClose       g_close       = nullptr;
PFnGetErr      g_getErr      = nullptr;

bool loadNpcap(std::string& error) {
    if (!g_wpcap) {
        // System32 first: that is where Npcap installs, and loading by name
        // without the current directory in the search path stops a stray
        // wpcap.dll next to the executable from shadowing it. The plain load is
        // only a fallback for the unusual Npcap install that puts its files
        // somewhere else.
        g_wpcap = LoadLibraryExW(L"wpcap.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!g_wpcap) {
            g_wpcap = LoadLibraryW(L"wpcap.dll");
        }
    }
    if (!g_wpcap) {
        error = "Npcap is not installed";
        return false;
    }

    g_findAllDevs = reinterpret_cast<PFnFindAllDevs>(
        GetProcAddress(g_wpcap, "pcap_findalldevs"));
    g_freeAllDevs = reinterpret_cast<PFnFreeAllDevs>(
        GetProcAddress(g_wpcap, "pcap_freealldevs"));
    g_openLive = reinterpret_cast<PFnOpenLive>(
        GetProcAddress(g_wpcap, "pcap_open_live"));
    g_sendPacket = reinterpret_cast<PFnSendPacket>(
        GetProcAddress(g_wpcap, "pcap_sendpacket"));
    g_close = reinterpret_cast<PFnClose>(GetProcAddress(g_wpcap, "pcap_close"));
    g_getErr = reinterpret_cast<PFnGetErr>(GetProcAddress(g_wpcap, "pcap_geterr"));

    if (!g_findAllDevs || !g_freeAllDevs || !g_openLive || !g_sendPacket ||
        !g_close || !g_getErr) {
        error = "Npcap is installed but incomplete";
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
// Small helpers
//-----------------------------------------------------------------------------
void put16(uint8_t* p, uint16_t v) {
    p[0] = static_cast<uint8_t>((v >> 8) & 0xFF);
    p[1] = static_cast<uint8_t>(v & 0xFF);
}

std::string dotted(uint32_t ipHostOrder) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%u.%u.%u.%u",
                  (ipHostOrder >> 24) & 0xFFu, (ipHostOrder >> 16) & 0xFFu,
                  (ipHostOrder >> 8) & 0xFFu, ipHostOrder & 0xFFu);
    return buf;
}

// Running 16-bit ones-complement sum, taking the bytes as they appear on the
// wire (big-endian pairs). The result is folded and complemented by checksum().
uint32_t sum16(const uint8_t* data, size_t len, uint32_t sum) {
    for (size_t i = 0; i + 1 < len; i += 2) {
        sum += (static_cast<uint32_t>(data[i]) << 8) | data[i + 1];
    }
    if (len & 1u) {
        sum += static_cast<uint32_t>(data[len - 1]) << 8;
    }
    return sum;
}

uint16_t checksum(uint32_t sum) {
    while (sum >> 16) {
        sum = (sum & 0xFFFFu) + (sum >> 16);
    }
    return static_cast<uint16_t>(~sum & 0xFFFFu);
}

//-----------------------------------------------------------------------------
// client.txt
//-----------------------------------------------------------------------------
std::wstring exeFolder() {
    wchar_t buf[MAX_PATH] = {};
    const DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring path(buf, n);
    const size_t slash = path.find_last_of(L"\\/");
    return (slash == std::wstring::npos) ? std::wstring() : path.substr(0, slash + 1);
}

// Whole file into `out`. Returns false when there is no such file, which is
// not an error: most deployments have no extra IG PCs to feed.
bool readTextFile(const std::wstring& path, std::string& out) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) {
        return false;
    }
    char buf[4096];
    DWORD got = 0;
    while (out.size() < kMaxConfigBytes &&
           ReadFile(h, buf, sizeof(buf), &got, nullptr) && got > 0) {
        out.append(buf, got);
    }
    CloseHandle(h);
    return true;
}

struct ParsedList {
    std::vector<Client> clients;
    int                 skipped = 0;
};

// Parse the client list. Usable lines are `address` or `address:port`; `#`
// starts a comment and blank lines are ignored. Anything else is skipped and
// counted, so an operator who mistypes an address is told about it in the
// window instead of silently feeding four IGs instead of five.
ParsedList parseClientList(const std::string& text,
                           const std::vector<uint32_t>& localIps,
                           uint16_t defaultPort) {
    ParsedList out;
    size_t pos   = 0;
    bool   first = true;

    while (pos <= text.size()) {
        const size_t eol = text.find('\n', pos);
        std::string  line = text.substr(
            pos, (eol == std::string::npos ? text.size() : eol) - pos);
        pos = (eol == std::string::npos) ? text.size() + 1 : eol + 1;

        if (first) {
            first = false;
            // A file written by Notepad begins with a UTF-8 BOM, which would
            // otherwise make the first (and usually only) address unusable.
            if (line.compare(0, 3, "\xEF\xBB\xBF") == 0) {
                line.erase(0, 3);
            }
        }
        const size_t hash = line.find('#');
        if (hash != std::string::npos) {
            line.erase(hash);
        }

        const size_t b = line.find_first_not_of(" \t\r");
        if (b == std::string::npos) {
            continue;  // blank
        }
        line = line.substr(b, line.find_last_not_of(" \t\r") - b + 1);

        std::string hostPart = line;
        std::string portPart;
        const size_t colon = line.find(':');
        if (colon != std::string::npos) {
            hostPart = line.substr(0, colon);
            portPart = line.substr(colon + 1);
            if (portPart.find(':') != std::string::npos) {
                ++out.skipped;  // IPv6, which this deployment does not use
                continue;
            }
        }

        in_addr addr{};
        if (inet_pton(AF_INET, hostPart.c_str(), &addr) != 1) {
            ++out.skipped;
            continue;
        }
        const uint32_t ip = ntohl(addr.s_addr);
        if (ip == 0 || ip == 0xFFFFFFFFu) {
            ++out.skipped;  // "any" / broadcast: never a single IG PC
            continue;
        }

        uint16_t port = defaultPort;
        if (!portPart.empty()) {
            bool     ok    = true;
            unsigned value = 0;
            for (char ch : portPart) {
                if (ch < '0' || ch > '9') { ok = false; break; }
                value = value * 10 + static_cast<unsigned>(ch - '0');
                if (value > 65535) { ok = false; break; }
            }
            if (!ok || value == 0) {
                ++out.skipped;
                continue;
            }
            port = static_cast<uint16_t>(value);
        }

        // An entry for one of our own addresses would have the relay copy its
        // own copies straight back to itself -- a packet storm. Refuse it.
        if (std::find(localIps.begin(), localIps.end(), ip) != localIps.end()) {
            ++out.skipped;
            continue;
        }

        const bool duplicate = std::any_of(
            out.clients.begin(), out.clients.end(),
            [&](const Client& c) { return c.ip == ip && c.port == port; });
        if (duplicate) {
            ++out.skipped;
            continue;
        }
        if (static_cast<int>(out.clients.size()) >= kMaxClients) {
            ++out.skipped;
            continue;
        }

        Client c;
        c.ip   = ip;
        c.port = port;
        out.clients.push_back(c);
    }
    return out;
}

//-----------------------------------------------------------------------------
// This machine's own addresses and adapters
//-----------------------------------------------------------------------------
struct LocalAddr {
    uint32_t ip      = 0;     // host byte order
    uint8_t  mac[6]  = {};
    bool     haveMac = false;
};

std::vector<LocalAddr> localAddresses() {
    std::vector<LocalAddr> out;
    std::vector<uint8_t>   buf(16 * 1024);
    ULONG size = static_cast<ULONG>(buf.size());
    const ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST |
                        GAA_FLAG_SKIP_DNS_SERVER;

    auto* first = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data());
    ULONG rc = GetAdaptersAddresses(AF_INET, flags, nullptr, first, &size);
    if (rc == ERROR_BUFFER_OVERFLOW) {
        buf.resize(size);
        first = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buf.data());
        rc = GetAdaptersAddresses(AF_INET, flags, nullptr, first, &size);
    }
    if (rc != NO_ERROR) {
        return out;
    }

    for (auto* a = first; a; a = a->Next) {
        const bool haveMac = (a->PhysicalAddressLength == 6);
        for (auto* u = a->FirstUnicastAddress; u; u = u->Next) {
            if (!u->Address.lpSockaddr ||
                u->Address.lpSockaddr->sa_family != AF_INET) {
                continue;
            }
            const auto* sin = reinterpret_cast<const sockaddr_in*>(u->Address.lpSockaddr);
            LocalAddr la;
            la.ip      = ntohl(sin->sin_addr.s_addr);
            la.haveMac = haveMac;
            if (haveMac) {
                std::memcpy(la.mac, a->PhysicalAddress, 6);
            }
            out.push_back(la);
        }
    }
    return out;
}

// The address the kernel would use to reach `dstIpHostOrder` -- i.e. the
// adapter this IG's link runs over. A UDP connect() transmits nothing; it only
// asks the stack to choose a route, and therefore a source address.
uint32_t localAddressFor(uint32_t dstIpHostOrder) {
    SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s == INVALID_SOCKET) {
        return 0;
    }
    sockaddr_in to{};
    to.sin_family      = AF_INET;
    to.sin_port        = htons(1);
    to.sin_addr.s_addr = htonl(dstIpHostOrder);

    uint32_t result = 0;
    if (connect(s, reinterpret_cast<const sockaddr*>(&to), sizeof(to)) == 0) {
        sockaddr_in me{};
        int len = sizeof(me);
        if (getsockname(s, reinterpret_cast<sockaddr*>(&me), &len) == 0) {
            result = ntohl(me.sin_addr.s_addr);
        }
    }
    closesocket(s);
    return result;
}

// The capture device that owns `ip`. Matching on the address rather than on a
// name means the right adapter is found whatever the machine calls it.
std::string deviceForAddress(uint32_t ip, std::string& description,
                             std::string& error) {
    char errbuf[PCAP_ERRBUF_SIZE] = {};
    pcap_if_t* all = nullptr;
    if (g_findAllDevs(&all, errbuf) != 0) {
        error = "Npcap: ";
        error += (errbuf[0] != '\0') ? errbuf : "cannot list capture adapters";
        return std::string();
    }

    std::string name;
    for (pcap_if_t* d = all; d; d = d->next) {
        for (pcap_addr_t* a = d->addresses; a; a = a->next) {
            if (!a->addr || a->addr->sa_family != AF_INET) {
                continue;
            }
            const auto* sin = reinterpret_cast<const sockaddr_in*>(a->addr);
            if (ntohl(sin->sin_addr.s_addr) != ip) {
                continue;
            }
            name        = d->name;
            description = (d->description != nullptr) ? d->description : d->name;
            break;
        }
        if (!name.empty()) {
            break;
        }
    }
    g_freeAllDevs(all);

    if (name.empty()) {
        error = "no capture adapter holds " + dotted(ip);
    }
    return name;
}

//-----------------------------------------------------------------------------
// The frame
//-----------------------------------------------------------------------------
// Assemble one Ethernet + IPv4 + UDP frame carrying `len` payload bytes to
// `client`, as if the HOST had sent them. `out` must hold
// kEthLen + kIpLen + kUdpLen + kMaxUdpPayload bytes, which is the largest frame
// this program will ever build. Returns the frame length, padding included.
size_t buildFrame(uint8_t* out, const Client& client, const uint8_t* payload,
                  size_t len, uint16_t id) {
    uint8_t* eth = out;
    uint8_t* ip  = out + kEthLen;
    uint8_t* udp = ip + kIpLen;
    uint8_t* pay = udp + kUdpLen;
    const size_t udpTotal = kUdpLen + len;

    // ---- Ethernet ----
    std::memcpy(eth + 0, client.mac, 6);
    std::memcpy(eth + 6, g_srcMac, 6);
    put16(eth + 12, 0x0800);  // IPv4

    // ---- IPv4 ----
    ip[0] = 0x45;             // version 4, five 32-bit words of header
    ip[1] = 0x00;             // DSCP / ECN
    put16(ip + 2, static_cast<uint16_t>(kIpLen + udpTotal));
    put16(ip + 4, id);
    put16(ip + 6, 0x0000);    // no flags, no fragment offset
    ip[8] = 128;              // TTL, the Windows default
    ip[9] = 17;               // UDP
    put16(ip + 10, 0);        // checksum, filled in once the header is complete
    std::memcpy(ip + 12, &g_srcIpNet, 4);
    const uint32_t dstNet = htonl(client.ip);
    std::memcpy(ip + 16, &dstNet, 4);
    put16(ip + 10, checksum(sum16(ip, kIpLen, 0)));

    // ---- UDP ----
    put16(udp + 0, g_srcPort);
    put16(udp + 2, client.port);
    put16(udp + 4, static_cast<uint16_t>(udpTotal));
    put16(udp + 6, 0);        // checksum, filled in below
    std::memcpy(pay, payload, len);

    // The UDP checksum covers a pseudo-header (source, destination, protocol,
    // UDP length) followed by the UDP header and payload. Checksumming the
    // whole thing is what makes the copies indistinguishable from the real
    // datagrams: a client IG that reads them through an ordinary socket -- not
    // just through Npcap -- receives them normally.
    uint8_t pseudo[12];
    std::memcpy(pseudo + 0, ip + 12, 4);
    std::memcpy(pseudo + 4, ip + 16, 4);
    pseudo[8] = 0;
    pseudo[9] = 17;
    put16(pseudo + 10, static_cast<uint16_t>(udpTotal));

    uint16_t csum = checksum(sum16(udp, udpTotal, sum16(pseudo, sizeof(pseudo), 0)));
    if (csum == 0) {
        csum = 0xFFFF;  // 0 means "no checksum" in UDP over IPv4
    }
    put16(udp + 6, csum);

    // Short frames are padded to the Ethernet minimum; the NIC appends the
    // FCS. The padding is not part of the IP datagram, so neither length nor
    // checksum above account for it.
    size_t frameLen = kEthLen + kIpLen + udpTotal;
    if (frameLen < kMinFrameLen) {
        std::memset(out + frameLen, 0, kMinFrameLen - frameLen);
        frameLen = kMinFrameLen;
    }
    return frameLen;
}

//-----------------------------------------------------------------------------
// Resolving the clients
//
// SendARP answers from the ARP cache when it can and otherwise sends a request,
// which means it blocks for a second or more when a client is switched off.
// That is why it runs here, on its own thread, and never on the receive path.
//
// stop() clears the flag and joins, so the loop leaves after the probe it is
// already inside, at the latest. The flag is therefore checked *between* probes
// as well as between passes: with several switched-off clients in client.txt,
// checking only between passes would keep the process alive for seconds after
// the window has closed, still holding the CIGI port.
//-----------------------------------------------------------------------------
void resolveLoop() {
    while (g_resolving.load()) {
        std::vector<Client> snapshot;
        {
            std::lock_guard<std::mutex> lk(g_mtx);
            snapshot = g_clients;
        }

        for (const Client& c : snapshot) {
            // SendARP can block for a second or more, so check before starting
            // one: on shutdown the join() below must wait for the probe in
            // flight, not for the rest of the list.
            if (!g_resolving.load()) {
                break;
            }

            uint8_t mac[8] = {};
            ULONG   macLen = sizeof(mac);
            const DWORD rc = SendARP(htonl(c.ip), g_localIpNet, mac, &macLen);
            const bool  ok = (rc == NO_ERROR && macLen >= 6);

            std::lock_guard<std::mutex> lk(g_mtx);
            for (Client& t : g_clients) {
                if (t.ip != c.ip || t.port != c.port) {
                    continue;
                }
                if (ok) {
                    std::memcpy(t.mac, mac, 6);
                    t.ready = true;
                } else {
                    // Not answering (yet): stop feeding it until it does, and
                    // pick it up on a later pass if it is switched on again.
                    t.ready = false;
                }
                break;
            }
        }

        for (int waited = 0; waited < kResolveEveryMs && g_resolving.load();
             waited += 100) {
            Sleep(100);
        }
    }
}

} // namespace

//-----------------------------------------------------------------------------
// Public interface
//-----------------------------------------------------------------------------
void start(const char* hostIp, uint16_t hostPort, const char* igIp, uint16_t igPort) {
    if (g_pcap != nullptr || g_resolving.load()) {
        return;  // already started
    }

    // The copies carry the HOST's address and port as their source.
    in_addr host{};
    in_addr ig{};
    uint32_t hostOrder = 0;
    uint32_t igOrder   = 0;
    if (inet_pton(AF_INET, hostIp, &host) == 1) {
        g_srcIpNet = host.s_addr;
        hostOrder  = ntohl(host.s_addr);
    }
    if (inet_pton(AF_INET, igIp, &ig) == 1) {
        igOrder = ntohl(ig.s_addr);
    }
    g_srcPort = hostPort;

    // client.txt first: with no extra IG PCs there is nothing to relay, and the
    // capture driver should not be touched at all.
    std::string text;
    g_fileFound = readTextFile(exeFolder() + L"client.txt", text);

    const std::vector<LocalAddr> locals = localAddresses();
    std::vector<uint32_t>        localIps;
    localIps.reserve(locals.size());
    for (const LocalAddr& a : locals) {
        localIps.push_back(a.ip);
    }

    if (g_fileFound) {
        const ParsedList parsed = parseClientList(text, localIps, igPort);
        g_skipped = parsed.skipped;
        g_clients = parsed.clients;
    }
    if (g_clients.empty()) {
        return;  // nothing to feed; the window says why
    }

    // Which adapter carries this IG? Its own address when the machine actually
    // has it (the deployment case), otherwise whichever adapter the route to
    // the HOST leaves by -- so the program still works on a test box that is
    // not configured with the IG address.
    uint32_t wanted = 0;
    if (igOrder != 0 &&
        std::find(localIps.begin(), localIps.end(), igOrder) != localIps.end()) {
        wanted = igOrder;
    } else {
        wanted = localAddressFor(hostOrder);
    }
    if (wanted == 0) {
        g_error = "no route to the HOST " + dotted(hostOrder);
        return;
    }

    // The frame's source hardware address has to be the adapter's own: spoofing
    // it would teach every switch on the way that the HOST lives on this port.
    for (const LocalAddr& a : locals) {
        if (a.ip == wanted && a.haveMac) {
            std::memcpy(g_srcMac, a.mac, 6);
            g_haveSrcMac = true;
            break;
        }
    }
    if (!g_haveSrcMac) {
        g_error = "adapter " + dotted(wanted) + " reports no hardware address";
        return;
    }
    g_localIpNet = htonl(wanted);

    if (!loadNpcap(g_error)) {
        return;
    }

    std::string       description;
    const std::string device = deviceForAddress(wanted, description, g_error);
    if (device.empty()) {
        return;
    }

    char errbuf[PCAP_ERRBUF_SIZE] = {};
    pcap_t* handle = g_openLive(device.c_str(),
                                /*snaplen=*/100,   // send-only: never captures
                                /*promisc=*/0,
                                /*timeout_ms=*/1000,
                                errbuf);
    if (handle == nullptr) {
        g_error = (errbuf[0] != '\0') ? errbuf : "cannot open the capture adapter";
        return;
    }

    g_pcap   = handle;
    g_device = description;

    g_resolving.store(true);
    g_resolver = std::thread(resolveLoop);
}

void stop() {
    g_resolving.store(false);
    if (g_resolver.joinable()) {
        g_resolver.join();
    }
    if (g_pcap != nullptr) {
        if (g_close != nullptr) {
            g_close(g_pcap);
        }
        g_pcap = nullptr;
    }
}

void forward(const uint8_t* data, size_t len) {
    if (g_pcap == nullptr) {
        return;  // relay off: no client.txt, or no capture adapter for it
    }
    if (len > kMaxUdpPayload) {
        g_dropped.fetch_add(1);  // would need fragmenting; see the file header
        return;
    }

    std::lock_guard<std::mutex> lk(g_mtx);
    if (g_clients.empty()) {
        return;
    }

    const uint16_t id = ++g_frameId;
    uint8_t frame[kEthLen + kIpLen + kUdpLen + kMaxUdpPayload];

    bool anyReady  = false;
    int  delivered = 0;
    for (const Client& c : g_clients) {
        if (!c.ready) {
            continue;
        }
        anyReady = true;
        const size_t n = buildFrame(frame, c, data, len, id);
        if (g_sendPacket(g_pcap, frame, static_cast<int>(n)) == 0) {
            ++delivered;
        }
    }

    if (delivered > 0) {
        g_forwarded.fetch_add(1);
    } else if (anyReady) {
        // Every send failed. Clients that have not answered ARP yet are not
        // counted: nothing was expected of them.
        g_dropped.fetch_add(1);
    }
}

Status status() {
    Status s;
    std::lock_guard<std::mutex> lk(g_mtx);
    s.fileFound = g_fileFound;
    s.clients   = static_cast<int>(g_clients.size());
    s.skipped   = g_skipped;
    s.device    = g_device;
    s.error     = g_error;
    for (const Client& c : g_clients) {
        if (c.ready) {
            ++s.ready;
        }
    }
    s.forwarded = g_forwarded.load();
    s.dropped   = g_dropped.load();
    return s;
}

} // namespace relay
