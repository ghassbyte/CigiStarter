#!/usr/bin/env python3
"""Build docs/cigi-starter-map.excalidraw -- the whole CigiStarter project on
one canvas.

Every panel is a zone; the coordinates are chosen by hand (Excalidraw has no
auto-layout), on a 60px margin / 60px panel padding / 40px sibling gap scale.

    python docs/cigi-starter-map.py

Writes the .excalidraw, an .svg and a .png preview next to this file.
Regenerate rather than hand-editing the scene: this script is the source of
truth for the geometry.
"""

import os
import sys

sys.path.insert(0, os.path.expanduser(
    r"~\.config\opencode\skills\excalidraw\scripts"))
from excalidraw import Scene, FONT_HAND, FONT_NORMAL, FONT_CODE  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "cigi-starter-map")

s = Scene(seed=2026)

# --------------------------------------------------------------------------
# helpers
# --------------------------------------------------------------------------


def txt(x, y, body, size=13, font=FONT_CODE, **kw):
    return s.text(x, y, body, size=size, font=font, **kw)


def panel(x, y, w, h, title):
    s.zone(x, y, w, h, title, size=19)
    return x + 60, y + 60  # content origin


# --------------------------------------------------------------------------
# title
# --------------------------------------------------------------------------

txt(60, 30, "CigiStarter - a CIGI 3.3 Image Generator", size=34, font=FONT_HAND,
    color="#1e1e1e")
txt(60, 86,
    "A minimal replacement for the Master IG PC: wakes the HOST with SOF, answers its HAT/HOT requests from the terrain Prepar3D will actually fly the aircraft onto "
    "(the PdkTerrainProbe add-on), falls back to an offline SRTM database and then to the sim over FSUIPC, and fans the HOST's downstream stream out to the extra IG PCs. "
    "CIGI 3.3, UDP, little-endian. Single Win32 binary, runs unelevated, streams from launch with the window minimised.",
    size=16, font=FONT_NORMAL, width=3260)

# ==========================================================================
# 1 - the link
# ==========================================================================

cx, cy = panel(60, 150, 2200, 560, "1 - The link: HOST, IG, extra IG PCs")

txt(cx, cy + 2,
    "Deployment defaults: hardcoded in src\\main.cpp\n"
    "  HOST 192.168.70.2    IG 192.168.70.70\n"
    "  --host-ip / --host-port / --ig-ip / --ig-port\n"
    "  exist for a bench test only; the deployment\n"
    "  passes none of them",
    size=12.5)

host = s.box(160, 310, 420, None,
             "HOST  192.168.70.2\nflight model, owns the frame rate\n"
             "receives SOF on :8001\nsends flight data from :8000",
             fill="blue", size=15)
ig = s.box(1000, 310, 460, None,
           "IG - CigiStarter  192.168.70.70\nbinds 0.0.0.0:8000 for flight data\n"
           "sends SOF to 192.168.70.2:8001\nrides HAT/HOT responses on the SOF stream",
           fill="green", size=15)
clients = s.box(1780, 310, 420, None,
                "Extra IG PCs  192.168.70.102 ... .106\nUDP 8000, passive:\n"
                "nothing ever comes back",
                fill="grey", dashed=True, size=15)

s.arrow(ig, host, bend=(790, 235),
        label="SOF - id 101, 24 B - the request that wakes the HOST",
        label_size=13, label_font=FONT_NORMAL)
s.arrow(host, ig, bend=(790, 480),
        label="flight data: IGCtrl - EntityCtrl x3 - HAT/HOT Request - ViewCtrl - ArticPart x5 - RateCtrl x8",
        label_size=12, label_font=FONT_NORMAL)
s.arrow(ig, clients, label="one copy per datagram", label_size=13,
        label_font=FONT_NORMAL, dashed=True)

txt(1780, 430,
    "The copies keep the HOST's own address and port as\n"
    "their source (192.168.70.2:8001), so a client's\n"
    "bridge needs no reconfiguration: it sees exactly\n"
    "the stream it would have seen as the IG.",
    size=12.5)

