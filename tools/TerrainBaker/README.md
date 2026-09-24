# TerrainBaker

Builds a terrain database for CigiStarter's HAT/HOT answer out of **the
simulator's own ground heights**, instead of the generic SRTM world model.

```
  option 1  tools\get_terrain.ps1      generic SRTM tiles        minutes, no sim
  option 2  tools\TerrainBaker         SRTM base + sim patches   seconds, needs P3D
```

Option 2 builds on option 1: the SRTM tile is the base, and the bake overlays
the sim's readings on the parts of it that matter.

The sim's readings come through **`PdkTerrainProbe.dll`**, the same in-process
add-on CigiStarter answers HAT/HOT from at run time (see
`tools\PdkTerrainProbe\README.md`). It does not use FSUIPC, slew mode or
repositioning: the surface query is answered about the world at the coordinate
asked for, so a whole tile is sampled in seconds rather than the hours the old
one-cell-at-a-time FSUIPC sweep took.

This file is the design and the reference. **`RUNBOOK.md`** is the step-by-step
procedure for running a real bake on the IG PC.

## Why

The HOST drives its aircraft altitude to the Height Of Terrain the IG reports.
Where Prepar3D has levelled the ground for a runway, SRTM still has the natural
hillside — so a generic tile answers a few metres wrong exactly on final
approach, which is where a few metres matters. Prepar3D's own ground model has
the flattening; nothing else does. This tool asks it and writes the answer back
out as an ordinary `.hgt` tile, so `TerrainDb` reads the result unchanged.

## Build

The project is part of `CigiStarter.sln` (configurations `Debug|x64` and
`Release|x64`), and drops `TerrainBaker.exe` next to `CigiStarter.exe`:

```powershell
msbuild CigiStarter.sln /p:Configuration=Release /p:Platform=x64
```

It links the same static CRT as the IG, so it runs on a fresh PC, and it uses no
SDK of its own (`--help`, `plan` and `compose` need no simulator, and the project
builds without the Prepar3D SDK installed).

## Modes

| Mode | Needs P3D | What it does |
|------|-----------|--------------|
| `plan` | no | prints the tiles, the point counts and a rough cost |
| `probe` | yes | asks the sim about a few spread-out points and prints sim vs SRTM |
| `survey` | yes | walks every grid point in the boxes, appending to `<out>\<tile>.samples.csv` |
| `compose` | no | folds the SRTM base and that CSV into `<out>\<tile>.hgt` |

`probe` and `survey` need Prepar3D running with the add-on loaded; everything
else is offline.

`plan`, `survey` and `compose` take the same `--site` / `--box` / `--res`
options, so the survey and the compose agree on the area. `--tile` selects tiles
for `compose` explicitly.

## Procedure

Run it on the **IG PC**, with Prepar3D up and running (not paused) and the
`PdkTerrainProbe` add-on loaded. Nothing else has to be held still: the bake
reads the world rather than steering the aircraft, so the vendor program or
`CigiP3dBridge` may be running, and the aircraft is never touched.

```powershell
# 0. the add-on, once per IG PC
copy tools\PdkTerrainProbe\bin\Release\PdkTerrainProbe.dll <add-ons folder>
copy tools\PdkTerrainProbe\add-on.xml                     <add-ons folder>
#    ...and restart Prepar3D. See tools\PdkTerrainProbe\README.md.

# 1. the SRTM base (option 1)
.\tools\get_terrain.ps1 -Points "-6.54,106.76","-6.27,106.89","-6.13,106.66","-6.90,107.58" `
                        -Out x64\Release\terrain

# 2. what would be baked
.\x64\Release\TerrainBaker.exe plan

# 3. prove the add-on is answering -- seconds
.\x64\Release\TerrainBaker.exe probe

# 4. the sweep. Ctrl+C is safe: it stops between batches
.\x64\Release\TerrainBaker.exe survey

# 5. assemble and deploy
.\x64\Release\TerrainBaker.exe compose
copy x64\Release\terrain-sim\*.hgt x64\Release\terrain\
```

`compose` prints how far the sim disagreed with SRTM over the sampled cells.
On a flattened field expect a handful of metres; over open country expect ~0.

That is the short version. `RUNBOOK.md`, in this folder, is the full procedure
for a real bake: preconditions, what to check at each step, what good output
looks like, and the failure modes.

### Probe first, and read what it says

`probe` asks about the centre and corners of each box, far enough apart that one
invented number could not pass for five, then asks about the same points again.
It exits non-zero if the add-on answers nothing (Prepar3D paused, or scenery not
loaded) or if the repeat disagrees with the first answer, which would mean the
answers are not tied to the points asked. It also prints the SRTM height and the
difference, so the size of the correction is visible before anything is baked.

The query is **altitude dependent** — Prepar3D decides which surface is meant
from the height it is asked about — which is why `--altitude` exists and defaults
to 3500 m: high enough to be above the ground everywhere in these boxes, so the
answer is the surface an aircraft would land on. It is the same field
CigiStarter fills in at run time with the HAT point's altitude, so a baked tile
and a live answer describe the same surface.

Measured on Prepar3D v5 (dev box, 2026-09), the retired FSUIPC path: `--slew on`
made the position writes land and repeat to 0.05 m, while `--slew off` mostly did
not apply them. That whole question is gone with the surface query, which never
moves the aircraft — and it is why CigiStarter's own live answer moved to the
same source.

### Cost

With the built-in site boxes (±0.10°, ~22 km across) at the default 3
arc-seconds (~90 m), the four fields are about **230 000 points** — S07E106
(172 800) plus S07E107 (57 600), the only two tiles involved.

The probe answers every enabled slot of its table on each simulation tick, and
`survey` keeps all 256 of them full, so the ceiling is 60 ticks/s × 256 ≈
**15 000 points/s** and the full sweep is **well under a minute** — measured at
about 9 400 points/s against a stand-in sim, i.e. ~25 s for the 230 400 points.
In practice the pace is set by the sim's tick, not by the tool.

The sweep is resumable, `--limit N` caps a run for a smoke test, and a point the
sim has no surface for (open water, or no data) is simply not recorded, so that
cell keeps its SRTM height.

Widen a field's box with `--box lat0,lon0,lat1,lon1` if the HOST's approaches
start further out; the cost rises with the square of the box.

### Choosing the resolution

`--res 3` (default) is 3 arc-seconds, ~90 m, a `1201 × 1201` tile of about
2.9 MB — the same shape and cost as an SRTMGL3 download, and the resolution this
deployment asked for. `--res 1` is ~30 m and a `3601 × 3601` tile of about
25 MB; it needs nine times the samples.

A 1 arc-second SRTM base is stepped down to 3 arc-seconds exactly 3:1, so the
bake still builds on option 1's tiles with no second download. Stepping *up* is
refused: it would invent detail the base never had.

## Notes and limits

- The bake does not touch the aircraft: no slew mode, no position writes, so the
  sim is left exactly as it was found.
- The tool writes `<out>\<tile>.samples.csv` alongside the tiles. That file is
  the bake — keep it, and re-running `survey` resumes from it. `--no-resume`
  starts a tile over. `survey` creates `<out>` if needed and stops with a clear
  error if it cannot write: a survey that cannot record its samples must not look
  like it succeeded.
- Only the two `.hgt` shapes `TerrainDb` accepts are written (1 or 3
  arc-second), so a baked tile is a drop-in replacement. Keep the original SRTM
  tiles somewhere for a re-bake.
- The bake reflects the **installed scenery** at the time it runs. Change an
  airport's add-on scenery and that area wants a re-bake. (An IG PC with the
  add-on loaded does not: it answers the same question live.)
