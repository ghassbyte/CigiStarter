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
asked for, so a whole tile is sampled in seconds.

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

`plan`, `survey` and `compose` take the same `--site` / `--box` / `--res`
options, so the survey and the compose agree on the area. `--tile` selects tiles
for `compose` explicitly.

## Running a bake

Do it on the **IG PC**, with Prepar3D up and the `PdkTerrainProbe` add-on loaded.
The add-on answers from Prepar3D's own ticks while the sim is running and from a
thread of its own while the sim is paused, so a bake works through a pause too;
`probe` (below) proves it before you start. Nothing else has to be held still:
the bake reads the world rather than steering the aircraft, so the vendor program
or `CigiP3dBridge` may be running, and the aircraft is never touched.

The IG PC needs `TerrainBaker.exe` and the SRTM base in `<exeDir>\terrain\` —
either the 1″ set or the 90 m set. A bake of the default boxes consults only
`S07E106` and `S07E107`; `plan` says when a tile's base is missing.

```powershell
# 0. the add-on, once per IG PC (see tools\PdkTerrainProbe\README.md)
$addons = "$env:USERPROFILE\Documents\Prepar3D v6 Add-ons\PdkTerrainProbe"
mkdir $addons
copy tools\PdkTerrainProbe\bin\Release-v6\PdkTerrainProbe.dll $addons
copy tools\PdkTerrainProbe\add-on.xml                      $addons
#    ...and restart Prepar3D.

# 1. the SRTM base (option 1)
.\tools\get_terrain.ps1 -Points "-6.54,106.76","-6.27,106.89","-6.13,106.66","-6.90,107.58" `
                        -Out x64\Release\terrain

# 2. what would be baked -- both tiles must say "base ok"
.\x64\Release\TerrainBaker.exe plan

# 3. prove the add-on is answering -- seconds; this is the gate
.\x64\Release\TerrainBaker.exe probe

# 4. the sweep. Ctrl+C is safe: it stops between batches
.\x64\Release\TerrainBaker.exe survey

# 5. assemble and deploy
.\x64\Release\TerrainBaker.exe compose
copy x64\Release\terrain-sim\*.hgt x64\Release\terrain\
```

Restart CigiStarter after deploying so it indexes the new tiles (they are read
at startup), and confirm the `TerrainDB` line in its window picked them up.
CigiStarter's own lookup at the fields — the SRTM heights, before the bake's
correction — should read Atang ~175 m, Halim ~25 m, Soekarno-Hatta ~8 m,
Husein ~745 m.

### `plan`

```
plan: 3 arc-sec (1201x1201, ~93 m)
  S07E106  base ok              points 172800  already    0  todo 172800
  S07E107  base ok              points  57600  already    0  todo  57600
total points to sample: 230400
```

### `probe` — the gate

`probe` asks about the centre and the corners of each box (four boxes × five
points = twenty), far enough apart that one invented number could not pass for
five, prints sim against SRTM, then asks about the same points a second time.
Read **four** things:

| line | must show | meaning |
|---|---|---|
| `sim  NNN m` | a number, not `no surface` | the add-on is loaded, ticking and answering |
| the `d` column | usually nonzero | the sim really does disagree with SRTM where it matters |
| `worst repeat \|d\|` | `0.00 m` | the answer is tied to the point asked, not invented |
| final `VERDICT` | `answering per point and repeating` | safe to survey |

`sim` may carry a bracketed suffix naming the sim's own classification of what
it answered with — `[hard]`, `[water]`, `[soft, platform]` (see `surface::*` in
`../../TerrainProbeIpc.h`). **`platform` is the interesting one**: it means the
surface is an *object's* rather than terrain — a vessel's deck, a rig — where
Prepar3D's collision will rest an aircraft while the terrain under it is still
sea level. No suffix means the add-on that answered did not publish a
classification (a DLL built before it did, or a point the sim refused); it does
not mean "no platform".

It exits non-zero (2) if the add-on answers nothing — scenery not loaded, the DLL
not installed, or no tick reaching the sim — or if the repeat disagrees, in which
case the answers are not tied to the points asked and the bake must not run. The
message names which case it is; `tasklist /m PdkTerrainProbe.dll` is the direct
check that the DLL is loaded. A **paused** Prepar3D is not a failure: Prepar3D's
own ticks stop while it is paused, and the add-on's own thread keeps the table
answering underneath them.

After a successful gate it prints a `probe: creates …, frames …, own thread …,
sim …` line — the add-on's own view of what is driving it, straight from the
shared diagnostic word and tail. `frames` is the count of Prepar3D's per-frame
events (nonzero is the frame tick answering; zero from an old DLL is the object
tick alone), and `own thread yes` means Prepar3D's ticks have stopped and the
add-on is answering from its own thread — what a paused sim looks like. Keep the
line with the run. `tools\PdkTerrainProbe\README.md` has the layout.

**Record the probe output** with the bake (screenshot or pasted), together with
the `compose` residual. They are the evidence the run was sound, and a property
of the installed scenery, so re-run `probe` if the scenery changes.

### `survey` — the sweep

A smoke test first, ten full batches, to exercise the multi-batch path:

```powershell
.\x64\Release\TerrainBaker.exe survey --limit 2560
```

`limit reached` (exit 0) means go, then run the sweep proper. The pace is set by
the sim's tick, not by the tool: the probe answers all 256 slots of its table on
each tick (60/s), so the full sweep is **well under a minute** — measured at
about 9 400 points/s against a stand-in sim, i.e. ~25 s for the 230 400 points.