txt(cx, 540,
    "Downstream   HOST -> IG :8000      IGCtrl - EntityCtrl x3 - HAT/HOT Request - ViewCtrl - ArticPart x5 - RateCtrl x8 - Atmosphere (warm-up)\n"
    "Upstream     IG -> HOST :8001      SOF id 101 (+ the HAT/HOT responses, ids 102/103, when answering)\n"
    "Fan-out      HOST -> each client   one copy of every downstream datagram, labelled with the HOST's source",
    size=12.5)

# ==========================================================================
# 2 - the per-frame loop
# ==========================================================================

cx, cy = panel(2300, 150, 1080, 560, "2 - The per-frame loop")

ig2 = s.box(2360, 270, 340, None, "IG (CigiStarter)\n192.168.70.70",
            fill="green", size=15)
host2 = s.box(2880, 270, 380, None, "HOST\n192.168.70.2", fill="blue", size=15)
s.arrow(ig2, host2, bend=(2770, 215), label="SOF, frame N",
        label_size=13, label_font=FONT_NORMAL)
s.arrow(host2, ig2, bend=(2770, 410), label="one datagram per SOF",
        label_size=13, label_font=FONT_NORMAL)

txt(2360, 450,
    "The HOST is a request/response slave: it sends nothing until the IG sends a Start-of-Frame. "
    "Each SOF is answered with one datagram carrying:",
    size=14, font=FONT_NORMAL, width=900)
txt(2360, 505,
    "IG Control x1 - Entity Control x3 - HAT/HOT Request x1 - View Control x1\n"
    "Articulated Part Control x5 - Rate Control x8\n"
    "Atmosphere Control x1 (first frames only, warm-up)",
    size=12.5)
txt(2360, 580,
    "The SOF echoes the HOST's own frame counter at offset 8 - never a count of the SOFs the IG sent - "
    "so the outgoing SOF stays byte-identical to the reference Master's.",
    size=14, font=FONT_NORMAL, width=900)

# ==========================================================================
# 3 - protocol and wire layout
# ==========================================================================

cx, cy = panel(60, 760, 1080, 840,
               "3 - Protocol and wire layout (CIGI 3.3, little-endian)")

txt(cx, cy,
    "framing: [id 1B][size 1B][payload]    size = total bytes incl. header\n"
    "\n"
    "SOF   id 101 (0x65), 24 bytes\n"
    "  65 18 03 00 00 b5 00 80 <frame counter> 00 00 00 00 00 .. 00   the reference\n"
    "  65 18 03 00 00 30 00 80 <frame counter> 00 00 00 00 00 .. 00   this IG\n"
    "     |        |  |\n"
    "     |        |  +-- 0x00 0x80 at bytes 6-7: the byte-swap flag (little-endian)\n"
    "     |        +----- byte 5: IG mode (bits 0-1), timestamp-valid (2), earth\n"
    "     |                 model (3), CIGI minor version (4-7); 3.3 => 0x30\n"
    "     +-------------- byte 0: id (0x65), byte 1: size (0x18 = 24 bytes)\n"
    "  offset 8: the HOST's frame counter, echoed out of the IG Control it arrived in\n"
    "  offsets 12 / 16: timestamp and last-host-frame - left at 0 by the reference\n"
    "  Master, still 0 here, which is what keeps the SOF byte-identical\n"
    "  the IG never picks its own mode: byte 5 becomes 0x31 (Operate) as soon as the\n"
    "  HOST commands Operate; --ig-mode only seeds it before the first IG Control",
    size=12.5)

