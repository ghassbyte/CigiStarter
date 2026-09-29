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

The three `cloud pat*.bin` captures are the same HOST with a different sortie:
they carry no Atmosphere Control and no clock, and describe their sky the other
way, with a **Component Control** (id 4) on a cloud entity — the *Cloud pattern*
row in the key facts below has the decode.

On a bench there is no flight model: a **CIGI Host Emulator**
(`external\HostEmulator_Win32_v3.3.3\`) runs instead, on the same PC as
CigiStarter, to send the requests and watch the answers. It stands in for the
HOST only as far as this program is concerned — it produces none of the flight
data above — and `README.md`, [Bench testing](README.md#bench-testing), has its
setup and the automated tests built on it. It is driven by its `.def` tables and
a script and reads no `.bin`, so the flight data above is replayed against
CigiStarter from the capture itself instead: `tools\play-bin.ps1` by hand,
`tools\test-simlink-e2e.ps1` with the checks. The emulator's own script route is
gone — the converter that wrote those scripts could only be played through the
emulator's GUI, and the direct replay needs nothing but CigiStarter.

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
  an offline SRTM database, and then to `Valid = 0`. On by default. Entity-relative
  requests are anchored to the HOST's own Entity Control pose (entity 0); the
  sim's own aircraft position was once the fallback anchor (read over FSUIPC)
  and is gone, so there is no anchor at all until the first Entity Control
  arrives. The
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
  `--no-sim-link` turns it off. The same add-on also flies the sim's view when
  `PdkSimLink.ini` sets `camera=<entity id>`: entities moving is not the same as
  the picture moving, because they are non-user objects and Prepar3D's view is a
  per-window camera (the *Camera* row below has the mechanism and the bench
  measurement). The entity that camera rides is created from `own_ship_title`
  rather than from `title` — by default the model-less container the add-on
  ships — because the eye is inside it and a model there is a hull across the
  whole picture. This is the rendering path in this IG PC; the
  vendor program that used to drive the sim's own aircraft over FSUIPC is what
  it replaces.
- **Puts the HOST's world on screen, not just its aircraft** — a Celestial
  Sphere Control (id 9) is the wall clock and date, an Atmosphere Control
  (id 10) the air, a Weather Control (id 12) the cloud and precipitation, and a
  Cloud Pattern (a Component Control, id 4 — the only way the reference HOST
  ever describes a sky, in the three `cloud pat*.bin` captures). The IG
  accumulates them, folds the operator's time-of-day and weather presets on top
  (`Environment.h`), and publishes the resolved world on the same shared region
  (`environment`, `kVersion` 4) only when something changed — a once-and-forget
  clock, not one re-set every frame. A group nobody has spoken for is not
  applied, so the real HOST, whose per-frame list has an Atmosphere Control and
  no clock or weather, leaves the sim's own clock and sky alone. The add-on sets
  the clock through `IGlobalData::SetTime`/`SetDate` **local** (the site is
  UTC+7) and the air and sky through a synthesised METAR plus the exact
  `IWeatherSystemV500` global setters, and reports back the generation it
  applied, the time of day the sim itself reports (`GetGeneralTimeOfDay`) and
  the sky the sim itself holds — the `GLOB` station's cloud layers and the
  observation it parsed (`kVersion` 5), which is how "the cloud reached the sim"
  is answered by the sim rather than by the region the IG wrote.
  The gate is enforced on the sim side too: the METAR is sent only for a sky
  that is present (a METAR always names a sky, so an atmosphere-only change
  would otherwise clear the sim's cloud) and the air setters only for an
  atmosphere that is present, so an operator's preset chosen before warm-up
  cannot push the resolved row's place-holder temperature and pressure into the
  sim.
  On by default with the rest of the writer (`--no-sim-link` turns it off too).
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
  local sim's own aircraft. The requests are the same six offsets wherever the
  aircraft is, so an anchor left over another airfield cannot be seen on the
  wire — it just answers the wrong terrain, and the HOST flies the aircraft to
  meet it. That is the rig's "repositions, then parks at the departure ground"
  failure: the anchor had frozen at the departure field while the HOST flew
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
| Terrain source    | P3D's own terrain via the `PdkTerrainProbe` add-on; then an SRTM `.hgt` tileset; then `Valid = 0` (the sim's own ground height, FSUIPC `0x0020`, was the last resort and went with the rest of the FSUIPC path) |
| Entity anchor     | entity-relative HAT/HOT is anchored to the HOST's Entity Control (entity 0), a pose at offset 24/32/40 (lat/lon/alt, CIGI 3.3 layout); until one has arrived there is no anchor and those requests are answered `Valid = 0` (the sim's own aircraft position, once read over FSUIPC, used to stand in) |
| Entity writer     | the HOST's entities, rendered by the `tools\PdkSimLink` PDK add-on inside the local Prepar3D: it creates a sim object per entity and moves it to the pose CigiStarter published. The object is `CigiEntity`, a physics-free `ISimObject` the add-on registers (`tools\PdkSimLink\CigiEntity.h`); a stock container title still creates the sim's own object for comparison, and the entity the camera rides is created from a second, permanently model-free container (`own_ship_title`, default `CigiStarter.Entity.OwnShip`) so the eye is not inside a drawn model. On by default; `--no-sim-link` leaves the sim alone |
| Sim link contract | `SimLinkIpc.h`, a private mapping `Local\CigiStarter.SimLink.v1` with its own `kVersion` (5): 64 entity rows (degrees + metres, per-row seqlock), 64 read-back rows (`held[]`) plus the drift words the add-on fills with the pose each object is actually at, then the HOST's own eye (`view`, one row), then the resolved world (`environment`) and the add-on's answer to it (`environmentStatus` — the generation it applied, the time of day the sim reports, and the sky the sim holds: its `GLOB` station's cloud layers and the observation it parsed), each appended in turn so the tables above keep their offsets, plus the add-on's status words. One-way — nothing goes back to the HOST — and deliberately separate from `TerrainProbeIpc.h`, so writer and terrain work cannot break each other |
| Entity hold | Measured on the bench, 2026-09-29. Against the AS332, a stock object: the sim holds a written pose (0.02 m on a fixed pose; 0.19 m / 0.24° worst over a whole captured sortie), but **refuses a pose far below the ground under it** — the object is held above the ground at an altitude that is a property of the *place*, not the surface (176.6 m over 174.3 m terrain; +26.7 m over water at 0.0 m), and a command only a little below the ground is followed (168 m → 168.2 m; −7 m → −6.8 m). Against `CigiEntity`, the physics-free object: held **exactly** (0.00 m / 0.00 deg), and a command below the terrain is where the command puts it, with no pause or slew |
| Entity writer cut | pose (position and attitude) **and the world** (time of day, air, cloud and precipitation, and the HOST's cloud *pattern* — see *Cloud pattern* above); effects (lights, smoke, damage) are next. The physics-free object ships with **no model**, so nothing is drawn until its container is given one — a content decision per entity type, not part of the link |
| Time and weather  | the HOST's Celestial Sphere (id 9), Atmosphere (id 10) and Weather Control (id 12), accumulated and resolved with the operator's presets (`Environment.h`) and published only on change as `environment` (`kVersion` 4). Applied by the add-on as PDK `IGlobalData::SetTime`/`SetDate` **local** (the site is UTC+7) and a synthesised `GLOB` METAR plus the exact `IWeatherSystemV500` setters. No capture carries a Weather Control, so cloud/precipitation is contract-tested and preset-driven; the four `*_replay.bin` time captures carry a clock but the real HOST never does, so a group the HOST has not sent is not applied rather than read as a default. Operator presets: time `Host`/Dawn/Day/Dusk/Night, weather `Host`/Clear/Cloudy/Rain/Storm |
| Cloud pattern     | the reference HOST's *other* way of describing a sky, and the only one in the three `cloud pat*.bin` captures: a **Component Control** (id 4), component `0x1001` on the entity `0x9001` it also creates, class Entity (0 — *not* the ICD's Global Layered Weather), the state the pattern's index (1/2/3, then 0 for "take it away") and the first three data floats the pattern's own footprint and altitude, **all in metres** — the vendor's tool writes nautical miles and feet and converts (14816 = 8 × 1852, 12964 = 7 × 1852, 914.4 = 3000 ft). Prepar3D has cloud layers, not cloud volumes at a place, so a pattern resolves to one: base = data 3, coverage = the footprint's area in square nautical miles at 1 %/nmi² floored at 30 % and capped at 100 %, cumulus, ~900 m thick, no precipitation; state 0 is an explicit clear sky so the HOST taking the cloud away does not leave the sim holding it. The numbers are the wire's; the mapping is this IG's and is recorded as an assumption, because the vendor's own mapping is not in the capture. Measured on the bench (2026-09-29): pat1 56 % broken at 3000 ft, pat2 30 % scattered at 3500 ft, pat3 100 % overcast at 3000 ft, each read back out of the sim's own weather system as `5CU030` / `3CU035` / `8CU030`, and `CLR` again after the state-0 message |
| Camera            | the same add-on also flies the sim's view when `PdkSimLink.ini` sets `camera=<entity id>` (off by default): it puts every 3D window onto a Fixed-origin camera definition (`camera_view`, default Prepar3D's own `Cigi View` — `Origin = Fixed`, `MenuHidden`, `CycleHidden`, `HideUserAircraft`) and applies the HOST's own eye for that entity every frame from the window plugin's `OnPreCameraUpdate`, through `ICameraSystemV430::SetLLA` (lat/lon degrees, altitude metres) and `SetPBH` (pitch, bank, heading — CIGI's roll/pitch/yaw swap into those slots, and the bank is **negated**: Prepar3D's bank axis is the opposite of CIGI's roll, a sign a read-back can never show because it round-trips the number it was given; the predecessor measured the same inversion by eye, and a replay of `gnd-fly-bank.bin` through this writer — the aircraft turning right while banking left — is what showed the writer was missing it; with the negation in and replayed again, the same pair leans into its turn both ways, checked by eye on the bench on 2026-09-29). The eye is the HOST's View Control, carried across with the poses (`view` in the shared region, a `kVersion` 3 addition): the offsets are **body** offsets (`+X` out the nose, `+Y` out the right wing, `+Z` out the bottom), so they are rotated out of the body frame by the entity's *own* attitude and added to its position — `SimLinkMap.h`'s `viewEyePose()` — while the view's angles are added to the entity's, heading wrapping. With no View Control, or one attached to another entity, the eye stays on the reference point, which is what this did before. In every capture the definition is one and the same — 2.8 m forward of entity 0's reference point, 5 cm right, 0.5 m up, level, yaw 359, a degree left — i.e. the pilot's seat rather than a point on the airframe, which is the whole of the difference between "the picture is drawn from the aircraft" and a cockpit view. Moving an entity object is not enough on its own: they are non-user objects and the view belongs to a *window*, and a ViewGroup renders its channels from that window's **active** camera with each channel's frustum offsets on top, so the dome keeps its own angles and gains a moving origin. An "AI-object camera" was the first choice and is unreachable from a PDK add-on — the PDK has `CreateCameraInstance` but no `CreateCameraDefinition`, and `ActivateEntityTracking` is a look-at, not an eye. The entity the camera rides is created from `own_ship_title` (default `CigiStarter.Entity.OwnShip`, a model-less container the add-on ships), not from `title`, because the eye is *inside* it: a model there is a hull across the whole picture, which is what the bench showed the first time the camera ran against a container that had been given the AS332's body ("the view is inside the cabin of aircraft, not the cockpit view like usual" - and the same parked pose, with the own ship on its own container, showed the apron and the hangars). Prepar3D hides the sim's *own* aircraft for this camera already (`HideUserAircraft` on the `Cigi View` definition) and this is the same rule for the entity the add-on created. The own ship is still created, moved and read back like every other entity, so the link is proved on it too. Measured on the bench, 2026-09-29 (replays of the banked pair, `right.bin` then `left.bin`, `camera=0`, `camera_log=1`, `title=CigiStarter.Entity`): the sim's `GetLLA`/`GetPBH` read back **exactly** the eye every sample while the aircraft was moving — e.g. entity `-6.473351 106.725805 465.26 | 15.51 1.56 253.38` becomes the eye `-6.473356 106.725780 465.81 | 15.51 1.56 252.38` (2.8 m out the nose rotated into a latitude and longitude, the view's 1° taken off the heading), and the camera was told the bank as its negative (`told pbh 1.56 -15.51 252.38`) and reads back exactly that, with a heading of 252.38 returning as the equivalent `-107.62` past 180°. The first sample read `0.0 0.0 1.00` — the definition's own fixed point — and every sample after the first `SetLLA` matched the eye, so the pose is the HOST's and not the definition's. The picture was checked by eye at the same time: the parked pose shows the apron, the hangars and the mountains (the world from the eye, no hull across it), and a mid-sortie pose at 292 m shows the terrain and the horizon from the pilot's seat. What the bench cannot show is the dome itself — one monitor, no Immersive Display Pro — so the per-channel angles are proved only where the ViewGroup is, and a level aircraft cannot tell a 1° bank from a 1° turn, which is why the order of the View Control's angles is still on the rig list (`TODO.md`, 8). `camera_log=1` writes that comparison beside the DLL — what the eye is and what the camera reports — and the camera has no status word in the shared region |
| Entity state      | the HOST marks the entities it drives **Active (1)**, the CIGI Common Library's numbering (`Standby=0, Active=1, Remove/Destroyed=2`): entities 0 and 3 carry state 1 and a moving pose in every frame of every capture, and entity 5, the all-zero placeholder, carries Remove (2). Read the field the other way round and the writer renders nothing at all — every pose still arrives perfectly, which is why it was caught against the captures rather than on the rig |
| Terrain probe     | `tools\PdkTerrainProbe`, a PDK DLL loaded by Prepar3D; free-standing query, no SimObject; optional, and loaded on the bench's Professional Plus v5 and the rig's Professional v6. The answer carries the sim's own surface classification (hard/soft/water, and whether it is an object's *platform*); the answering policy needs no more than the full surface query already gives it — its height carries a platform's deck where the elevation-only call returned sea level — and `TerrainBaker probe` and the window's `Terrain` line show the classification |
| Terrain database  | `.hgt` tiles in `terrain\` by default or `--terrain-dir`; from `tools\get_terrain.ps1` or a `tools\TerrainBaker` bake |
| Extra IG PCs      | `client.txt` beside the executable, `address[:port]`, read once at startup |
| Fan-out           | one Npcap-injected copy per client, source address/port spoofed as the HOST by default (`--no-relay-spoof` sends this IG's) |
| Fan-out adapter   | found automatically: this IG's adapter, else the one the route to the HOST leaves by |
| Bench HOST        | the CIGI Host Emulator in `external\`, on the IG PC, in place of the flight model; `tools\test-hathot-emulator.ps1` drives it |
| Recorded sortie   | a `.bin` capture is a recording, and the emulator reads none; `tools\play-bin.ps1` replays one straight into a bench CigiStarter for watching, and `tools\test-simlink-e2e.ps1` replays it with the checks. (The emulator's own `.scp` script route is gone with the converter that fed it) |
| Contract tests    | `tools\test-all.ps1` builds and runs every SDK-free harness — the entity writer's and the terrain probe's — with no Prepar3D and no SDK; `tools\test-simlink.ps1` is the writer's alone |
| Local e2e test    | `tools\test-simlink-e2e.ps1` is the only check that reaches inside the sim: it replays a recorded `.bin` HOST stream into a bench CigiStarter and reads PdkSimLink's status words, entity rows, the HOST's eye and the sim's own weather station back out of the running Prepar3D, so the objects it creates, the poses it applies, the viewpoint it flies and the sky it builds are proved on this PC. On `cloud pat1/2/3.bin` it also checks the pattern the HOST sent, the base and coverage it resolved to, and the cloud the sim's own station then holds. It fails up front on a stale `PdkSimLink.dll` (the add-on is loaded once, at sim startup) or a `title=` the installation does not have |
| Elevation         | none requested (`asInvoker`); the fan-out needs Npcap, not elevation |
| Reference SOF rate | ~100 Hz (~10 ms, an 8/12 ms alternation) |
| HOST frame rate   | ~53 Hz — its IG Control counter advances 0.54 per datagram, i.e. the HOST is asked for about twice the frames it has |
| SOF rate here     | 150 Hz default (1..100000 Hz), paced to the HOST unless flow control is off |
| Operator UI       | plain Win32 window: START / STOP / EXIT, a HAT/HOT check box, a flow-control check box, a SOF rate box, and time-of-day and weather combo boxes (`Host` + the four times / `Host` + Clear/Cloudy/Rain/Storm); its `World` line says what is being applied and its `Sky` line what the sim's own weather system holds, so the HOST's cloud pattern and a HOST that sent no sky can be told apart |
| Start-up          | streams from launch, window minimised; `--no-auto-start` starts paused |
| Exit path         | EXIT button only — the title-bar X / Alt+F4 are swallowed |
| Process type      | GUI subsystem: no console; `--help` and startup errors use a message box |

`README.md`, [Protocol notes](README.md#protocol-notes), has the byte layout
behind the ids and ports.
