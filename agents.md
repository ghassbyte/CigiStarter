# Agents

The **agents** (nodes) in the CIGI flight-simulator network and how they
interact. The reference behaviour was reverse-engineered from
`Host-Master.pcapng`.

For how CigiStarter is built, configured and operated, see `README.md`; this
document is the network view.

## Topology

```
                     Downstream (control / flight data)   port 8000
   ┌──────────────┐ ───────────────────────────────────────────►  ┌────────────────────┐
   │              │                                               │                    │
   │     HOST     │                                               │  IG (image gen.)   │
   │ 192.168.70.2 │                                               │  192.168.70.70     │
   │              │ ◄───────────────────────────────────────────  │                    │
   └──────────────┘   Upstream (SOF + HAT/HOT responses) port 8001└────────────────────┘
                                                                          │
   ┌──────────────────────────────────────────────────────────────────────┘
   │  the same downstream datagrams, copied by the IG for the extra IG PCs
   ▼  listed in client.txt (they are passive: nothing ever comes back)
 ┌─────────────────────────────────┐
 │ 192.168.70.102 ... .106  :8000  │
 └─────────────────────────────────┘
```

The copies keep the HOST's address and port as their source, so each extra IG PC
runs the ordinary bridge with no reconfiguration and sees exactly the stream the
HOST sent. `--no-relay-spoof` sends this IG's own address and port instead, for a
client-side driver that follows the IG rather than the HOST.

## Agents

### HOST — `192.168.70.2`
The simulation host. It runs the aircraft/flight model and owns the
authoritative entity state. It **does not send anything until the IG first
sends a Start-of-Frame**, so it is effectively a request/response slave.

Per frame, in response to a SOF, it transmits a single UDP datagram to the IG
(`192.168.70.70:8000`) containing:

| Message                | Count | Purpose                                   |
|------------------------|-------|-------------------------------------------|
| IG Control             | 1     | IG mode (Operate) + frame counter         |
| Entity Control         | 3     | entity pose (roll/pitch/yaw, lat/lon/alt) |
| HAT/HOT Request        | 1     | terrain height request                    |
| View Control           | 1     | view definition                           |
| Articulated Part Ctrl  | 5     | articulated parts                         |
| Rate Control           | 8     | motion rates                              |
| Atmosphere Control     | 1     | first frames only (warm-up)               |

