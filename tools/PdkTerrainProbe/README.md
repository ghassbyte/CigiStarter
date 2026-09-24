# PdkTerrainProbe

An add-on DLL for Prepar3D that lets CigiStarter answer the HOST's HAT/HOT
terrain requests from the sim's **own terrain engine**, instead of from an
offline copy of the world.

## Why

The HOST drives its aircraft altitude from the Height Of Terrain the IG reports,
so a wrong or stale HOT flies the aircraft into the ground. CigiStarter had two
ways to get it, and both are approximations:

| Source | Problem |
|---|---|
| `TerrainDb` (offline SRTM `.hgt`) | Cannot know a runway's flattening or any changed scenery, and has to be re-baked whenever the scenery changes. At Atang Senjaya it reads ~175.5 m where the reference IG reported ~178 m. |
| `TerrainSource` (FSUIPC `0x0020`) | Reports the ground under the local aircraft, which **freezes** while the aircraft is being slewed into position — which is exactly how CigiP3dBridge drives it. |

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
        |  writes queries[i]  --------------->   |  on its next sim tick:
        |    lat, lon, alt, key                  |    asks ISurfaceQueryManagerV400
        |                                        |    for each enabled point
        |  <---------------  writes results[i]   |
        |    hot, status, key, echo of lat/lon   |
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
  of being misread.
* The table is **metric**: CIGI defines HAT/HOT in metres, and that is what
  CigiStarter puts on the wire. The DLL converts at the sim boundary — the
  surface query takes its altitude in **feet** and returns a height in feet — so
  the metres in `results[i]` are the query's feet value converted back. Nothing
  metric reaches the PDK call, and nothing imperial reaches the wire.

CigiStarter only ever wants about six points at a time, but the table holds 256
and the add-on answers every enabled one on each tick. `tools\TerrainBaker` uses
that: it fills the whole table to sample a tile in seconds, which is why a bake
no longer needs FSUIPC, slew mode or a reposition per cell.

## Building

There are two DLLs, one per simulator, each built against that simulator's own
headers. Needs the Prepar3D PDK headers (v5 or v6); a copy of `inc\PDK` from the
v6 SDK ships in `..\..\external\Prepar3D-SDK\inc\PDK\`, and that is what the
project defaults to — the add-on links only system libraries, so a full SDK
installation is not needed.

The surface query (`ISurfaceQueryManagerV400`) is the same in v5 and v6, but the
tick that drives it hangs off the object manager, and that interface grew a
method: **v5 tops out at `V520`, v6 at `V540`**, and the two do not share a GUID.
The project takes the interface from the SDK path — `v5 SDK` in it selects
`V520` — so point `/p:P3DSdkDir` at the SDK for the simulator you are building
for:

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

No elevation is needed, and unlike the fan-out it needs no Npcap.

## Behaviour when it is not there

Everything degrades quietly, so an IG PC without the add-on still works exactly
as before:

* CigiStarter opens the shared region lazily and re-tries every 2 s, so the two
  may be started in either order.
* No add-on, or no tick yet, or a tick that has stopped (the sim exited and left
  a mapping behind) → the probe reports itself unavailable and CigiStarter falls
  back to `TerrainDb`, and then to FSUIPC.
* `--terrain-offset` is **not** applied to a height that came from the sim. That
  option exists to calibrate the *database's* datum against the sim; a value
  from the sim needs no calibration, and applying it would correct it twice.

## Testing without Prepar3D

`test\PdkTerrainProbeTest` compiles CigiStarter's half of the protocol and
drives it against a fake sim that stands in for this DLL. It covers the cases
that matter for safety: that no height is ever invented, that a reading is
refused once its point has moved, that a sim which stops ticking is dropped, and
that a row caught mid-rewrite is not believed.

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

What it cannot cover is Prepar3D's side: whether `QuerySurfaceElevation` returns
what the reference IG returned at a given airport. That needs the sim, and is
the first thing to check on the IG PC once the add-on is loaded.