txt(cx, 1090,
    "HAT/HOT Request  ->  Response            id 102, 16 B: Height double @ 8\n"
    "  types 0 (HAT) / 1 (HOT)\n"
    "                 ->  Extended Response   id 103, 40 B: HAT @ 8, HOT @ 16\n"
    "  type 2 (extended)\n"
    "both carry a 3-byte reserved gap after the flags byte, which is what aligns\n"
    "the first double to an 8-byte boundary. CIGI 3.2 packed them straight after\n"
    "the flags: the HOST then read HOT out of the material-code bytes - zero -\n"
    "and flew the aircraft down to the terrain.",
    size=12.5)

txt(cx, 1265,
    "HAT = request altitude - HOT, which is what catches the two doubles packed in the wrong place. "
    "Valid is set only when a terrain source could answer.",
    size=13.5, font=FONT_NORMAL, width=940)
txt(cx, 1330,
    "Pacing uses timeBeginPeriod(1): Windows' default 15.6 ms (~64 Hz) timer granularity otherwise caps any rate "
    "above ~60 Hz at ~64 with an occasional burst - worth knowing when comparing rates with another tool.",
    size=13.5, font=FONT_NORMAL, width=940)
txt(cx, 1410,
    "In Host-Master.pcapng the counter advances ~0.52 per SOF (~53 Hz) while the SOF rate was ~100 Hz "
    "(an 8/12 ms alternation): the Master asked for about twice the replies the HOST had frames to give.",
    size=13.5, font=FONT_NORMAL, width=940)

# ==========================================================================
# 4 - HAT/HOT answering
# ==========================================================================

cx, cy = panel(1180, 760, 1080, 840,
               "4 - HAT/HOT answering: three sources, one anchor")

src1 = s.box(1240, 820, 960, None,
             "1 - Prepar3D's own terrain, through PdkTerrainProbe.dll\n"
             "ISurfaceQueryManagerV400, a free-standing query: the surface the sim will "
             "place the aircraft on, so runway flattening and any added scenery are "
             "included and nothing has to be pre-mapped",
             fill="green", size=14)
src2 = s.box(1240, 925, 960, None,
             "2 - the SRTM terrain database, offline .hgt tiles\n"
             "answered from latitude and longitude alone, so it stays correct for the "
             "whole flight, but it is a copy of the world: no flattening, no changed scenery",
             fill="yellow", size=14)
src3 = s.box(1240, 1030, 960, None,
             "3 - the local Prepar3D via FSUIPC 0x0020\n"
             "the ground under the sim's own aircraft: one point only, so it can stand "
             "in only for a request where that aircraft is",
             fill="orange", size=14)
s.arrow(src1, src2)
s.arrow(src2, src3)

txt(1240, 1150,
    "HOT   = the height from the first source that can answer\n"
    "HAT   = test-point altitude - HOT\n"
    "        entity-relative: the HOST aircraft's altitude (Entity Control) - the\n"
    "                         request's Z offset   (+Z points down; entity axes are NED)\n"
    "        geodetic:        the request carries its own altitude\n"
    "Valid = 0 - never a fabricated 0 m - when no source can cover the point: an\n"
    "        entity other than the ownship (0), an entity-relative request with no\n"
    "        anchor, or a point no source can reach\n"
    "updatePeriod = 0 -> answered once;  n -> re-answered every n-th frame until a\n"
    "        fresh request with the same HAT/HOT ID replaces it (answered at once)\n"
    "the geodetic FSUIPC fallback is accepted only within 500 m of the aircraft",
    size=12.5)

txt(1240, 1330,
    "ANCHOR   entity-relative answers are anchored to the HOST's own Entity Control\n"
    "         (entity 0) - the pose its flight model is commanding - not to the sim's\n"
    "         FSUIPC read-back. All-zero and off-globe poses are refused, not adopted.\n"
    "         The six probe offsets are identical wherever the aircraft is, so a stuck\n"
    "         anchor is invisible on the wire: on the rig it parked the aircraft on the\n"
    "         departure ground, a ~550 ft step at the destination.",
    size=12.5)