- **Ctrl+C is safe** — it stops between batches and keeps the CSV.
- Re-running **resumes**: it skips everything already in
  `terrain-sim\<tile>.samples.csv`. `--no-resume` starts the tile over — do not
  use it casually.
- `--limit N` caps a run, for a smoke test.
- Points the sim has no surface for (open water, no data) are **not recorded**,
  so those cells keep their SRTM height; the summary line counts them.
- Keep the CSV afterwards. **That file is the bake**; the tile can always be
  rebuilt from it without the sim (that is what `compose` does).

Ends with `survey done (230400 samples this run). Now run: TerrainBaker compose`
(exit 0). `FAILED --` with exit 1 means the `--out` folder could not be written,
or the sim stopped answering mid-run — the message says which; `interrupted --`
is a Ctrl+C, and a re-run continues.

### `compose` — assemble

```
S07E106: 172800 sample(s) applied -> <exeDir>\terrain-sim\S07E106.hgt
   sim vs SRTM: mean -1.6 m, |max| 18 m over 172800 sampled cell(s)
   coverage: 11.98% of the tile is sim-exact
S07E107: 57600 sample(s) applied -> ...
```

A few metres mean over a **flattened field** is expected and is the point of the
bake — that is the runway levelling SRTM cannot know about; large values over
open country would mean the wrong terrain was answered. `coverage` is the share
of the **whole tile** that came from the sim: the default boxes cover a fraction
of a 1° cell, so a fully sampled run reports about 12% for S07E106 and the rest
keeps its SRTM height. Re-run `compose` after any extra survey; new samples do
not reach the tile by themselves.

### Choosing the resolution

`--res 3` (default) is 3 arc-seconds, ~90 m, a `1201 × 1201` tile of about
2.9 MB — the same shape and cost as an SRTMGL3 download, and the resolution this
deployment asked for. `--res 1` is ~30 m and a `3601 × 3601` tile of about
25 MB; it needs nine times the samples.

A 1 arc-second SRTM base is stepped down to 3 arc-seconds exactly 3:1, so the
bake still builds on option 1's tiles with no second download. Stepping *up* is
refused: it would invent detail the base never had.

### The query is altitude dependent

Prepar3D decides which surface is meant from the height it is asked about, which
is why `--altitude` exists and defaults to 3500 m: high enough to be above the
ground everywhere in these boxes, so the answer is the surface an aircraft would
land on. It is the same field CigiStarter fills in at run time with the HAT
point's altitude, so a baked tile and a live answer describe the same surface.

## Notes and limits

- The bake does not touch the aircraft: no slew mode, no position writes, so the
  sim is left exactly as it was found and nothing has to be re-flown.
- The tool writes `<out>\<tile>.samples.csv` alongside the tiles. That file is
  the bake — keep it, and re-running `survey` resumes from it. `survey` creates
  `<out>` if needed and stops with a clear error if it cannot write: a survey
  that cannot record its samples must not look like it succeeded.
- Only the two `.hgt` shapes `TerrainDb` accepts are written (1 or 3
  arc-second), so a baked tile is a drop-in replacement. Keep the original SRTM
  tiles somewhere for a re-bake.
- The bake reflects the **installed scenery** at the time it runs. Change an
  airport's add-on scenery and that area wants a re-bake. (An IG PC with the
  add-on loaded does not: it answers the same question live, and the bake is only
  for a PC that cannot run the add-on.)
- Widen a field with `--box lat0,lon0,lat1,lon1` if the HOST's approaches start
  further out; the cost rises with the square of the box, so re-run `plan` first.
- **Platforms:** the oil rig and the military ship are the *platform* case — an
  object's surface standing in the sea, which Prepar3D will rest an aircraft on
  while the terrain under it is sea level. `probe` prints the sim's own
  classification per point (`[water]`, `[hard, platform]`). A run over the rig's
  moored vessel settled it: the deck comes back as `[hard, platform]` at its own
  height, where the elevation-only call had answered sea level, so the deck is
  answered by the surface query itself — no table of platforms needed. A point
  answers the deck only from at or just under its height; asked from several
  metres below, it answers the water under the hull.

## Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `PdkTerrainProbe.dll is not loaded in Prepar3D` | add-on not installed for this Prepar3D version, or the sim has not been restarted since it was |
| `the probe is loaded but has no surface service` | the DLL could not get the service; check `lastError` in the shared region |
| `the probe is loaded but no tick is running` | neither Prepar3D's per-frame event nor the object simulation has ever run, and the add-on's own thread has not taken over either — so the whole add-on is not running. Deploy the current `PdkTerrainProbe.dll`; the message carries `frameFails` (`tools\PdkTerrainProbe\README.md`) |
| `the probe's per-frame tick has stopped after N frames` | Prepar3D's frame loop is not running and the add-on's own thread has not taken over — the add-on is not running at all. A load or a modal dialog resumes on its own; if it does not, the sim is hung |
| `the sim stopped answering` mid-run | Prepar3D was closed or hung; re-run after reading the `probe:` line |
| `cannot append to ...` + `FAILED`, exit 1 | `--out` (or its parent) not writable, or is a file |
| probe says `no surface` for every point | scenery not loaded yet, or the sim is at a different location |
| probe says `repeat queries disagree` | answers are not tied to the points asked; do not bake with this |
| a tile does not say `base ok` | the `.hgt` files are not where the exe expects them (`--terrain-dir`) |
| tile is a surprising size | `--res`: 3 → 2,884,802 B, 1 → 25,934,402 B |
| `survey` finishes suspiciously fast | it is supposed to: the sweep is seconds, not hours |
