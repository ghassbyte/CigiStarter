# Terrain bake runbook

Step-by-step procedure for a real **option 2** bake: filling CigiStarter's
HAT/HOT terrain database with Prepar3D's own ground heights over an SRTM base.
Run it **on the IG PC**.

Design, options and rationale: `README.md` in this folder.
The two ways to fill the database at all: the terrain section of the repository
`README.md`.

---

## 0. Scope and shape

- Output: `<exeDir>\terrain-sim\<tile>.hgt`, the same `.hgt` format `TerrainDb`
  already reads — a drop-in replacement for the SRTM tiles.
- Coverage: only the two tiles this deployment needs, `S07E106` (172,800 points)
  and `S07E107` (57,600 points) — **230,400 points** total.
- Fields in the default boxes (±0.10°): Atang Senjaya, Halim Perdanakusuma,
  Soekarno-Hatta, Husein Sastranegara. Astina sits inside the Atang box.
- Cost: **well under a minute** of sampling (256 points per simulation tick at
  60 ticks/s; measured at ~9 400 points/s against a stand-in sim). The aircraft
  is never moved, so there is nothing to land afterwards and nothing else needs
  to be closed.

Option 1 (plain SRTM) needs none of this — it is just fetching and copying
`.hgt` tiles (`tools\get_terrain.ps1`).

---

## 1. Preconditions

Do not start until all of these hold.

1. **Files ferried to the IG PC**, next to where the exe will run:
   - `TerrainBaker.exe` — statically linked to the same CRT as the IG, so there
     is no runtime to install, and not linked against any SDK.
   - The SRTM base in `<exeDir>\terrain\` — either
     - 1″ set: `S06E106, S07E106, S07E107, S08E107` (4 × 25.9 MB), or
     - 90 m set: the same four at 1201² (4 × 2.9 MB).

     Only `S07E106` and `S07E107` are actually consulted.
2. **Prepar3D up and running.** The answers ride the simulation tick, so a paused
   sim, a menu, or a loading screen can stop them; `probe` says so rather than
   returning an empty bake.
3. **The `PdkTerrainProbe` add-on installed and loaded.** This is the same
   add-on CigiStarter answers HAT/HOT from. If CigiStarter's status block shows
   the sim as its terrain source, it is loaded; the direct check is
   `tasklist /m PdkTerrainProbe.dll`. Install procedure:
   `tools\PdkTerrainProbe\README.md`.
4. Nothing else is required to be closed. The bake reads terrain out of the sim
   and never writes to it, so the vendor program, `CigiP3dBridge` and
   CigiStarter may all keep running.

`--help`, `plan` and `compose` need no simulator at all.

---

## 2. Step 1 — `plan` (no sim, seconds)

```powershell
.\x64\Release\TerrainBaker.exe plan
```

Expected:

```
plan: 3 arc-sec (1201x1201, ~93 m)
base: <exeDir>\terrain
out : <exeDir>\terrain-sim

  S07E106  base ok              points 172800  already    0  todo 172800
  S07E107  base ok              points  57600  already    0  todo  57600

total points to sample: 230400
through the probe: up to 15360 samples/s (60 ticks/s x 256 slots), so about 0.2 min,
plus whatever the surface query costs the sim per tick.
```

Check: both tiles say `base ok`. If a tile does not say `base ok`, the `.hgt`
files are not where the exe expects them.

---

## 3. Step 2 — `probe` (seconds; the gate)

```powershell
.\x64\Release\TerrainBaker.exe probe
```

This is the gate. It asks the sim about the centre and the corners of every box,
prints sim against SRTM, then asks about the same points a second time. Read
**three** things:

| line | must show | meaning |
|---|---|---|
| `sim  NNN m` | a number, not `no surface` | the add-on is loaded, ticking and answering |
| the `d` column | usually nonzero | the sim really does disagree with SRTM where it matters |
| `worst repeat \|d\|` | `0.00 m` | the answer is tied to the point asked, not invented |
| final `VERDICT` | `answering per point and repeating` | safe to survey |

A good run looks like this in shape (numbers from a stand-in sim, not a real
field — the point is the columns, not the values):

```
probe: surface queries through PdkTerrainProbe.dll, asked from 3500 m

   -6.54280 106.75660   sim 178 m   srtm 175 m   d 3
   -6.63280 106.66660   sim 141 m   srtm 143 m   d -2
   ...

