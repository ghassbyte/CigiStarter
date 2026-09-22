//=============================================================================
//  Relay.h — copies the HOST's CIGI stream to the extra (passive) IG PCs
//
//  Why this exists
//    The HOST addresses its flight data to a single IG. A deployment that has
//    more than one IG PC therefore needs the IG the HOST does talk to -- this
//    one -- to pass every datagram on, so that every IG shows the same
//    aircraft. The extra IG PCs are passive: they only need the data, they
//    never answer it.
//
//  What it does
//    `client.txt`, next to CigiStarter.exe, lists the extra IG PCs, one
//    `address[:port]` per line (the port defaults to the IG port, 8000).
//    Every datagram the HOST sends to this IG is then re-sent, byte for byte,
//    to each client -- carrying the HOST's own address and port as its source,
//    so that the clients need no configuration at all: CigiP3dBridge's default
//    live filter (`udp and src host <HOST> and dst port 8000`) picks the copies
//    up exactly as it picks up the real thing.
//
//    The copies are whole Ethernet frames injected with Npcap
//    (`pcap_sendpacket`). That is not a preference: Windows will not send a
//    UDP datagram whose source address is not local, on a raw socket or
//    otherwise -- see the note at the top of Relay.cpp. Npcap is loaded at run
//    time rather than linked, so this program still runs as a plain IG on an IG
//    PC that does not have it; the relay just reports that it is off.
//
//  Never fatal
//    No client.txt, no Npcap, a client that is switched off: none of these stop
//    CigiStarter being an IG. Each is reported in the operator window's status
//    block (`gui::Status`) and nothing else. The relay is read once, at
//    startup: editing client.txt takes effect on the next run.
//=============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace relay {

// Start the fan-out: read client.txt from the folder holding the executable,
// open the network adapter that carries this IG, and begin resolving the
// clients' hardware addresses on a background thread. Call once, after the
// link addresses are known. Never fails -- see the note above.
//
//   hostIp / hostPort  the HOST: the copies carry this address and port as
//                      their source, because a client IG PC identifies the CIGI
//                      stream by looking for the HOST's address
//   igIp   / igPort    this IG: igIp chooses the adapter the copies leave by
//                      (falling back to the route to the HOST when the machine
//                      does not carry that address itself), igPort is the port
//                      a client.txt line gets when it does not name one
void start(const char* hostIp, uint16_t hostPort,
           const char* igIp,   uint16_t igPort);

// Stop the resolver thread and close the capture handle. Safe to call when
// never started, and may be followed by another start(). The caller must have
// stopped calling forward() first (main() joins the receive thread before it
// gets here).
void stop();

// Send one datagram, unchanged, to every client whose hardware address is
// known. Called on the receive thread; does nothing while the relay is off.
void forward(const uint8_t* data, size_t len);

// What the operator window shows about the relay. All of it comes from one
// read of client.txt and the adapter the relay injects on, plus two running
// counters.
struct Status {
    bool     fileFound = false;  // client.txt was there to be read
    int      clients   = 0;      // usable entries in it
    int      ready     = 0;      // entries whose hardware address is known
    int      skipped   = 0;      // client.txt lines that could not be used
    uint64_t forwarded = 0;      // datagrams copied to at least one client
    uint64_t dropped   = 0;      // datagrams no client could receive
    std::string device;          // adapter the copies leave by ("" = none)
    std::string error;           // why the relay is off ("" = nothing wrong)
};

Status status();

} // namespace relay
