# PdkTerrainProbe

An add-on DLL for Prepar3D that lets CigiStarter answer the HOST's HAT/HOT
terrain requests from the sim's **own terrain engine**, instead of from an
offline copy of the world.

## Why

The HOST drives its aircraft altitude from the Height Of Terrain the IG reports,
so a wrong or stale HOT flies the aircraft into the ground. CigiStarter had two
ways to get it before this add-on, and both are approximations:

| Source | Problem |
|---|---|
| `TerrainDb` (offline SRTM `.hgt`) | Cannot know a runway's flattening or any changed scenery, and has to be re-baked whenever the scenery changes. At Atang Senjaya it reads ~175.5 m where the reference IG reported ~178 m. |
| `TerrainSource` (FSUIPC `0x0020`) | Reported the ground under the local aircraft and nothing else, so it could stand in only when the request was where the sim's own aircraft was. It **froze** while the program that drives the aircraft held the sim in slew. That path has since been **removed** from CigiStarter: the local sim's own aircraft is no longer flown (the HOST's entities are separate objects), so a reading taken from it described a parked aircraft. `TerrainDb` is the only fallback left. |

Prepar3D will answer the question directly, from the terrain it is about to put
the aircraft on. The PDK exposes that as `ISurfaceQueryManagerV400`, and it is a
**free-standing query** — not tied to an object — so nothing has to be created,
placed or teleported around the world. This DLL picks the service up and
publishes the answers through a block of shared memory for CigiStarter to read.
No scene needs pre-mapping; a newly added airport is answered live.

## How the two halves talk

`PdkTerrainProbe.dll` runs **inside** Prepar3D. CigiStarter runs **outside** it.
They share one block of memory, described in `../../TerrainProbeIpc.h`:

```
   CigiStarter (IG)                    PdkTerrainProbe.dll (in Prepar3D)
        |                                        |
        |  writes queries[i]  --------------->   |  on its next tick:
        |    lat, lon, alt, key                  |    asks ISurfaceQueryManagerV400
        |                                        |    for each enabled point
        |  <---------------  writes results[i]   |
        |    hot, status, surface, key,          |
        |    echo of lat/lon                     |
```

* The IG's read is guarded by a seqlock, so a row being rewritten underneath it
  is never believed.
* Every result echoes the point it was taken at, so a reading is only ever used
  for the point it was actually measured at. A request that has moved in the
  meantime is refused, and the IG falls back to the database — it is never
  answered with the terrain of wherever the aircraft used to be.
* The first request for a point is waited for, briefly, rather than being handed
  to the database: an answer only lands on a tick, so the IG waits up to 120 ms
  for that first reply and stops the moment it arrives. Later replies for the
  same point are already in the row, so only the first one ever waits.
* The layout of that memory is pinned in `TerrainProbeIpc.h` — an explicit
  `pack(8)` plus a `static_assert` on every size and offset. The fields being
  fixed width is not enough on its own: if the two binaries were built at
  different packings they would agree on the first slot and silently misread
  every slot after it, answering from the wrong row. A layout change therefore
  has to bump `kVersion` too, so a stale DLL is refused when it attaches instead
  of being misread. That applies to the diagnostic word `diag` as well: it was
  the header's reserved word given a meaning, so it moved no offset, but the
  diagnostic **tail** added in `kVersion` 2 comes after the tables and made the
  mapping larger, so the version was bumped with it.
* The table is **metric**: CIGI defines HAT/HOT in metres, and that is what
  CigiStarter puts on the wire. The DLL converts at the sim boundary — the
  surface query takes its altitude in **feet** and returns a height in feet — so
  the metres in `results[i]` are the query's feet value converted back. Nothing
  metric reaches the PDK call, and nothing imperial reaches the wire.
* Each result also carries the sim's **classification of the surface** it
  answered with: the category (hard / soft / water) and whether the surface is
  an *object's* platform rather than terrain. Both live in `materialCode`, a
  field the contract had always reserved for exactly this (it sat at 0 while the
  add-on asked the elevation-only call), so no offset moved and `kVersion` did
  not change. The flag matters because Prepar3D's collision will rest an aircraft
  on a platform — a vessel's deck, a rig — while the terrain under it is still
  sea level, so an answer taken from the height alone flies the aircraft into the
  hull. Switching to the full surface query is what fixed that: over a moored
  vessel it returns the deck's height where the elevation-only call returned sea
  level, so no policy had to be built on the flag — it is what lets an operator
  see that the ground answered with is an object and not terrain.
  `TerrainBaker probe` prints it per point, and the operator window shows it on
  the `Terrain` line.

CigiStarter only ever wants about six points at a time, but the table holds 256
and the add-on answers every enabled one on each tick. `tools\TerrainBaker` uses
that: it fills the whole table to sample a tile in seconds, which is why a bake
no longer needs FSUIPC, a reposition per cell, or any particular sim mode.

## Building

There are two DLLs, one per simulator, each built against that simulator's own
headers. Needs the Prepar3D PDK headers (v5 or v6); a copy of `inc\PDK` from the
v6 SDK ships in `..\..\external\Prepar3D-SDK\inc\PDK\`, and that is what the
project defaults to — the add-on links only system libraries, so a full SDK
installation is not needed.

The surface query (`ISurfaceQueryManagerV400`) is the same in v5 and v6, but the
services around it grew between the SDKs and the versions do not share a GUID:
the frame-event service (`IEventServiceV510` on v5, `IEventServiceV600` on v6)
and the object manager, used only for the fallback tick (`V520` on v5, `V540` on
v6). The project takes the interfaces from the SDK path — `v5 SDK` in it selects
the v5 ones — so point `/p:P3DSdkDir` at the SDK for the simulator you are
building for:

```
# for Prepar3D v6, against the vendored v6 headers (the default)
msbuild PdkTerrainProbe.vcxproj /p:Configuration=Release /p:Platform=x64

# for Prepar3D v5, against the v5 SDK
msbuild PdkTerrainProbe.vcxproj /p:Configuration=Release /p:Platform=x64 ^
        /p:P3DSdkDir="C:\Program Files\Lockheed Martin\Prepar3D v5 SDK 5.4.9.28482"
```

Each lands in its own folder, named for the simulator, so both can be built and
kept side by side — the SDK path selects the folder as well as the interface, so
one build cannot overwrite the other:

| Built for | Headers | Output |
|---|---|---|
| Prepar3D v6 | the vendored copy, or a v6 SDK via `/p:P3DSdkDir` | `bin\Release-v6\PdkTerrainProbe.dll` |
| Prepar3D v5 | a v5 SDK via `/p:P3DSdkDir` | `bin\Release-v5\PdkTerrainProbe.dll` |

Deploy the one that matches the Prepar3D on the machine. Both are called
`PdkTerrainProbe.dll`, because `add-on.xml` names it that way.

Note: this project is deliberately **not** in `CigiStarter.sln`. It needs the
sim's SDK, and a machine that only builds the IG should not have its build
broken by that. Build it where the SDK is, and ferry the DLL.

## Installing on the IG PC

1. Put the pair in Prepar3D's default add-on discovery folder, one level deep:

   ```
   %USERPROFILE%\Documents\Prepar3D v6 Add-ons\PdkTerrainProbe\
   ```

   (use `...\Prepar3D v5 Add-ons\` on v5). The Documents folder is the one
   Prepar3D discovers; the `add-ons` folder beside `add-ons.cfg` under
   `%PROGRAMDATA%\Lockheed Martin\Prepar3D v6\` is **not** scanned, and an
   add-on left there is silently never loaded. For a machine-wide install
   instead of a per-user one, register the package:

   ```
   Prepar3D.exe "-Configure: Category=Add-on Package, Operation=Add, Path=<the folder>"
   ```

2. Copy `PdkTerrainProbe.dll` and `add-on.xml` into it. `add-on.xml` refers to
   the DLL by relative path, so the two must sit together.

3. Restart Prepar3D. Nothing is clicked or configured — the add-on registers
   itself, and CigiStarter starts using it as soon as its tick is running.

Check that it loaded:

```
tasklist /m PdkTerrainProbe.dll
```

Prepar3D should be listed. If it is not, the usual cause is not the files but
the enable flag: Prepar3D keeps one `ACTIVE` entry per package in
`%APPDATA%\Lockheed Martin\Prepar3D v5\add-ons.cfg` (v6 likewise), its
**Options → Add-ons** dialog shows the same switch, and a package with
`ACTIVE=false` is not loaded at all — the folder and `add-on.xml` can be
perfect while the DLL never appears. Look there first, then at the folder and
`add-on.xml`, then at Prepar3D's own logging for loading errors.

It has been loaded on both editions of the sim in use — Professional Plus (the
bench's v5) and Professional (the rig's v6) — so it is not restricted to
Professional Plus.

No elevation is needed, and unlike the fan-out it needs no Npcap.

## Behaviour when it is not there

Everything degrades quietly, so an IG PC without the add-on still works exactly
as before:

* CigiStarter opens the shared region lazily and re-tries every 2 s, so the two
  may be started in either order.
* No add-on, or no tick yet, or a tick that has stopped (the sim exited and left
  a mapping behind) → the probe reports itself unavailable and CigiStarter falls
  back to `TerrainDb`, and then to `Valid = 0`. A **paused** sim is not one of
  those
  cases: Prepar3D's own ticks stop while it is paused, and the add-on's own
  thread keeps the table answering underneath them.
* `--terrain-offset` is **not** applied to a height that came from the sim. That
  option exists to calibrate the *database's* datum against the sim; a value
  from the sim needs no calibration, and applying it would correct it twice.

## How the tick survives a pause — the diagnostics

The surface query is free-standing, but something has to *call* it, and that
something has to keep running while the IG's aircraft is being positioned. The
program that drives the aircraft holds Prepar3D **paused** — the rig's vendor
program does so throughout, with slew off — and a paused sim stops calling an
object's `ISimulation::Update` altogether. A tick hung off the user object
therefore goes silent exactly when the HOST is asking for terrain, and with the
sim paused throughout it never runs at all. From outside it looked present but
silent: `alive` stayed 1 (nothing had detached it) while `ticks` froze.

The first answer was Prepar3D's global **`EVENTID_Frame`** event
(`IEventServiceV400`), signalled once per rendered frame, which is taken to keep
being signalled while the sim is paused. **On v5 it does. On the rig's v6 it does
not**: pausing stops the frame loop as well, so the frame tick dies with the
pause, and the probe went quiet for as long as the sim was paused. The object
`ISimulation` cannot cover that either (a pause stops it too) and neither can any
other PDK callback.

So the probe now has a tick of its own, and Prepar3D's ticks are the fast path:
the table is serviced by, in order,

1. the per-frame event, once per rendered frame — the tick of a running sim;
2. the object's `ISimulation`, until a frame event has been seen and again if one
   stops arriving (`Update()` defers while the frame event is live, so the two
   never both service the table);
3. a **thread of the add-on's own**, started at load and stopped at unload, which
   steps in only after both of the above have been silent for a full second
   (`kWatchGraceMs`), and then services the table every `kWatchMs` (50 ms) for as
   long as they stay silent. Prepar3D's silence is timed from its own last tick,
   not from the shared heartbeat, because the thread's own passes advance that
   heartbeat too. A running sim never reaches it, so the calls made off
   Prepar3D's thread can only happen while Prepar3D's own loop has stopped —
   which is when the terrain engine is not being driven by a frame. The rate
   matters: a paused sim is the rig's normal state, and at about one answer a
   second every request for a point that had moved waited out CigiStarter's
   first-answer budget, dropping it onto the database and stalling the SOF
   stream while it waited.

Because the frame event needs no object at all, the object-lifecycle failure mode
— a reposition handing the user role to a fresh object and stranding the tick —
is gone; the old object counters are still published for comparison.

What the sim is doing is recorded as well, from Prepar3D's message events
(`EVENTID_Message`): pause/unpause, SimConnect "simulating" start/stop, a
scenario load, a user-vehicle change. That is what tells a paused sim apart from
a genuine loss of the terrain service.

The report is the last word of the shared header (`Shared::diag`) plus a
four-word **diagnostic tail appended after the two tables**, so no slot offset
moves. Both are diagnostic only; nothing decides anything from them. Bit 31
(`diag::kPresent`) is set once the word is filled, so a reader treats a 0 as "no
diagnosis available" rather than as fact.

| `diag` bits | Meaning |
|---|---|
| 0..7 | user-object create callbacks seen (saturating at 255) |
| 8..15 | removals of the object the tick is on, or of a user object (saturating at 255) |
| 16..23 | `RegisterSimulation` failures (saturating at 255) |
| 24 | a simulation is bound (the fallback) |
| 25 | the bound object **was** the sim's user object at the last tick |
| 26 | a user-object create was ignored because a simulation was already bound |
| 27 | the newest pass was serviced by the add-on's **own thread** (Prepar3D's ticks have stopped) |
| 31 | the word is filled (a build that publishes diagnostics) |

| Tail field | Meaning |
|---|---|
| `frameTicks` | `EVENTID_Frame` callbacks seen; > 0 means the frame tick is being delivered |
| `simEvents` | simulator messages: bit 0 is live "paused", bits 8..16 are sticky "seen pause / unpause / sim-stop / sim-start / scenario-load / vehicle-change / frozen" |
| `simMode` | `GetMode()` at the last object tick |
| `frameFails` | failures to obtain or register the frame event |

How to read a stopped tick:

* **`ticks` advancing, bit 27 set** — Prepar3D's own ticks have stopped and the
  add-on's thread is servicing the table. This is the *normal* state of a paused
  sim, not a fault: the answers keep coming, they are just not coming from
  Prepar3D's frame loop.
* **`ticks` frozen, `frameTicks` > 0** — the frame event was delivered at some
  point but is no longer arriving, and neither the object tick nor the add-on's
  own thread has stepped in. The thread is started at load and only stops with
  the DLL, so this means the whole add-on has stopped running, not just the frame
  loop.
* **`ticks` frozen, `frameTicks` == 0** — no frame event was ever delivered and
  the object tick never ran either. Deploy the current DLL; `frameFails` says
  whether set-up failed.
* **`alive` clear** — there is no surface service at all, which is a different
  fault.

Three places report it, and they all read the same layout:

* CigiStarter's window grows a `Probe :` line under `HAT/HOT` whenever there is
  something to say — when the probe is not the source (naming the reason and the
  counters), and also when it *is* the source but answering from its own thread
  because Prepar3D has stopped ticking (the pause). Previously the window could
  only say that the answer had fallen back, never why.
* `TerrainBaker probe` (and `requireProbe`, the gate the other modes pass) turns
  the old one-line `the probe is loaded but its simulation tick is not running`
  into the evidence behind it, and prints a healthy baseline on a good run.
* `tools\test-hathot-emulator.ps1` decodes the word and the tail in its preflight
  and watches the tick counter move, so a bench run says `TICK STOPPED` instead
  of a tick-rate that looks healthy. It skips the "sim is the terrain source"
  test when the tick has stopped, rather than blaming the add-on for not being
  loaded.

`tools\TerrainBaker` reads the same layout through `../../TerrainProbeIpc.h`, so
the two never drift.

## Testing without Prepar3D

`test\PdkTerrainProbeTest` compiles CigiStarter's half of the protocol and
drives it against a fake sim that stands in for this DLL. It covers the cases
that matter for safety: that no height is ever invented, that a reading is
refused once its point has moved, that a sim which stops ticking is dropped,
that a row caught mid-rewrite is not believed, and that the surface
classification is passed through when it is published without ever being left
over from another point when it is not.

```
msbuild test\PdkTerrainProbeTest.vcxproj /p:Configuration=Release /p:Platform=x64
test\x64\Release\PdkTerrainProbeTest.exe
```

The same executable can also play the sim for a *different* process, which is
how TerrainBaker's batch driver is exercised without Prepar3D:

```
test\x64\Release\PdkTerrainProbeTest.exe --serve 30
```

While it is serving it answers every enabled slot with `100 + lat*10 + lon +
alt*0.001`, so a tool run against it can be checked against that formula.

What it cannot cover is Prepar3D's side: whether `QuerySurfaceInformation`
returns what the reference IG returned at a given airport — and, for a vessel or
rig standing in the sea, whether the sim calls that surface a *platform* at all.
Both need the sim, and are the first things to check on the IG PC once the add-on
is loaded (`TerrainBaker probe`, or the window's `Terrain` line). On the rig's v6
the platform half is **settled**: over a moored vessel the sim answers the deck
as `[hard, platform]` at the deck's own height, from a query point at or above
it — where the elevation-only call answered sea level.
