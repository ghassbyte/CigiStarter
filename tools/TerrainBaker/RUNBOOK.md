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
- Cost: **~6–7 hours** at the measured ~9–10 points/s. Resumable across
  sessions; Ctrl+C is safe.

Option 1 (plain SRTM) needs none of this — it is just fetching and copying
`.hgt` tiles (`tools\get_terrain.ps1`).

---

## 1. Preconditions

Do not start until all of these hold.

1. **Files ferried to the IG PC**, next to where the exe will run:
   - `TerrainBaker.exe` — statically linked to the same CRT as the IG, so there
     is no runtime to install.
   - The SRTM base in `<exeDir>\terrain\` — either
     - 1″ set: `S06E106, S07E106, S07E107, S08E107` (4 × 25.9 MB), or
     - 90 m set: the same four at 1201² (4 × 2.9 MB).

     Only `S07E106` and `S07E107` are actually consulted.
2. **Prepar3D v5 up**, aircraft flying normally.
3. **Pause on task switch = OFF** (Options → General). A console that steals
   focus otherwise pauses the sim mid-survey.
4. **Nothing else driving the aircraft.** Close the vendor program, close
   `CigiP3dBridge`, and **close CigiStarter**. The IG PC's Prepar3D is
   configured as a CIGI IG (it receives on 8001), so a live CigiStarter means
   the HOST's Entity Control is flying the aircraft while the baker repositions
   it. They will fight, and the bake will be wrong.
5. `FSUIPC6.dll` loaded by the sim — true if the IG runs normally.

`--help`, `plan` and `compose` need no simulator.

---

## 2. Step 1 — `plan` (no sim, seconds)

```powershell
.\x64\Release\TerrainBaker.exe plan
```

Expected:

```
plan: 3 arc-sec (1201x1201, ~93 m), 60 ms settle, slew ON
base: <exeDir>\terrain
out : <exeDir>\terrain-sim

  S07E106  base ok              points 172800  already    0  todo 172800
  S07E107  base ok              points  57600  already    0  todo  57600

total points to sample: 230400
```

Check: both tiles say `base ok`, and the setting line says `slew ON`. If a tile
does not say `base ok`, the `.hgt` files are not where the exe expects them.

---

## 3. Step 2 — `probe` (1 minute; decides the setting)

```powershell
.\x64\Release\TerrainBaker.exe probe
```

This is the gate. It samples the centre and corners of every box, then revisits
three of the points. Read **four** things:

| line | must show | meaning |
|---|---|---|
| `pos  X.X m` on each point | `0.0 m` | the aircraft really is at the requested coordinate |
| the `d` on each `revisit` line | `0.00 m` | readings repeat at fixed coordinates |
| the `worst d` in the summary | `0.00 m` | same check, worst case |
| final `VERDICT` | `tracked ... and readings repeat` | safe to survey |

A good run looks like:

```
   -6.26640 106.89130   sim    25.60 m   pos  0.0 m   srtm 24 m   d 2
   -6.35640 106.80130   sim    68.29 m   pos  0.0 m   srtm 72 m   d -4
  revisit  -6.26640 106.89130   first    25.60 m   again    25.60 m   d 0.00 m

4 distinct sim values across 4 points; 3 revisited, worst |d| 0.00 m
VERDICT: the ground model tracked the reposition and readings repeat at fixed coordinates.
```

If it exits non-zero:

- *"did NOT follow the reposition (frozen)"* → try the other `--slew` setting.
- *"readings do not repeat at fixed coordinates"* → the position writes are not
  taking effect. Try `--slew off`, but see §8: on the dev box slew off was
  **worse**, not better.

**Record the probe output** with the bake (screenshot or pasted). It is the
evidence that the run was sound, and it is a property of the sim, not the tool —
so it is worth re-running if the sim is ever rebuilt or its scenery changes.

---

## 4. Step 3 — smoke test (5 minutes)

```powershell
.\x64\Release\TerrainBaker.exe survey --limit 60
```

Check the tail:

```
[S07E106] wrote 60 new sample(s) to <exeDir>\terrain-sim\S07E106.samples.csv

position: worst residual 0.0 m over 60 sample(s)

limit reached -- survey done (60 samples this run). Now run: TerrainBaker compose
```

`worst residual 0.0 m` plus `limit reached` (exit 0) means go. Only then commit
the hours.

---

## 5. Step 4 — `survey` (the long one)

```powershell
.\x64\Release\TerrainBaker.exe survey
```

- ~9–10 points/s → **6–7 h**. Rate and ETA print every 25 samples.
- **Ctrl+C is safe** — it stops after the sample in flight and keeps the CSV.
- Re-running **resumes**: it skips everything already in
  `terrain-sim\S07E106.samples.csv`. Splitting across sessions is expected and
  fine.
- `--no-resume` starts the tile over — do not use it casually.
- Keep the CSV afterwards. **That file is the bake**; the tile can always be
  rebuilt from it without the sim (that is what `compose` does).

Ends with (exit 0):

```
position: worst residual 0.0 m over 230400 sample(s)

