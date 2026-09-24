# TerrainBaker

Builds a terrain database for CigiStarter's HAT/HOT answer out of **the
simulator's own ground heights**, instead of the generic SRTM world model.

```
  option 1  tools\get_terrain.ps1      generic SRTM tiles        minutes, no sim
  option 2  tools\TerrainBaker         SRTM base + sim patches   hours, needs P3D
```

Option 2 builds on option 1: the SRTM tile is the base, and the bake overlays
the sim's readings on the parts of it that matter.

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

It links the same FSUIPC SDK as the IG, with the same static CRT, so it runs on
a fresh PC. `--help`, `plan` and `compose` need no simulator.

## Modes

| Mode | Needs P3D | What it does |
|------|-----------|--------------|
| `plan` | no | prints the tiles, the point counts and a rough cost |
| `probe` | yes | samples a few spread-out points and says whether the ground model tracks |
| `survey` | yes | walks every grid point in the boxes, appending to `<out>\<tile>.samples.csv` |
| `compose` | no | folds the SRTM base and that CSV into `<out>\<tile>.hgt` |

`plan`, `survey` and `compose` take the same `--site` / `--box` / `--res`
options, so the survey and the compose agree on the area. `--tile` selects tiles
for `compose` explicitly.

## Procedure

Run it on the **IG PC**, with Prepar3D up and running (not paused) and nothing
else driving the aircraft — the vendor program or `CigiP3dBridge` must be
closed, because this tool moves the aircraft itself. It sets slew mode for the
sweep and switches it back off at the end.

```powershell
# 1. the SRTM base (option 1)
.\tools\get_terrain.ps1 -Points "-6.54,106.76","-6.27,106.89","-6.13,106.66","-6.90,107.58" `
                        -Out x64\Release\terrain

# 2. what would be baked
.\x64\Release\TerrainBaker.exe plan

# 3. prove the link and the tracking -- minutes, not hours
.\x64\Release\TerrainBaker.exe probe

# 4. the sweep. Ctrl+C is safe: it stops after the sample in flight
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

`probe` samples the centre and corners of each box, far enough apart that a
frozen ground model cannot hide, then revisits three of the points. It exits
non-zero on either failure: values that never move at all (a frozen ground
model), or values that do not repeat at fixed coordinates (the position writes
are not being applied, so a reading belongs to wherever the aircraft already
was). The second one is the nastier, because it still produces plausible-looking
numbers.

FSUIPC's `0x0020` is *ground altitude under the player aircraft*, and Prepar3D
can freeze its ground model while an aircraft is held in slew — which is why
CigiStarter needs its own database at all. So neither mode is assumed: the tool
reads its own latitude/longitude back in the same round trip as the ground
height and records a point only once the aircraft is verifiably at the requested
coordinate (1 m tolerance). `survey` reports the worst residual, which should be
`0.0 m`.

Measured on Prepar3D v5 (dev box, 2026-09), flattened airfield:

| setting | result |
|---------|--------|
| `--slew on` | writes land; reading repeats to 0.05 m at any `--settle-ms` from 25 up; agrees with SRTM to ~1.3 m mean |
| `--slew off` | writes are mostly not applied; the number still changes from point to point, but it belongs to wherever the aircraft had drifted to — up to ~8 m along a survey row |

So `--slew on` is the default: in slew the aircraft is held at the written
coordinate, so the ground under it *is* that cell's ground by construction.
`--slew off` stays available for a sim where slew freezes the ground model
instead. `probe` is how you find out which sim you have — run it on the machine
that will actually do the bake, since it is a property of the sim, not the tool.

That measurement is about the *baker's* case, where the aircraft is moved to a
new coordinate and asked about that one point. It does not contradict the
runtime behaviour the IG sees — Prepar3D freezing the ground model while the
bridge holds an aircraft in slew — which is why CigiStarter carries its own
database rather than trusting a live HOT. Both are worth confirming on the IG
PC: `probe` there settles the baker's half.

### Cost

With the built-in site boxes (±0.10°, ~22 km across) at the default 3
arc-seconds (~90 m), the four fields are about **230 000 points** — S07E106
(172 800) plus S07E107 (57 600), the only two tiles involved.

The pace is one FSUIPC round trip per point plus `--settle-ms`, so about
**9–10 points/s** at the default, i.e. several hours for the full sweep, and
proportionally less if `--repeat` is raised. `--settle-ms` is the wait between
*position checks* rather than a fixed delay, so it can be short without risking
the wrong cell; `probe` defaults to a much longer 400 ms because it teleports
tens of kilometres, into scenery that has not loaded yet. The sweep is resumable,
so a long bake can be split across sessions, and `--limit N` caps a run for a
smoke test.

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

- The sweep moves the aircraft under slew, so when it finishes the sim is
  wherever the last sample was, at `--altitude`, with slew switched back off.
  Land it or reload the scenario.
- The tool writes `<out>\<tile>.samples.csv` alongside the tiles. That file is
  the bake — keep it, and re-running `survey` resumes from it. `--no-resume`
  starts a tile over. `survey` creates `<out>` if needed and stops with a clear
  error if it cannot write: a survey that cannot record its samples must not look
  like it succeeded.
- Only the two `.hgt` shapes `TerrainDb` accepts are written (1 or 3
  arc-second), so a baked tile is a drop-in replacement. Keep the original SRTM
  tiles somewhere for a re-bake.
- The bake reflects the **installed scenery** at the time it runs. Change an
  airport's add-on scenery and that area wants a re-bake.