txt(1240, 1465,
    "--terrain-offset applies to the database only: a height from the sim needs no calibration, and applying it would correct it twice. "
    "Entity-relative answers are given at the HOST's aircraft position - the reference capture's six points sit within 10.4 m of it, far below any terrain source's resolution - "
    "and the sim is still asked for the exact point.",
    size=13.5, font=FONT_NORMAL, width=940)

# ==========================================================================
# 5 - the probe tick ladder
# ==========================================================================

cx, cy = panel(2300, 760, 1080, 840, "5 - The probe tick ladder")

txt(cx, cy,
    "Something has to call the free-standing surface query, and it has to keep\n"
    "running while the aircraft is being positioned. Three ticks, tried in order:",
    size=14, font=FONT_NORMAL, width=900)

t1 = s.box(2360, 878, 900, None,
           "1 - EVENTID_Frame, Prepar3D's per-frame event (IEventServiceV510/V600)\n"
           "once per rendered frame: the tick of a running sim. v5 keeps signalling it "
           "while paused; the rig's v6 does not - pausing stops the frame loop too.",
           fill="green", size=13.5)
t2 = s.box(2360, 975, 900, None,
           "2 - the object's ISimulation::Update\n"
           "used until a frame event has been seen, and again if one stops arriving; "
           "Update() defers while the frame event is live, so the two never both "
           "service the table",
           fill="blue", size=13.5)
t3 = s.box(2360, 1078, 900, None,
           "3 - a thread of the add-on's own - the last resort\n"
           "started at DLLStart, stopped and waited for at DLLStop; steps in only after "
           "both ticks above have been silent for kWatchGraceMs = 1000 ms. It calls "
           "QuerySurfaceElevation and nothing else of Prepar3D's.",
           fill="violet", size=13.5)
s.arrow(t1, t2)
s.arrow(t2, t3)

txt(2360, 1200,
    "A running sim never reaches the thread, so the one call made outside Prepar3D's thread can only happen while Prepar3D's own loop has stopped - "
    "which is what the rig does for the whole session. The object-tick lifecycle fault (a reposition stranding the tick) is gone with it.",
    size=13.5, font=FONT_NORMAL, width=900)

txt(2360, 1265,
    "diag word   (bit 31 kPresent must be set; a 0 means \"no diagnosis\")\n"
    "  0..7    user-object create callbacks      (saturate at 255)\n"
    "  8..15   removals of the tick's object\n"
    " 16..23   RegisterSimulation failures\n"
    " 24       a simulation is bound\n"
    " 25       the bound object was the user object then\n"
    " 26       a user-object create was ignored (already bound)\n"
    " 27       the newest pass came from the ADD-ON'S OWN THREAD\n"
    " 31       the word is filled\n"
    "tail        frameTicks  frame-event callbacks seen\n"
    "            simEvents   bit 0 paused now; sticky pause/unpause, sim\n"
    "                        stop/start, scenario load, vehicle change, frozen\n"
    "            simMode     GetMode() at the last object tick\n"
    "            frameFails  frame-event registration failures",
    size=12.5)

txt(2360, 1505,
    "ticks advancing + bit 27      normal for a paused sim, not a fault\n"
    "ticks frozen, frameTicks > 0  the whole add-on has stopped running\n"
    "ticks frozen, frameTicks = 0  no frame event was ever delivered\n"
    "alive clear                   no surface service at all",
    size=12.5)

# ==========================================================================
# 6 - shared-memory contract
# ==========================================================================

cx, cy = panel(60, 1650, 1080, 800, "6 - Shared-memory contract (src\\TerrainProbeIpc.h)")