On a bench there is no flight model: a **CIGI Host Emulator**
(`external\HostEmulator_Win32_v3.3.3\`) runs instead, on the same PC as
CigiStarter, to send the requests and watch the answers. It stands in for the
HOST only as far as this program is concerned — it produces none of the flight
data above — and `README.md`, [Bench testing](README.md#bench-testing), has its
setup and the automated tests built on it. It is driven by its `.def` tables and
a script, and reads no `.bin`; `tools\BinToScenario` rewrites a recorded stream
as the script that replays the same sortie on it, so the flight data above can be
watched against CigiStarter without a `.bin` player in the loop.

### Master IG — `192.168.70.70`
The original Image Generator that CigiStarter replaces. In the capture it is the
frame-timing master: it emits a **SOF** at ~100 Hz (an 8/12 ms alternation) and,
after a ~100-frame warm-up, appends **HAT/HOT Extended Response** messages. Its
SOF frame-number field advances only ~53 times a second — it carries the HOST's
own counter, not a count of SOFs.

### CigiStarter (this program) — the new IG PC
A drop-in replacement for the Master IG. It plays the IG side of the link:

- **Wakes the HOST and answers it** — binds `0.0.0.0:8000` for the flight data,
  sends SOF to `192.168.70.2:8001` at a configurable rate, and rides its HAT/HOT
  responses along on the SOF stream. Those are the hardcoded defaults in
  `main.cpp`; `--host-ip` / `--ig-ip` and the matching ports repoint the link for
  a bench test, and the deployment passes none of them.
- **Answers HAT/HOT** from the terrain the aircraft will actually fly onto —
  Prepar3D's own engine, through the `PdkTerrainProbe` add-on — falling back to
  an offline SRTM database and then to FSUIPC. On by default. Entity-relative
  requests are anchored to the HOST's own Entity Control pose (entity 0), not to
  the FSUIPC read-back; FSUIPC is only the fallback before one has arrived. The
  rig holds Prepar3D paused throughout, and on v6 a pause stops Prepar3D's own
  ticks, so there the add-on answers from a thread of its own.
- **Fans the downstream stream out** to the extra IG PCs in `client.txt`, byte
  for byte, source-labelled as the HOST (`--no-relay-spoof` labels them as this
  IG instead). Optional, Npcap-backed, passive clients only.
- **Puts the HOST's world on screen** — every Entity Control message is decoded
  (attitude as well as position) and published into shared memory
  (`SimLinkIpc.h`, `SimLink.h`); the `PdkSimLink` add-on inside the local
  Prepar3D creates a sim object per entity and moves it to the commanded pose on
  the sim's own frame tick. The object is `CigiEntity`, physics-free, so the
  commanded pose is the pose it holds — no flight model between the writes, no
  ground clamp, and no pause or slew. One-way, and the IG never touches the sim;
  the add-on
  does publish the pose it finds each object at (the read-back), which is the
  sim's own state rather than an answer to anything.
  `--no-sim-link` turns it off. This is the rendering path in this IG PC; the
  vendor program that used to drive the sim's own aircraft over FSUIPC is what
  it replaces.
- **Starts on its own** — streams from launch, minimises its window, and leaves
  EXIT as the only way off the link. `--no-auto-start` restores the paused
  window.

Three design points are where it differs from a naive IG, and all of them matter
to the HOST:

- **Flow control (default on)** holds a SOF back while the HOST has not answered
  the previous one, so the stream fits the frame rate the HOST can actually
  produce instead of piling requests up on it. The rate box is a ceiling, not a
  promise.
- **The SOF echoes the HOST's frame counter** (offset 8), not an IG-side count of
  SOFs. That is what the reference Master did; an IG-side count would drift away
  from the HOST by (rate − 53) frames every second, which is what used to break
  it above ~60 Hz.
- **Entity-relative HAT/HOT answers are anchored to the HOST's Entity Control**
  (entity 0), the pose the HOST's flight model is flying, rather than to the
  sim's FSUIPC read-back. The requests are the same six offsets wherever the
  aircraft is, so an anchor left over another airfield cannot be seen on the
  wire — it just answers the wrong terrain, and the HOST flies the aircraft to
  meet it. That is the rig's "repositions, then parks at the departure ground"
  failure: the sim link had frozen at the departure field while the HOST flew
  the aircraft to the destination.

`README.md` has the mechanism behind all of it: the operator window, every
option, the terrain sources, the fan-out and the wire layout.

## Message flow

```
  IG (CigiStarter)                          HOST
        │                                     │
        │──── SOF (frame N) ────────────────► │   ← the "request" that wakes the host
        │                                     │
        │◄─── IGCtrl + EntityCtrl + ... ──────│   ← flight data for frame N
        │                                     │
        │──── SOF (frame N+1) [+ HAT/HOT] ──► │
        │◄─── IGCtrl + EntityCtrl + ... ──────│
        │                ...                  │
```

The loop is self-limiting: flow control stops the IG asking for a frame the HOST
has not made. With HAT/HOT answering on, the IG asks the sim's own terrain engine
for each request's height before it replies; with `client.txt` populated, every
downstream datagram also leaves the IG again — unaltered — towards each client,
looking to them exactly like the HOST's own.

## Key facts

| Property          | Value                        |
|-------------------|------------------------------|
| Protocol          | CIGI 3.3, UDP, little-endian |
| HOST address      | `192.168.70.2` default (`--host-ip`) |
| IG address        | `192.168.70.70` default (`--ig-ip`)  |
| Downstream port   | 8000 (HOST → IG)             |
| Upstream port     | 8001 (IG → HOST)             |
| SOF message id    | 101 (`0x65`)                 |
| IG mode           | commanded by the HOST via IG Control; the SOF echoes it back |
| HAT/HOT resp ids  | 102 (`0x66`) / 103 (`0x67`)  |
| SOF live field    | the HOST's frame counter, echoed at offset 8; offsets 12/16 stay 0 |
| Sim at the IG PC  | Prepar3D Professional (v6) — its CIGI feature is a Professional Plus extra, so the IG's 8000/8001 are not contested there |
| Sim driver        | the **deployed** rig still runs a 3rd-party vendor program that holds Prepar3D **paused**, with **slew off**, positioning the sim's own aircraft over FSUIPC; the sim's own aircraft altitude can stay at Prepar3D's startup value, so it is not the HOST's — the anchor is the HOST's Entity Control. The entity writer above is its replacement, and needs the sim **running**, not paused |
| Terrain source    | P3D's own terrain via the `PdkTerrainProbe` add-on; then an SRTM `.hgt` tileset; then FSUIPC `0x0020` |
| Entity anchor     | entity-relative HAT/HOT is anchored to the HOST's Entity Control (entity 0), a pose at offset 24/32/40 (lat/lon/alt, CIGI 3.3 layout); FSUIPC only until one has arrived |
| Entity writer     | the HOST's entities, rendered by the `tools\PdkSimLink` PDK add-on inside the local Prepar3D: it creates a sim object per entity and moves it to the pose CigiStarter published. The object is `CigiEntity`, a physics-free `ISimObject` the add-on registers (`tools\PdkSimLink\CigiEntity.h`); a stock container title still creates the sim's own object for comparison. On by default; `--no-sim-link` leaves the sim alone |
| Sim link contract | `SimLinkIpc.h`, a private mapping `Local\CigiStarter.SimLink.v1` with its own `kVersion` (2): 64 entity rows (degrees + metres, per-row seqlock), 64 read-back rows (`held[]`) plus the drift words the add-on fills with the pose each object is actually at, and the add-on's status words. One-way — nothing goes back to the HOST — and deliberately separate from `TerrainProbeIpc.h`, so writer and terrain work cannot break each other |
| Entity hold | Measured on the bench. Against the AS332, a stock object: the sim holds a written pose to 2 cm and 0.1 deg, moving or still, but **clamps the object to its own ground** — a commanded altitude below the sim's terrain leaves it standing on the surface. Against `CigiEntity`, the physics-free object: held **exactly** (0.00 m / 0.00 deg), and a command below the terrain is where the command puts it, with no pause or slew |
| Entity writer cut | pose only (position and attitude); time of day, weather and effects are next. The physics-free object ships with **no model**, so nothing is drawn until its container is given one — a content decision per entity type, not part of the link |
| Entity state      | the HOST marks the entities it drives **Active (1)**, the CIGI Common Library's numbering (`Standby=0, Active=1, Remove/Destroyed=2`): entities 0 and 3 carry state 1 and a moving pose in every frame of every capture, and entity 5, the all-zero placeholder, carries Remove (2). Read the field the other way round and the writer renders nothing at all — every pose still arrives perfectly, which is why it was caught against the captures rather than on the rig |
| Terrain probe     | `tools\PdkTerrainProbe`, a PDK DLL loaded by Prepar3D; free-standing query, no SimObject; optional, and loaded on the bench's Professional Plus v5 and the rig's Professional v6. The answer carries the sim's own surface classification (hard/soft/water, and whether it is an object's *platform*); the answering policy needs no more than the full surface query already gives it — its height carries a platform's deck where the elevation-only call returned sea level — and `TerrainBaker probe` and the window's `Terrain` line show the classification |
| Terrain database  | `.hgt` tiles in `terrain\` by default or `--terrain-dir`; from `tools\get_terrain.ps1` or a `tools\TerrainBaker` bake |
| Extra IG PCs      | `client.txt` beside the executable, `address[:port]`, read once at startup |
| Fan-out           | one Npcap-injected copy per client, source address/port spoofed as the HOST by default (`--no-relay-spoof` sends this IG's) |
| Fan-out adapter   | found automatically: this IG's adapter, else the one the route to the HOST leaves by |
| Bench HOST        | the CIGI Host Emulator in `external\`, on the IG PC, in place of the flight model; `tools\test-hathot-emulator.ps1` drives it |
| Recorded sortie   | a `.bin` capture is a recording, and the emulator reads none; `tools\BinToScenario` rewrites one as the emulator script that replays it — entities, time of day and the warm-up environment; `tools\bin-to-scenario.ps1` builds and runs it |
| Contract tests    | `tools\test-all.ps1` builds and runs every SDK-free harness — the entity writer's, the terrain probe's and the capture converter's — with no Prepar3D and no SDK; `tools\test-simlink.ps1` is the writer's alone |
| Local e2e test    | `tools\test-simlink-e2e.ps1` is the only check that reaches inside the sim: it replays a recorded `.bin` HOST stream into a bench CigiStarter and reads PdkSimLink's status words and entity rows back out of the running Prepar3D, so the objects it creates and the poses it applies are proved on this PC. It fails up front on a stale `PdkSimLink.dll` (the add-on is loaded once, at sim startup) or a `title=` the installation does not have |
| Elevation         | none requested (`asInvoker`); the fan-out needs Npcap, not elevation |
| Reference SOF rate | ~100 Hz (~10 ms, an 8/12 ms alternation) |
| HOST frame rate   | ~53 Hz — its IG Control counter advances 0.54 per datagram, i.e. the HOST is asked for about twice the frames it has |
| SOF rate here     | 150 Hz default (1..100000 Hz), paced to the HOST unless flow control is off |
| Operator UI       | plain Win32 window: START / STOP / EXIT, a HAT/HOT check box, a flow-control check box, a SOF rate box |
| Start-up          | streams from launch, window minimised; `--no-auto-start` starts paused |
| Exit path         | EXIT button only — the title-bar X / Alt+F4 are swallowed |
| Process type      | GUI subsystem: no console; `--help` and startup errors use a message box |

`README.md`, [Protocol notes](README.md#protocol-notes), has the byte layout
behind the ids and ports.
