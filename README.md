# CigiStarter

A minimal **CIGI 3.3 Image Generator (IG)** for a new IG PC.

The HOST in a CIGI flight-simulator link stays silent until the IG transmits
**Start-of-Frame (SOF)** messages. `CigiStarter` plays that IG role: it sends
SOF at a configurable rate to wake up the HOST and begin the flight-data
stream, and answers HAT/HOT requests with the real terrain height under the
aircraft, read from the local Prepar3D via FSUIPC — see
[HAT/HOT answering](#hathot-answering).

## What it does

1. Binds a UDP socket on the downstream port (`8000`) where the HOST sends
   control/flight data.
2. Sends **SOF** (24-byte Start-of-Frame) datagrams to the HOST upstream port
   (`8001`) at the configured frame rate — this is the "request" that starts
   the data flow. The rate defaults to **60 Hz** and the stream is paced to the
   HOST's replies by default — see
   [SOF rate and flow control](#sof-rate-and-flow-control).
3. Parses inbound CIGI messages (framed by `[id][size]`) and shows a live status
   block: datagrams/s, total datagrams, entity updates, the SOF rate (requested
   and achieved), SOF frames sent, the HOST frame counter and the terrain sample.
4. Answers HAT/HOT requests, riding the responses along on the SOF stream as the
   reference IG did. This is on by default (the window's check box switches it
   off, as does `--no-respond-hathot`), and the terrain height is read from the
   local Prepar3D via FSUIPC — see [HAT/HOT answering](#hathot-answering).
5. Runs as a small window with **START** / **STOP** / **EXIT**: it starts paused
   and stays silent until START is clicked — see
   [Operator window](#operator-window).
6. **Fans the HOST's downstream stream out to the other IG PCs** listed in
   `client.txt`, byte for byte, so several visual channels run off the one link
   the HOST already feeds — see [Fan-out](#fan-out-to-the-other-ig-pcs).

## Requirements

- Windows 10/11, x64.
- A **windowed** (GUI-subsystem) executable: there is no console window. A
  startup error or `--help` is reported in a message box instead of on stdout.
  The project links with `SubSystem=Windows` plus `EntryPointSymbol=mainCRTStartup`,
  which produces a GUI binary while still allowing a plain `int main(argc, argv)`.
- Windows system libraries only, plus the **FSUIPC SDK** while HAT/HOT answering
  is switched on (`Ws2_32.lib` for sockets, `Winmm.lib` for timing,
  `User32.lib` for the window, `Iphlpapi.lib` for the client MAC lookups, and
  `FSUIPCuser64.lib` for the terrain read). With answering switched off the
  program never touches FSUIPC.
- **Npcap** is needed only for the [fan-out](#fan-out-to-the-other-ig-pcs). It is
  loaded at run time rather than linked, so the program still builds and runs
  without it. The Npcap *SDK* (headers) is needed to build; it is shared with
  `CigiP3dBridge`:
  `..\CigiP3dBridge\external\npcap-sdk-1.16\`. Point the build elsewhere with
  `/p:NpcapSdkDir=<directory>`.
- The FSUIPC header/library are the ones already in this repository, shared with
  `CigiP3dBridge`:
  `..\CigiP3dBridge\external\UIPC64_SDK_C_version2\`. If your checkout lays the
  two projects out differently, point the build at another copy with
  `/p:FsuipcSdkDir=<directory>`.
- `FSUIPCuser64.lib` was compiled with **MSVC v140** and is an LTCG library, so
  this project must **not** build with `/GL` — `WholeProgramOptimization` is
  therefore `false` for Release. Building with `/GL` fails at link with
  `C1047` / `LNK1257` ("created by a different version of the compiler").
- Built with the static CRT (`/MT`, `/MTd`), so the executable runs on a fresh
  IG PC **without installing the Visual C++ redistributable**. (The FSUIPC SDK
  does not use the CRT across its boundary, so the mixed CRT is harmless —
  expect a benign `LNK4098` about `MSVCRT`.)
- The embedded manifest requests **administrator** privileges (the app launches
  elevated for network/firewall access in the simulator environment).

## Building

Using MSBuild from a Visual Studio / Build Tools developer environment:

```
msbuild CigiStarter.sln /p:Configuration=Release /p:Platform=x64
```

Or open `CigiStarter.sln` in Visual Studio and build **Release | x64**.
The output is `x64\Release\CigiStarter.exe`.

## Usage

The link addresses and ports are **fixed by the deployment** and hardcoded:

| Role | Address          | Port |
|------|------------------|------|
| HOST | `192.168.70.2`   | 8001 (upstream SOF destination) |
| IG   | `192.168.70.70`  | 8000 (downstream RX bind)       |

```
CigiStarter.exe [options]

  --fps <rate>        initial SOF transmit rate in Hz   (default 60)
                      Keep this at or below the rate the HOST itself
                      manages — see SOF rate and flow control below.
  --ig-mode <0..3>    IG mode reported until the HOST sends its first
                      IG Control (0=Standby 1=Operate)  (default 0)
  --respond-hathot    answer HAT/HOT requests using the terrain height read
                      from the local Prepar3D via FSUIPC       (default on)
  --no-respond-hathot do not answer HAT/HOT requests, and leave FSUIPC
                      untouched
  --flow-control      hold a SOF back while the HOST has not answered the
                      previous one                            (default on)
  --no-flow-control   transmit on the rate alone, without waiting for the HOST
  --help              show this help (in a message box)
```

The options only set the *starting* state. The SOF rate, HAT/HOT answering and
flow control can all be changed from the window while the program runs — see
[Operator window](#operator-window).

One input file matters too: **`client.txt`**, next to the executable, lists the
other IG PCs to copy the downstream stream to. It is optional and is read once at
startup — see [Fan-out](#fan-out-to-the-other-ig-pcs).

Examples:

```
CigiStarter.exe                        # 60 Hz, paced to the HOST
CigiStarter.exe --no-flow-control --fps 100
```

## SOF rate and flow control

The HOST owns the frame rate: it answers each SOF with one datagram of flight
data, and it cannot produce frames faster than its own simulation runs. The
reference capture (`Host-Master.pcapng`) shows what that means in practice — the
HOST emits a datagram every ~10.2 ms (~98 Hz) while its own frame counter only
advances ~53 times a second, i.e. the Master IG was asking for roughly twice as
many replies as the HOST had frames to give.

That is why:

- the SOF rate defaults to **60 Hz** rather than the ~100 Hz of the reference
  Master, and
- **flow control is on by default**: the transmit loop holds a SOF back while the
  previous one is still unanswered (`SofPacer.h`), so the stream fits whatever the
  HOST on site can absorb, whatever number is typed in the rate box.

With flow control on the configured rate is an upper bound, not a promise: when
the HOST keeps up the stream runs at the rate in the box, and when it cannot the
SOF stream follows the HOST down instead of piling requests up on it. A frame
that is held back is dropped, never replayed, so no mode of operation can turn
the stream into a burst. Two cases keep the link usable on their own: a HOST that
has never answered — at startup, before it has been woken — is transmitted to at
the full rate, and a SOF that has gone unanswered for 100 ms is followed by
another one anyway, so a HOST that has dropped out still receives something to
wake up on.

That limit is a wall-clock one rather than a number of frames, and that is what
stops a mistyped rate from becoming a load: a HOST answering more slowly than
once per 100 ms sees at most ~10 requests a second, whether the rate box says 60
or 6000. Counting frames instead would make the retries scale with the rate,
i.e. exactly the runaway this is here to prevent.

`--no-flow-control`, or clearing the window's check box, transmits on the rate
alone — what the original Master IG did, and the mode to use when measuring what
a HOST really tolerates:

```
CigiStarter.exe --no-flow-control --fps 100
```

The window's `Stream` line always names the mode in force, and says when it is
the HOST that is setting the pace: `paced to the HOST, which is the limit`.

**Why the rate matters at all.** The SOF has exactly one live field, the *HOST's*
frame counter echoed back (see [Protocol notes](#protocol-notes)). An IG that
counted its own transmitted SOFs in that slot instead would drift away from the
HOST a little further every second — at 100 Hz against a 53 Hz HOST, about 47
frames a second — which is what used to break the HOST when the rate was set
above 60 Hz. Echoing the HOST's counter keeps the outgoing SOF byte-identical to
the packet the reference Master sent.

## HAT/HOT answering

The HOST drives its aircraft altitude to the **Height Of Terrain (HOT)** that
the IG reports, so an IG that answers `HOT = 0` flies the aircraft into the
ground. CigiStarter therefore reports the *real* terrain **by default** — turn
it off in the window, or start with `--no-respond-hathot`, if you must:

| Request                | Response                                      |
|------------------------|-----------------------------------------------|
| Type 0 (HAT) / 1 (HOT) | HAT/HOT Response (id 102, 16 bytes)           |
| Type 2 (Extended)      | HAT/HOT Extended Response (id 103, 40 bytes)  |

- `HOT` = FSUIPC `0x0020`, the ground altitude under the player aircraft (m).
- `HAT` = test-point altitude − `HOT`. Entity-relative requests use the
  aircraft's FSUIPC altitude minus the request's Z offset (CIGI entity axes are
  local NED, so **+Z points down**); geodetic requests carry the altitude
  themselves.
- `Valid` is set to 0 — never a fabricated 0 m — when no answer can be produced:
  no FSUIPC link, a request for an entity other than the ownship (entity 0), or
  a geodetic point more than 500 m from the aircraft. FSUIPC only exposes the
  terrain under the *player* aircraft, so that is the one test point this
  program can answer for.
- `updatePeriod = 0` (one-shot) requests are answered once; a non-zero period is
  honoured by re-answering every *n*-th frame.

The FSUIPC link is polled by a background thread that owns every FSUIPC call, so
the SOF pacing is never delayed by an FSUIPC round trip. The poller runs only
while answering is switched on — the window's check box starts and stops it, and
while it is off this program makes no FSUIPC calls at all. If Prepar3D is not
running, CigiStarter still starts: it keeps retrying, answers `Valid = 0` until
the sim appears, and shows the current terrain height in the window's status
block.

## Operator window

CigiStarter is a windowed program, so it is driven with the mouse rather than
from a console. The window is a fixed-size plain Win32 dialog:

```
+--------------------------------------------------------------------------+
|  HOST 192.168.70.2:8001   ->  SOF destination                            |
|  IG   :8000   (this IG is 192.168.70.70)                                 |
|                                                                          |
|  Stream    : RUNNING (paced to the HOST)                                 |
|  SOF rate  : 60 Hz requested, 60/s measured                              |
|  SOF frames: 101                                                         |
|  Received  : 0 datagrams (0/s), 0 entity updates                         |
|  IG mode   : 0 (Standby)  <- commanded by the HOST                       |
|  HOST frame: 0                                                           |
|                                                                          |
|  Relay     : 4 clients, 4 reachable, 1567 datagrams forwarded            |
|  Relay NIC : Killer E2600 Gigabit Ethernet Controller                    |
|                                                                          |
|  HAT/HOT   : answering from FSUIPC                                       |
|  Terrain   : 27.0 m under the aircraft, 30.1 m aircraft                  |
|                                                                          |
|  [x] Answer HAT/HOT (terrain from FSUIPC)  SOF rate: [ 60 ] Hz [ Apply ]  |
|  [x] Pace SOF to HOST replies (flow control)                             |
|                                                                          |
|  [ START ]  [ STOP ]                                              [ EXIT ] |
+--------------------------------------------------------------------------+
```

- The status block refreshes four times a second. The datagram and SOF rates are
  sampled once a second from the cumulative counters; the SOF line shows the
  rate that is *requested* next to the rate the transmit loop actually achieved,
  and the `Stream` line names the pacing mode — `paced to the HOST` or
  `free-running` — adding `which is the limit` when it is the HOST that is
  setting the pace rather than the rate box.
- **START** / **STOP** start and stop the SOF stream. The program boots
  *paused*, so nothing is transmitted to the HOST until START is clicked. The
  button that would be a no-op is disabled.
- **Answer HAT/HOT** switches HAT/HOT answering on and off, and with it the
  FSUIPC poller — switching it off means this program makes no FSUIPC calls at
  all, switching it back on reconnects. It starts **on**, unless
  `--no-respond-hathot` was given.
- **Pace SOF to HOST replies** is the [flow
  control](#sof-rate-and-flow-control) switch. It starts **on**, unless
  `--no-flow-control` was given. Switch it off to transmit on the rate alone —
  for instance to measure what the HOST really tolerates.
- **SOF rate** is a whole number of Hz, from 1 to 100000; it starts at **60**.
  It is applied to the transmit loop on the next frame, so the stream can be
  retuned *while it runs* — no stop/start needed. Commit it with **Apply** or by
  pressing **Enter**. A value that is not a whole number in range is refused with
  a warning and the box snaps back to the rate actually in force; when the box
  merely loses focus a valid value is applied quietly and an unfinished one is
  left alone, so clicking EXIT never nags. `--fps` only seeds it. With
  [flow control](#sof-rate-and-flow-control) on, this is the ceiling: the stream
  follows the HOST down when the HOST cannot keep up.
- **EXIT** is the **only** way out. The title-bar **X** and **Alt+F4** are
  deliberately swallowed — `SC_CLOSE` is deleted from the window's system menu
  (Windows then draws the X greyed out and non-clickable) and `WM_CLOSE` is
  ignored — so the IG cannot be taken off the link by a stray click. This
  matches the `CigiP3dBridge` GUI, which follows the same rule. The window
  disappears at once; the process itself is gone a moment later at the latest,
  since shutting down waits only for the [fan-out](#fan-out-to-the-other-ig-pcs)
  thread to finish the client-address lookup it is already in the middle of.
- Since the process has no console, `Ctrl+C` cannot reach it either.
- Terrain lines appear while answering is on: `answering from FSUIPC` with the
  ground/aircraft heights, or a warning that there is no FSUIPC link to
  Prepar3D. With answering off they read `not answered (switched off)`.
- `Relay` and `Relay NIC` report the [fan-out](#fan-out-to-the-other-ig-pcs):
  how many clients are listed, how many of them answer, how many datagrams have
  been copied, and which adapter the copies leave by. There is no button for it
  — it is on whenever `client.txt` says so.

## Fan-out to the other IG PCs

CigiStarter can copy the HOST's downstream stream to extra IG PCs, so several
visual channels can be driven from the one link the HOST already feeds. The list
of extra PCs is a plain text file, **`client.txt`, next to the executable**. It
is read once at startup — restart the program to pick up an edit:

```
# one address per line, with an optional port (default: the IG port, 8000)
192.168.70.102:8000
192.168.70.103
192.168.70.104:8000   # left visual
```

- Case by case: blank lines, `#` comments (whole-line or trailing) and a Notepad
  byte-order mark are all tolerated; IPv4 addresses only.
- Unusable lines — junk, a port of 0 or above 65535, a duplicate, this PC's own
  addresses — are skipped and **counted**, and the window says how many were
  ignored. The list is capped at 32 clients.
- The file is optional. Without it (or with nothing usable in it) nothing is
  relayed, the window says which of those it is, and the capture driver is never
  opened.

Every datagram the HOST sends is copied **exactly as it arrived**, on the same
UDP port, to each listed client — one Ethernet frame per client per datagram.
The copies carry the **HOST's address and port as their source**
(`192.168.70.2:8001`), so a client's bridge needs no reconfiguration at all: it
sees the same stream it would have seen if it were the IG on the link. Nothing
is ever sent back up the link, and clients never talk to CigiStarter.

Producing those copies needs **Npcap** (`wpcap.dll`), which is loaded at run
time — deliberately *not* linked — so CigiStarter still builds and runs as a
plain IG on a PC without it; the window then reports the relay as off, and why.
Npcap has to be installed on the IG PC for the fan-out to work.

How it works, and why:

- **The adapter is found automatically**: the one that owns this IG's address
  (`192.168.70.70`) — or, on a machine that does not have it, such as a test
  bench, whichever adapter the route to the HOST leaves by. No flag; the window
  names the adapter it picked.
- **Each copy is a hand-built Ethernet + IPv4 + UDP frame**, injected with
  `pcap_sendpacket`. Ethernet destination = the client's MAC; Ethernet source =
  this adapter's *own* MAC (spoofing that as well would teach every switch on
  the way that the HOST lives on this port); IP source = the HOST; IP
  destination = the client; UDP source = the HOST's port; UDP destination = the
  client's. Both the IP and the UDP checksum are computed, over the IP
  pseudo-header as the UDP one must be, so a copy is a complete, valid datagram
  rather than something only a capture driver would show.
- **Unicast, one frame per client** — not broadcast — so only the PCs named in
  `client.txt` see the copies.
- **Client MACs are resolved with `SendARP` on a background thread**, re-resolved
  every few seconds, so a client that is switched on or replaced later is picked
  up on its own. One that stops answering is taken out of the fan-out until it
  answers again; the window counts how many are reachable.
- **Datagrams that would have to be fragmented are dropped and counted**
  (anything over 1472 bytes of payload). A client's capture filter matches one
  datagram, not a fragment chain, so sending the first fragment of a truncated
  CIGI stream would be worse than not sending it.
- **The receive path is untouched**: only datagrams whose source address is the
  HOST's are copied at all, and the copy is taken before anything is parsed, so
  clients get precisely what the HOST sent.

The `Relay` line in the window is the whole state, and it never needs a button:

```
  Relay     : 5 clients, 4 reachable, 1250431 datagrams forwarded, 2 dropped, 1 client.txt line ignored
  Relay NIC : Killer E2600 Gigabit Ethernet Controller
```

`forwarded` counts datagrams that reached at least one client; `dropped` counts
datagrams that reached none (all of them too big, or every send failing while a
client was reachable). Clients that have not answered yet are neither.

## Network layout

| Direction   | From                  | To                    | Port | Payload                         |
|-------------|-----------------------|-----------------------|------|---------------------------------|
| Downstream  | HOST `192.168.70.2`   | IG `192.168.70.70`    | 8000 | IGCtrl, EntityCtrl, HAT/HOT req |
| Upstream    | IG `192.168.70.70`    | HOST `192.168.70.2`   | 8001 | SOF (+ HAT/HOT responses)       |
| Fan-out     | HOST `192.168.70.2`   | each `client.txt` PC  | 8000 | one copy of each downstream datagram |

The fan-out row is the only traffic this program puts on the link labelled with
somebody else's address: the copies claim to come from the HOST on purpose, so
the clients need no reconfiguration. See
[Fan-out](#fan-out-to-the-other-ig-pcs).

## Protocol notes

- **CIGI version 3.3**, little-endian on the wire.
- A message is framed as `[message-id(1)][message-size(1)][payload...]` where
  `size` is the total byte length including the two header bytes.
- The SOF is `65 18 03 00 00 <b5> 00 80` + a 4-byte field + 12 zero bytes, where
  byte 5 packs the CIGI minor version (bits 4-7; 3.3 => `0x30`) together with the
  **IG mode** (bits 0-1). The `0x00 0x80` at offsets 6-7 is the byte-swap flag
  (little-endian).
- That 4-byte field at offset 8 is the **HOST's frame counter**, echoed out of
  the IG Control it arrived in — *not* a count of the SOFs this program has sent.
  It is the only SOF field that ever changes, and in `Host-Master.pcapng` it
  advances ~0.52 per SOF, i.e. at the HOST's own frame rate (~53 Hz) rather than
  at the SOF rate (~100 Hz, an 8/12 ms alternation). Offsets 12 and 16
  (timestamp, last-host-frame) were left at zero by the reference Master and are
  still zero here, which makes the outgoing SOF byte-identical to the captured
  one — see [SOF rate and flow control](#sof-rate-and-flow-control) for why the
  field matters.
- The IG never picks its own mode: the HOST commands it with IG Control and the
  SOF reports it back. CigiStarter follows that, so byte 5 becomes `0x31`
  (Operate) as soon as the HOST asks for Operate — which the reference HOST in
  `hlm.bin` does in every one of its 1160 datagrams. `--ig-mode` only sets the
  mode reported *before* the first IG Control arrives.
- SOF pacing uses the 1 ms system timer resolution (`timeBeginPeriod(1)`).
  Windows' default granularity is 15.6 ms (~64 Hz), so without it any rate above
  ~60 Hz is really paced at ~64 Hz with an occasional burst. That is worth
  knowing when comparing rates against another tool: a replay loop built on
  `Sleep(8)`/`Sleep(12)` asks for 100 Hz and gets ~64. CigiStarter raises the
  timer so that the rate it is asked for is the rate it sends, and
  [flow control](#sof-rate-and-flow-control) then keeps that stream inside what
  the HOST can absorb.
- HAT/HOT responses use the CIGI 3.3 layouts: the 16-byte Response (102) carries
  a single `Height` double at offset 8, and the 40-byte Extended Response (103)
  carries `HAT` at offset 8 and `HOT` at offset 16. **Both** have a 3-byte
  reserved gap between the flags byte and the first double, which is what aligns
  that double to an 8-byte boundary. (CIGI 3.2 packed them immediately after the
  flags; getting this wrong makes the HOST read `HOT` out of the material-code
  bytes — i.e. zero — and fly the aircraft down to the terrain.)

## Project layout

| File               | Purpose                                        |
|--------------------|------------------------------------------------|
| `main.cpp`         | entry point, sockets, SOF + receive threads, window callbacks |
| `SofPacer.h`       | flow-control rule: whether a due SOF may be sent |
| `Gui.h`            | operator-window API: status snapshot + start/stop/HAT-HOT/flow-control/rate/quit hooks |
| `Gui.cpp`          | the plain Win32 window (status block, START / STOP / EXIT, HAT/HOT + flow-control check boxes, SOF rate box) |
| `cigi.h`           | CIGI 3.3 message IDs, sizes, pack/parse API    |
| `cigi.cpp`         | explicit little-endian message packing         |
| `TerrainSource.h`  | FSUIPC-backed terrain/aircraft snapshot API    |
| `TerrainSource.cpp`| the FSUIPC poller thread (owns all FSUIPC use) |
| `Relay.h`          | fan-out API: start/stop, per-datagram copy, status snapshot |
| `Relay.cpp`        | the fan-out itself: `client.txt`, frame building, Npcap injection, client MAC lookup |
| `client.txt`       | optional, next to the executable: the extra IG PCs to copy the stream to |
| `app.manifest`     | UAC `requireAdministrator` manifest            |
| `agents.md`        | description of the HOST / IG / CigiStarter agents |
| `CigiStarter.sln`  | Visual Studio solution (x64 only)              |
| `CigiStarter.vcxproj` | VS project (x64 only, static CRT, ws2_32, FSUIPC) |
| `CigiStarter.vcxproj.filters` | project file organization            |
| `.gitignore`       | ignores build output and VS artifacts          |

See `agents.md` for a description of each agent and the message flow.