txt(cx, cy,
    "mapping  Local\\CigiStarter.TerrainProbe.v1   (session-local: both sides run in\n"
    "         the logged-on user's session, so Global\\ would need elevation)\n"
    "magic 'PTBP' (0x50425450)    version 2    capacity 256 slots\n"
    "\n"
    "header (48 B)\n"
    "  magic - version - capacity - tickRateHz\n"
    "  alive       1 while a service and a tick exist\n"
    "  lastError   the last HRESULT the probe saw\n"
    "  probePid    Prepar3D's pid (what tasklist /m finds)\n"
    "  ticks       heartbeat: advances while the probe runs\n"
    "  reqSeq      bumped by the IG once it has written queries[]\n"
    "  rspSeq      seqlock over results[]: odd while rows are rewritten\n"
    "  queryFails  points Prepar3D has refused\n"
    "  diag        the diagnostic word (0 = an older add-on)\n"
    "queries[256] x 32 B   the IG writes:  latDeg lonDeg altMeters key enabled\n"
    "results[256] x 48 B   the probe writes: latDeg lonDeg hotMeters key status\n"
    "                      materialCode nrmAzimuthDeg nrmElevationDeg\n"
    "tail (16 B)           frameTicks simEvents simMode frameFails\n"
    "sizeof(Shared) = 48 + 256*(32+48) + 16 = 20528 B",
    size=12.5)

txt(cx, 2075,
    "1  the IG writes queries[i] (enabled, with its own key), then bumps reqSeq\n"
    "2  the probe walks the table on its next tick, asks the surface query for every\n"
    "   enabled point and writes results[], the batch guarded by rspSeq\n"
    "3  the IG accepts a row when status = Ok, the key matches and the echoed lat/lon\n"
    "   still match the point it asked about\n"
    "   -> a one-tick-old answer is safe; a request that has moved is refused and\n"
    "      falls back, never answered with somebody else's terrain",
    size=12.5)

txt(cx, 2205,
    "pack(8) plus a static_assert on every size and offset: a differently packed build would agree on slot 0 and misread every slot after it, "
    "so a layout change must bump kVersion. The table is metric - the wire is metres and only the probe converts to the PDK's radians and feet. "
    "256 slots because TerrainBaker fills the whole table to bake a tile in seconds.",
    size=13.5, font=FONT_NORMAL, width=940)
txt(cx, 2310,
    "The contract test overrides the mapping name on purpose: sharing it would make its first memset wipe a live add-on's header, "
    "which then reads as a probe that is loaded but has stopped answering.",
    size=13.5, font=FONT_NORMAL, width=940)

# ==========================================================================
# 7 - operator window
# ==========================================================================

cx, cy = panel(1180, 1650, 1080, 800, "7 - Operator window")

txt(cx, cy,
    "+----------------------------------------------------------------+\n"
    "| HOST 192.168.70.2:8001  ->  SOF destination                    |\n"
    "| IG   :8000   (this IG is 192.168.70.70)                        |\n"
    "|                                                                |\n"
    "| Stream    : RUNNING (paced to the HOST)                        |\n"
    "| SOF rate  : 150 Hz requested, 150/s measured                   |\n"
    "| SOF frames: 101                                                |\n"
    "| Received  : 0 datagrams (0/s), 0 entity updates                |\n"
    "| IG mode   : 0 (Standby)  <- commanded by the HOST              |\n"
    "| HOST frame: 0                                                  |\n"
    "|                                                                |\n"
    "| Relay     : 4 clients, 4 reachable, 1567 datagrams forwarded   |\n"
    "| Relay NIC : Killer E2600 Gigabit Ethernet Controller           |\n"
    "|                                                                |\n"
    "| TerrainDB : 2 tile(s), 1 in memory, 25 MB                      |\n"
    "| HAT/HOT   : answering from the sim's own terrain               |\n"
    "| Probe     : sim paused; the probe is answering from its own    |\n"
    "|             thread                                             |\n"
    "| Anchor    : HOST Entity Control -6.53973, 106.75963, 576 ft    |\n"
    "| Terrain   : 89 ft ground under the sim aircraft, 99 ft sim     |\n"
    "|             aircraft                                           |\n"
    "|                                                                |\n"
    "| [x] Answer HAT/HOT              SOF rate: [ 150 ] Hz [ Apply ] |\n"
    "| [x] Pace SOF to HOST replies (flow control)                    |\n"
    "|                                                                |\n"
    "| [ START ]  [ STOP ]                                   [ EXIT ] |\n"
    "+----------------------------------------------------------------+",
    size=12)