5 of 5 point(s) answered; 5 distinct height(s); worst repeat |d| 0.00 m
VERDICT: the sim is answering per point and repeating; sample against
         it is sound. Where SRTM and the sim differ, the bake moves the ground to the sim.
```

If it exits non-zero (2):

- *"PdkTerrainProbe.dll is not loaded in Prepar3D"* → the add-on is not
  installed for this Prepar3D version, or Prepar3D has not been restarted since
  it was. See `tools\PdkTerrainProbe\README.md`.
- *"the probe is loaded but has no surface service"* → the DLL is in the sim but
  could not pick up its service; the add-on writes the `HRESULT` it got to
  `lastError` in the shared region for diagnosis.
- *"the probe is loaded but its simulation tick is not running"* → the sim is
  paused, or sitting on a loading screen.

**Record the probe output** with the bake (screenshot or pasted), together with
the `compose` residual. They are the evidence the run was sound, and a property
of the installed scenery, so re-run `probe` if the scenery changes.

---

## 4. Step 3 — smoke test (seconds)

```powershell
.\x64\Release\TerrainBaker.exe survey --limit 2560
```

Check the tail:

```
[S07E106] 172800 points in the boxes, 0 already sampled
  ...
[S07E106] wrote 2560 new sample(s) to <exeDir>\terrain-sim\S07E106.samples.csv

limit reached -- survey done (2560 samples this run). Now run: TerrainBaker compose
```

`limit reached` (exit 0) means go. `--limit 2560` is ten full batches, so a pass
also proves the multi-batch path — a `--limit` below 256 only ever exercises one.

Then throw the smoke samples away before the real run, or keep them: `survey`
resumes, so they will simply not be sampled again. They were taken from the same
grid, so they are as good as any.

---

## 5. Step 4 — `survey` (the short one)

```powershell
.\x64\Release\TerrainBaker.exe survey
```

- 256 points per simulation tick, 60 ticks/s → the full sweep is **well under a
  minute** (~9400 points/s measured against a stand-in sim), and the rate line
  prints the pace actually achieved.
- **Ctrl+C is safe** — it stops between batches and keeps the CSV.
- Re-running **resumes**: it skips everything already in
  `terrain-sim\S07E106.samples.csv`.
- `--no-resume` starts the tile over — do not use it casually.
- Points the sim has no surface for (open water, no data) are **not recorded**,
  so those cells keep their SRTM height; the summary line counts them.
- Keep the CSV afterwards. **That file is the bake**; the tile can always be
  rebuilt from it without the sim (that is what `compose` does).

Ends with (exit 0):

```
[S07E106] wrote 172800 new sample(s) to <exeDir>\terrain-sim\S07E106.samples.csv
[S07E107] wrote 57600 new sample(s) to <exeDir>\terrain-sim\S07E107.samples.csv

survey done (230400 samples this run). Now run: TerrainBaker compose
```

If it prints `FAILED --` and exits 1, the `--out` folder could not be written, or
the sim stopped answering mid-run — the message says which. `interrupted --` is
a Ctrl+C; re-run to continue.

---

## 6. Step 5 — `compose` (no sim, seconds)

```powershell
.\x64\Release\TerrainBaker.exe compose
```

Expected:

```
S07E106: 172800 sample(s) applied -> <exeDir>\terrain-sim\S07E106.hgt
   sim vs SRTM: mean -1.6 m, |max| 18 m over 172800 sampled cell(s)
   coverage: 100.00% of the tile is sim-exact