survey done (230400 samples this run). Now run: TerrainBaker compose
```

If it prints `FAILED --` and exits 1, the `--out` folder could not be written —
fix that, not the sim. `interrupted --` is a Ctrl+C; re-run to continue.

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
about. Large values over open country would mean the survey was not tracking.

Re-run `compose` after any extra survey; new samples do not reach the tile by
themselves.

---

## 7. Step 6 — deploy

```powershell
copy <exeDir>\terrain-sim\*.hgt <exeDir>\terrain\
```

Then:

1. Reload the scenario — the aircraft is parked at the last sampled point at
   3500 m (the tool switches slew back off when it finishes).
2. Bring CigiStarter back up.
3. Confirm in the operator window's `TerrainDB` line that the tiles indexed.

Sanity check against CigiStarter's own lookup at the fields: Atang ~175 m,
Halim ~25 m, Soekarno-Hatta ~8 m, Husein ~745 m.

---

## 8. Why the settings are what they are (measured, dev box P3D v5)

These numbers are why `--slew on` is the default and why every sample is
position-checked. They are the reason a `--settle-ms`-only run cannot be trusted.

| setting | result over 60 adjacent cells |
|---|---|
| slew **on**, 25 / 60 / 100 / 250 ms | max **0.05 m** from a pinned 500 ms reference — settle-insensitive |
| slew **off**, 60 ms | max **8.08 m**, with runs of 8 identical cells |
| no position check | readings can belong to the previous cell, and look plausible while doing it |

- In slew the aircraft is held at the written coordinate, so `0x0020` is that
  cell's ground by construction. With slew off the sim was not applying the
  position writes at all — the number still changed from point to point, but it
  described wherever the aircraft had drifted to.
- `samplePoint` therefore reads its own lat/lon back in the same FSUIPC round
  trip and records a point only within 1 m of the requested coordinate. The
  `worst residual` line is that check.
- `--settle-ms` is the wait **between position checks**, not a fixed delay.
  Survey default 60 ms; probe default 400 ms, because it teleports tens of
  kilometres into scenery that has not loaded yet.
- Open item, worth a glance on the IG PC: at a verified-identical coordinate the
  sim's reported ground moved a few metres **between sessions** (repeatable
  within one session). That is Prepar3D's mesh reporting rather than the tool,
  and it is inside the ~1.3 m mean sim-vs-SRTM agreement anyway.
- This is the *baker's* case — move to a new coordinate, ask about that one
  point. It does not contradict the runtime behaviour the IG sees, where
  Prepar3D freezes the ground model while the bridge holds an aircraft in slew.
  That is why CigiStarter carries its own database instead of trusting a live
  HOT.

Known-good reference from the dev run: `sim vs SRTM: mean -1.62 m, |max| 18 m`
over 60 samples, with 0 cell mismatches on `compose`.

---

## 9. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `FSUIPC_Open failed` | P3D not running, or FSUIPC6 not loaded |
| `cannot append to ...` + `FAILED`, exit 1 | `--out` (or its parent) not writable, or is a file |
| `the aircraft never reached the requested position` | sim paused (task-switch), or something else is driving the aircraft |
| probe says *frozen* | the sim froze its ground model; try the other `--slew` setting |
| probe says *does not repeat* | position writes not applied; try the other `--slew` setting |
| survey rate far below ~9/s | `--repeat` raised, or the sim is struggling; lower the settle |
| tile is a surprising size | `--res`: 3 → 2,884,802 B, 1 → 25,934,402 B |
| aircraft ends up somewhere odd | expected — it is left at the last sample, at 3500 m |

Widen a field with `--box lat0,lon0,lat1,lon1` if the HOST's approaches start
further out. Cost grows with the square of the box, so re-run `plan` first.

---

## 10. For the record

- **Re-bake when the scenery changes.** The bake is a snapshot of the installed
  terrain at the time it ran; change an airport's add-on scenery and that area
  wants redoing. This is also why the sample CSVs are worth keeping — a re-bake
  can be split across sessions, and `compose` needs no sim.
- **Ferry the smaller base if that matters.** 1″ (104 MB) or 90 m (11.5 MB) are
  equivalent as bake inputs; only the copy differs. The deployment's plain
  option is staged at 1″ with the 90 m set beside it.
- **Not covered yet:** the oil rig and the military ship. Land was done first;
  those two are still to come.