txt(cx, 2120,
    "Refreshes 4x/s. START / STOP start and stop the SOF stream (the button that would be a no-op is the disabled one). "
    "Answer HAT/HOT also starts and stops the FSUIPC poller: off means no FSUIPC calls at all. Pace SOF to HOST replies is the flow-control switch. "
    "SOF rate is 1..100000 Hz, applied to the transmit loop on the next frame with Apply or Enter, so the stream retunes while it runs; --fps only seeds it.",
    size=13, font=FONT_NORMAL, width=940)
txt(cx, 2215,
    "EXIT is the only way out: SC_CLOSE is deleted from the system menu (the X is drawn greyed out), WM_CLOSE is ignored, and with no console Ctrl+C cannot reach it. "
    "The stream is already running when the window appears and the window minimises itself; --no-auto-start is the single way back to a window that waits for the button.",
    size=13, font=FONT_NORMAL, width=940)
txt(cx, 2310,
    "Terrain lines: HAT/HOT names the source in force; TerrainDB the fallback behind it and its size; Probe the tick that is answering; "
    "Anchor the HOST's pose; Terrain the sim's own aircraft over FSUIPC - the ground under it and its altitude in feet, the way Prepar3D shows them.",
    size=13, font=FONT_NORMAL, width=940)

# ==========================================================================
# 8 - fan-out
# ==========================================================================

cx, cy = panel(2300, 1650, 1080, 800, "8 - Fan-out to the extra IG PCs")

txt(cx, cy,
    "# one address per line, optional port (default: the IG port, 8000)\n"
    "192.168.70.102:8000\n"
    "192.168.70.103\n"
    "192.168.70.104:8000   # left visual",
    size=12.5)
txt(cx, 1795,
    "client.txt sits beside the executable and is read once at startup. Blank lines, # comments and a Notepad BOM are tolerated; IPv4 only. "
    "Junk, a port of 0 or above 65535, duplicates and this PC's own addresses are skipped and counted. Capped at 32 clients; with no usable line "
    "nothing is relayed and the capture driver is never opened.",
    size=13, font=FONT_NORMAL, width=900)

txt(cx, 1895,
    "per datagram   only datagrams whose source is the HOST, copied before\n"
    "               anything is parsed, so a copy is exact\n"
    "\n"
    "one hand-built Ethernet + IPv4 + UDP frame per client, injected with\n"
    "pcap_sendpacket; both checksums are computed, the UDP one over the IP\n"
    "pseudo-header, so a copy is a complete valid datagram\n"
    "  Ethernet dst = the client's MAC   src = THIS adapter's own MAC\n"
    "               (spoofing that as well would teach every switch on the\n"
    "                way that the HOST lives on this port)\n"
    "  IP   src = the HOST, dst = the client\n"
    "  UDP  src = the HOST's port, dst = the client's port\n"
    "unicast: one frame per client, never broadcast\n"
    "\n"
    "client MACs are resolved with SendARP on a background thread and\n"
    "re-resolved every few seconds, so a client switched on or replaced later\n"
    "is picked up on its own; one that stops answering drops out and the\n"
    "window counts how many are reachable\n"
    "datagrams over 1472 B of payload are dropped and counted: a client's\n"
    "capture filter matches one datagram, not a fragment chain\n"
    "the adapter is found automatically - the one that owns the IG address,\n"
    "else whichever the route to the HOST leaves by - and the window names it\n"
    "relay counts: forwarded = reached at least one client, dropped = reached\n"
    "none; clients that have not answered yet are neither",
    size=12.5)

txt(cx, 2280,
    "Needs Npcap (wpcap.dll), loaded at run time rather than linked, so a PC without it still builds and runs as a plain IG; nothing here needs elevation. "
    "The SOF link to the HOST is unaffected when the adapter cannot be opened.",
    size=13, font=FONT_NORMAL, width=900)