S07E107: 57600 sample(s) applied -> ...
```

Read the residual: a few metres mean over a **flattened field** is expected and
is the whole point of the bake — that is the runway levelling SRTM cannot know
about. Large values over open country would mean the terrain being answered was
not the terrain under those points.

Re-run `compose` after any extra survey; new samples do not reach the tile by
themselves.

---

## 7. Step 6 — deploy

```powershell
copy <exeDir>\terrain-sim\*.hgt <exeDir>\terrain\
```

Then restart CigiStarter so it indexes the new tiles (they are read at startup),
and confirm in the operator window's `TerrainDB` line that they were picked up.

Sanity check against CigiStarter's own lookup at the fields: Atang ~175 m,
Halim ~25 m, Soekarno-Hatta ~8 m, Husein ~745 m. (Those are the *SRTM* heights;
the baked tile at a flattened field will read a few metres different, which is
the correction.)

Nothing has to be re-flown: the bake never moved the aircraft, and the sim is
left exactly as it was found.

---

## 8. How it samples (and what it replaced)

The answers come from `PdkTerrainProbe.dll`, loaded inside Prepar3D, calling the
PDK surface query service. `survey` keeps all 256 slots of the shared table full
and the add-on answers them on each simulation tick, so the pace is the sim's
tick rate, not a round trip per point.

The query is **stateless and about the world**: it is asked about a latitude and
longitude and answered for the surface at that point, at the altitude passed in
(`--altitude`, default 3500 m — high enough to be above the ground everywhere in
these boxes, so the answer is the surface an aircraft would land on). Nothing is
repositioned, so there is no slew mode, no settling, and no way for a reading to
"belong to" the previous cell.

It replaced an FSUIPC survey that walked the grid one cell at a time, writing the
aircraft's position over FSUIPC and reading `0x0020` back. That needed slew mode
to make the writes land, had to read its own position back on every sample to be
sure the reading was for the requested cell, and took hours. Measured on the dev
box (Prepar3D v5, 2026-09): with slew on the readings repeated to 0.05 m at any
settle from 25 ms up; with slew off the position writes were mostly not applied
at all and a reading could describe a cell up to ~8 m away while still looking
plausible. The tool now avoids the whole question, and so does CigiStarter's own
live answer.

Known-good reference from the retired FSUIPC dev run, for comparison when
reading `compose`: `sim vs SRTM: mean -1.62 m, |max| 18 m` over 60 samples.

---

## 9. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `PdkTerrainProbe.dll is not loaded in Prepar3D` | add-on not installed for this Prepar3D version, or the sim has not been restarted since it was |
| `the probe is loaded but has no surface service` | the DLL could not get the service; check `lastError` in the shared region |
| `the probe is loaded but its simulation tick is not running` | Prepar3D is paused, on a menu, or still loading |
| `the sim stopped answering` mid-run | Prepar3D was paused, closed or hung; re-run and it resumes |
| `cannot append to ...` + `FAILED`, exit 1 | `--out` (or its parent) not writable, or is a file |
| probe says `no surface` for every point | scenery not loaded yet, or the sim is at a different location |
| probe says `repeat queries disagree` | answers are not tied to the points asked; do not bake with this |
| probe prints SRTM but no `sim` | the add-on answered nothing — see the first three rows |
| tile is a surprising size | `--res`: 3 → 2,884,802 B, 1 → 25,934,402 B |
| `survey` finishes suspiciously fast | it is supposed to: the sweep is seconds, not hours |

Widen a field with `--box lat0,lon0,lat1,lon1` if the HOST's approaches start
further out. Cost grows with the square of the box, so re-run `plan` first.

---

## 10. For the record

- **Re-bake when the scenery changes.** The bake is a snapshot of the installed
  terrain at the time it ran; change an airport's add-on scenery and that area
  wants redoing. An IG PC with the add-on loaded does not have this problem at
  all: it asks the same question live, and the bake is only for a PC that cannot
  run the add-on. The sample CSVs are still worth keeping — `compose` needs no sim.
- **Ferry the smaller base if that matters.** 1″ (104 MB) or 90 m (11.5 MB) are
  equivalent as bake inputs; only the copy differs. The deployment's plain
  option is staged at 1″ with the 90 m set beside it.
- **Not covered yet:** the oil rig and the military ship. Land was done first;
  those two are still to come.
