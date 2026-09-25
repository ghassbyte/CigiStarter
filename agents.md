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
HOST sent.

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
setup and the automated tests built on it.

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
  for byte, source-labelled as the HOST. Optional, Npcap-backed, passive clients
  only.
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
| Sim driver        | a 3rd-party vendor program holds Prepar3D **paused** throughout, with **slew off**, positioning the sim's aircraft over FSUIPC; the sim's own aircraft altitude can stay at Prepar3D's startup value, so it is not the HOST's — the anchor is the HOST's Entity Control |
| Terrain source    | P3D's own terrain via the `PdkTerrainProbe` add-on; then an SRTM `.hgt` tileset; then FSUIPC `0x0020` |
| Entity anchor     | entity-relative HAT/HOT is anchored to the HOST's Entity Control (entity 0), a pose at offset 24/32/40 (lat/lon/alt, CIGI 3.3 layout); FSUIPC only until one has arrived |
| Terrain probe     | `tools\PdkTerrainProbe`, a PDK DLL loaded by Prepar3D; free-standing query, no SimObject; optional, and loaded on the bench's Professional Plus v5 and the rig's Professional v6 |
| Terrain database  | `.hgt` tiles in `terrain\` by default or `--terrain-dir`; from `tools\get_terrain.ps1` or a `tools\TerrainBaker` bake |
| Extra IG PCs      | `client.txt` beside the executable, `address[:port]`, read once at startup |
| Fan-out           | one Npcap-injected copy per client, HOST source address/port spoofed |
| Fan-out adapter   | found automatically: this IG's adapter, else the one the route to the HOST leaves by |
| Bench HOST        | the CIGI Host Emulator in `external\`, on the IG PC, in place of the flight model; `tools\test-hathot-emulator.ps1` drives it |
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