# ==========================================================================
# 9 - SOF rate and flow control
# ==========================================================================

cx, cy = panel(60, 2500, 1080, 700, "9 - SOF rate and flow control")

txt(cx, cy,
    "The HOST owns the frame rate: one datagram per SOF, and its own frame counter advances about 53 times a second while the reference Master's SOF ran at "
    "~100 Hz (an 8/12 ms alternation) - the Master asked for roughly twice the replies the HOST had frames to give.",
    size=13.5, font=FONT_NORMAL, width=940)

txt(cx, 2610,
    "SOF rate default 150 Hz: every answer carries a fresh attitude (the position\n"
    "comes on every other one), so how often the IG asks is how smoothly the\n"
    "aircraft moves.\n"
    "\n"
    "FLOW CONTROL (default on, src\\SofPacer.h): a SOF is held back while the previous\n"
    "one is still unanswered, so the stream fits whatever the HOST on site can\n"
    "absorb, whatever number is typed in the rate box. A held-back frame is\n"
    "dropped, never replayed, so no mode of operation can turn the stream into a\n"
    "burst.\n"
    "  - a HOST that has never answered (startup, not yet woken) is transmitted\n"
    "    to at the full rate\n"
    "  - a SOF unanswered for 100 ms is followed by another anyway, so a HOST that\n"
    "    has dropped out still receives something to wake up on\n"
    "  - the limit is wall-clock, not a count of frames: a HOST answering less than\n"
    "    once per 100 ms sees at most ~10 requests a second whether the box says\n"
    "    150 or 6000. Counting frames would scale the retries with the rate -\n"
    "    exactly the runaway this exists to prevent",
    size=12.5)

txt(cx, 2905,
    "The SOF's only live field is the HOST's frame counter echoed at offset 8. An IG-side count would drift from the HOST by (rate - 53) frames a second, "
    "which is what broke it above ~60 Hz. The window's Stream line names the mode - paced to the HOST or free-running - and adds \"which is the limit\" when it is "
    "the HOST setting the pace; --no-flow-control transmits on the rate alone, as the reference Master did.",
    size=13.5, font=FONT_NORMAL, width=940)

# ==========================================================================
# 10 - terrain database
# ==========================================================================

cx, cy = panel(1180, 2500, 1080, 700, "10 - Terrain database (SRTM fallback)")

txt(cx, cy,
    "terrain\\ beside the executable, or --terrain-dir.  Raw SRTM .hgt, one\n"
    "1 x 1 degree cell per file, named by the corner it starts at:\n"
    "  S07E106.hgt   covers 7..6 S, 106..107 E   (Jakarta)\n"
    "  S08E112.hgt   covers 8..7 S, 112..113 E   (Surabaya)\n"
    "SRTMGL1 (1 arc-sec, ~30 m, 3601x3601) and SRTMGL3 (3 arc-sec, ~90 m,\n"
    "1201x1201) are both accepted; the sample count comes from the file size\n"
    "samples are signed 16-bit big-endian metres; row 0 is the northern edge\n"
    "and column 0 the western edge; -32768 is a void (interpolated around, or\n"
    "reported invalid when a point has nothing but voids)\n"
    "bilinear between the four surrounding samples; a tile is read in on first\n"
    "use and kept - the window's TerrainDB line shows indexed / in memory / MB",
    size=12.5)

txt(cx, 2775,
    "Two ways to get them.  tools\\get_terrain.ps1 downloads from the public AWS skadi mirror (-Points \"-6.13,106.66\" or -Tiles S07E106,S08E112), "
    "about 25 MB per 1 arc-sec tile, so fetch only the box the aircraft flies in: the generic world model, quick, but SRTM predates the scenery.  "
    "tools\\TerrainBaker overlays the simulator's own ground heights on that base - plan -> probe -> survey -> compose -> terrain-sim\\*.hgt - "
    "sim-exact at the fields, runway flattening and all (probe/survey need Prepar3D with the add-on loaded; plan and compose need neither).",
    size=13, font=FONT_NORMAL, width=940)

txt(cx, 2890,
    "Calibration: SRTM heights are orthometric (EGM96) and the sim's datum is its own; the two matched to ~2 m at Juanda in testing. "
    "--terrain-offset <m> shifts every database height if a site shows a constant bias - the database only, never FSUIPC and never a sim height.",
    size=13, font=FONT_NORMAL, width=940)

# ==========================================================================
# 11 - build, files, bench
# ==========================================================================

cx, cy = panel(2300, 2500, 1080, 700, "11 - Build, files and bench tooling")

txt(cx, cy,
    "BUILD\n"
    "  msbuild CigiStarter.sln /p:Configuration=Release /p:Platform=x64\n"
    "  -> x64\\Release\\CigiStarter.exe      (x64 only)\n"
    "  GUI subsystem (SubSystem=Windows + EntryPointSymbol=mainCRTStartup)\n"
    "  with a plain int main(): no console, so --help, --version and startup\n"
    "  errors use a message box\n"
    "  static CRT (/MT): a fresh IG PC needs no Visual C++ redistributable\n"
    "  no /GL: FSUIPCuser64.lib is an LTCG library built with MSVC v140\n"
    "  (WholeProgramOptimization=false; expect a benign LNK4098)\n"
    "  runs unelevated (asInvoker) - it starts from a logon path, where a\n"
    "  UAC prompt would block an unattended IG\n"
    "  vendored SDKs: external\\UIPC64_SDK_C_version2,\n"
    "  external\\npcap-sdk-1.16, external\\Prepar3D-SDK\\inc\\PDK\n"
    "  (/p:FsuipcSdkDir, /p:NpcapSdkDir, /p:P3DSdkDir)",
    size=12)

txt(cx, 2790,
    "FILES   src\\main.cpp  src\\SofPacer.h  src\\Gui.h/.cpp  src\\cigi.h/.cpp\n"
    "        src\\TerrainSource.h/.cpp   the FSUIPC poller thread\n"
    "        src\\TerrainDb.h/.cpp       the SRTM .hgt reader\n"
    "        src\\TerrainProbe.h/.cpp    this side of the shared-memory link\n"
    "        src\\TerrainProbeIpc.h      the pinned contract\n"
    "        src\\Relay.h/.cpp           client.txt, frames, Npcap, SendARP\n"
    "        client.txt / terrain\\      optional inputs beside the exe\n"
    "BENCH   tools\\test-hathot-emulator.ps1 -Local [-Stop] -> PASS/FAIL\n"
    "        tools\\capture-hat-hot.ps1 [-Summary]\n"
    "        external\\HostEmulator_Win32_v3.3.3\\Hemu3.exe (Boeing TSS)\n"
    "        tools\\PdkTerrainProbe\\test\\PdkTerrainProbeTest.exe\n"
    "        tools\\TerrainBaker   plan / probe / survey / compose\n"
    "        captures need Npcap + Wireshark's dumpcap and tshark",
    size=12)

txt(cx, 3010,
    "PdkTerrainProbe is deliberately not in the solution: it needs the sim's SDK, and a machine that only builds the IG should not be broken by that. "
    "The v5 and v6 DLLs are built separately and kept side by side in bin\\Release-v5 and bin\\Release-v6.",
    size=13, font=FONT_NORMAL, width=900)

# --------------------------------------------------------------------------
# write
# --------------------------------------------------------------------------

issues = s.report()
for issue in issues:
    print("ISSUE:", issue)
print(f"{len(issues)} issue(s)")

s.save(OUT + ".excalidraw")
s.svg(OUT + ".svg")
s.png(OUT + ".png", max_width=3000)
print("wrote", OUT + ".excalidraw / .svg / .png")
