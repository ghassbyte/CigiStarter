# TODO

What is still open, in the order it should be picked up. `README.md` is the
manual, `agents.md` is the network view, and `tools\*\README.md` is the detail on
each tool — this file is only the work that is not finished.

Last updated 2026-09-30, branch `feature/pdk-simlink`. Items 1 (the read-back)
and 2 (the physics-free object) are done and measured; item 4 (the rotor) is
answered from `rotor.bin` (see 4), which also shows the HOST's rotor command is
one CigiStarter does not carry yet; item 1's terrain figures were re-measured on
2026-09-29 and corrected (see 1). The camera is done too
(item 7): the sim's view is flown from the HOST's own eyepoint, so it is a
cockpit view rather than a view from the airframe's reference point — 2.8 m
forward, 0.5 m up, 5 cm right and a degree left of entity 0, one definition in
all 16 captures (yaw 359; earlier notes read it as two degrees, which the bytes
do not say) — and the bank sign is settled by looking, the bench's banked pair
leaning into its turn both ways. The **FSUIPC path is gone** (2026-09-29): it
read the local sim's own aircraft, which nothing flies any more now that the
HOST's entities are separate objects, so terrain is the sim's own probe and
then the SRTM database, and the operator window no longer has the frozen `Sim`
line.

---

## Where it stands

The entity writer **works end to end on this PC**. A recorded HOST capture
replayed into a bench CigiStarter makes the running Prepar3D create one sim
object per driven entity and move it to the pose the HOST sent: every check
passes (44 of them on the last run), `created 2, removed 2`, sim still alive
afterwards.

**Items 1 and 2 are answered** (2026-09-28):

* against a stock title (the AS332) the sim holds a written pose to a few
  centimetres in the air and does not fight the writer; it **refuses a pose far
  below the ground**, holding the object above it (item 1, see 1 below).
  `gnd-fly-bank.bin` never commands below the ground, so it shows no refusal and
  ~0.2 m of drift, not the 277 m once reported;
* against the **physics-free object** (`CigiStarter.Entity`) the pose is held
  exactly — 0.00 m / 0.00° — an altitude below the sim's terrain is where the
  command puts it, and the sim is never paused or in slew (item 2, see 2 below).

What is **not** done: nothing has been on the rig; the objects ship with no model,
so nothing is drawn yet (see 2); and the aircraft's own lights are still unsolved
(see 3). The rotor is answered — item 4 — and it turns out to be the IG that does
not carry it, not the model that steps it. The HOST's world beyond the aircraft
(item 5) is written and proved on the bench — its clock, its air and, from the
`cloud pat*.bin` captures, its cloud pattern, each confirmed in the sim's own
weather system — while effects (lights, smoke, damage) are still to come.

---

## 1. Prove the sim is *holding* the pose — **done, 2026-09-28**

`SetPosition` returning `S_OK` says the call was accepted, not that the object is
still there a frame later. The add-on now reads each object's pose back
(`IBaseObjectV400::GetPosition`) before it overwrites it, publishes it as a
`Held` row in `SimLinkIpc.h` (**`kVersion` went to 2 for this; it is 5 now**,
adding the view at 3, the world at 4 and the sim's own sky read-back at 5) and
reports the gap to the
pose it last wrote as `driftMaxMeters` / `driftMaxDeg`. `test-simlink-e2e.ps1`
checks the read-back is being taken and reports the drift.

Measured on the bench with `title=AS-332 Bond Helicopters`, feeding the capture's
own positions:

| Command | Result |
|---|---|
| a fixed pose at 1000 m | held exactly, 0.02 m, for as long as it is fed and after |
| a 30 m/s climb, 1000 → 1300 m | held exactly, 0.02 m, the whole way |
| roll 30°, pitch 10°, yaw 180/359/270° | held to <0.1°, wrap included |
| **a pose far below the sim's ground** | **refused — the object is held above it** |

The last row is the finding that mattered, and it was re-measured on 2026-09-29
because the numbers first written here did not hold up. What the bench actually
shows:

* **The capture never triggers it.** `gnd-fly-bank.bin` commands entity 0 between
  178.87 m and 913.3 m and entity 3 at −0.34 m. P3D's own terrain under entity 0
  is 174.3 m and under entity 3 is 0.0 m (water), and over entity 0's whole flight
  track it stays below 175.5 m — so entity 0 is always a few metres *above* the
  ground and entity 3 only 0.34 m under the water surface, which the stock object
  followed. Replaying the **whole** capture, worst drift was **0.19 m / 0.24°**:
  no refusal, and none of the 277 m / 211 m once reported.
* **The refusal is real, but it is not the surface.** Publishing a below-ground
  pose directly through the SimLink region, the stock object refuses a command
  well below the ground: over land (terrain 174.3 m) commands of 50–160 m were
  held at **176.6 m**, and over water (0.0 m) commands of −10 to −50 m were held
  at **+26.7 m**. The hold altitude is a property of the *place*, not the terrain
  height (moving one object to the other's position moved the hold with it), and
  a command only a little below the ground is followed (land 168 m → 168.2 m;
  water −7 m → −6.8 m).

The terrain figures first recorded here — 456.4 m and 210.9 m — were wrong: they
are not the terrain under the two entities. (Terrain at entity 0's *latitude*
with entity 3's *longitude*, ~14 km east, is 467.9 m, which is likely where the
reading was taken.) The HOST-vs-P3D datum gap at these points is a few metres
(the HOST flies entity 0 at 178.87 m where P3D's ground is 174.3 m), nowhere near
the 277 m the notes attributed to a refusal.

**Consequence:** the native flight model does not drift a pose it is given, so
item 2 is no longer forced by drift. It is still wanted because a stock object
refuses a pose far below the ground where a physics-free one is placed anywhere,
and for the transients a fast replay shows when a write has not been applied
before the next read-back.

## 2. Replace the stock object with a custom `ISimObject` (no flight model) — **done, 2026-09-28**

The plan of record was a follow-up; the rotor work had made it required, and item
1 added a second reason (the ground clamp) while removing the first (drift).

`tools\PdkSimLink\CigiEntity.h` is that object: an `ISimObjectV400` that registers
**no simulation** and has no `[contact_points]`. The DLL registers it with the sim
at load (`RegisterSimulationCategory`), and the shipped container
`SimObjects\CigiEntity\sim.cfg` carries the same GUID as its `[General]
CategoryId`, which is what routes `CreateObject("CigiStarter.Entity")` there. A
stock title never matches the GUID and goes on to the sim's own object exactly as
before, so the stock path is untouched and still testable for comparison.

Nothing in the shared-memory contract changed; the only additions are two `diag`
bits — 29 (`kPhysicsFree`, asked of each object's own category with
`GetCategoryId`, not assumed from the title) and 30 (`kModePauseOrSlew`).

Measured on the bench with `title=CigiStarter.Entity`, replaying
`gnd-fly-bank.bin`:

| Command | Result |
|---|---|
| every pose on the capture | held **exactly**: 0.00 m / 0.00° over 4330 comparisons |
| entity 0 commanded 178.9 m (terrain under it 174.3 m) | held at 178.9 m |
| entity 3 commanded −0.3 m (water surface 0.0 m) | held at −0.3 m — **below the surface** |
| paused or slewed? | never — `diag` bit 30 stayed clear |

The e2e test now **asserts** zero drift when `kPhysicsFree` is set; for a stock
object it is still only reported, because there a large number is a finding about
the sim's own object rather than a fault in the link.

**Left for item 7:** the container ships with no `model\` folder, so the object is
not drawn. The bench container gives it the AS332's body by junctioning the payware
`Model\` and `Texture.red\` into the deployed folder (and `texture=red`); the repo
keeps the container model-free because *which* model each entity type shows is
item 7's job, not the link's.

## 3. Decide the light-command path — **a decision, not a task**

The HOST must command the aircraft's exterior lights, not the crew. That runs
into the model: the AS332's lighting is owned by its own panel, whose
`systems.xml` servos a few `L:switch ...` variables and **reverts everything
else** — the standard light variables and bare K-events included. SimConnect
**cannot write `L:` variables**, so the write has to be one of:

| Path | Cost |
|------|------|
| FSUIPC | was vendored; removed with the rest of the HAT/HOT FSUIPC path (2026-09-29), so re-adding it is a new moving part, and a de-facto 32-bit-era channel at that |
| The PDK | no `L:` write from the PDK either, as far as we have found |
| A small bridging gauge in the sim | a shared-memory → `L:switch ...` writer beside `PdkSimLink`; cleanest, but a new component |

**Blocked on two things:** your call on the path, and **a capture of the real
HOST link that actually contains a light command** — every capture we have shows
the HOST never sends one.

## 4. Rotor: find out who steps the RPM — **answered, 2026-09-30**

`ROTOR RPM PCT:1` is the only *writable* rotor variable (`ROTOR ROTATION ANGLE:1`,
`DISK PITCH/BANK PCT:1` are read-only), and the model has exactly three rotor
looks: `rotor_still` under 40, `rotor_slow` 40–45, `rotor_blurred` above 45.

**The HOST steps it, and it does not step it in steps.** `rotor.bin` — a capture
of an engine start from idle — carries the rotor as two ordinary CIGI messages
that CigiStarter defines and decodes *neither* of:

* **Rate Control (id 8)**, entity 0, articulated part 15, flags `0x01`
  (apply-to-artpart), in its `YawRate` slot: the rotor's speed. Flat at
  **0.6749** for the first ~34 s (an idle the model's own three looks call
  `still`), then a smooth **S-curve** over ~34 → ~94 s up to a plateau of
  **~4.27**, held with ±0.005 of dither to the end of the capture. Every frame is
  a fresh float (~`6.6e-5` apart; raw `1.775007 → 1.7772305 → 1.7794522`): no
  staircase, no rungs, no 1 %/5 % jumps.
* **Articulated Part Control (id 6)**, entity 0, part 1, flags `0x31`
  (ArtPart|Roll|Pitch): the rotor disk — roll `0.6066 → −0.7167`, pitch
  `−18.4579 → −10.2661`, a fast transient at ~36–38 s as it comes up and a small
  second step at ~85–87 s. The disk tilts with the spool.

The reading is revolutions per second: 0.675 ≈ 40 rpm (≈15 %) and 4.27–4.59 ≈
256–275 rpm, the AS332's nominal main-rotor speed. The same slot is all-zero in
`tod_day.bin`, ramps `0 → 0.673` in `reposition_hlm.bin`, and sits at 4.3–4.6 in
the flying captures.

**So the stepped rotor is not ours to fix.** The stepping is the model's own
three-stage crossfade over one continuous percentage, and the wire changes that
percentage smoothly — it crosses the 40/45 band in a ~2 s window of the spool.

**What *is* ours: the link does not exist yet.** `cigi.h` and `main.cpp`'s
message switch cover ids 1, 2, 4, 9, 10, 12, 16 and 24 and drop **6 and 8** into
`default:`, so nothing in CigiStarter writes `ROTOR RPM PCT:1` from the HOST; on
this IG the rotor is the sim's own aircraft, not the HOST's. Carrying it is a
pair of decoders and a `ROTOR RPM PCT:1` write, the same shape as the world
(item 5).

**Still owed the rig:** `tools\rotor-probe.ps1 watch 40` during an IOS engine
start, which is what shows what the sim's own rotor does with the commanded
percentage — the capture proves only the command.

**Done when:** we know whether the stepped rotor is ours to fix. It is not; the
missing command is.

## 5. Time of day, weather, visibility, effects — **the world is done, effects are left (2026-09-29)**

**Done and contract-tested SDK-free** (2026-09-29). The same shared-memory
channel now carries the HOST's world beyond the entities — its clock, its air
and its cloud — as an `Environment` block appended after `view` (**`kVersion`
is now 5**), with the add-on's `EnvironmentStatus` answer beside it.
CigiStarter accumulates the CIGI messages into one world
(`Environment.h`), folds the operator's preset on top, and publishes only when
something changed — so a commanded clock is set once, not re-pinned every
frame.

* **Time of day** (Celestial Sphere Control, id 9) through PDK
  `IGlobalData::SetTime`/`SetDate`, **local**: the four `*_replay.bin` time
  captures read as the four local times (06:15 / 12:00 / 17:30 / 23:30) and the
  site is UTC+7, so Zulu would put the "day" capture on the wrong side of
  sunset. Operator presets: `Host` / Dawn / Day / Dusk / Night. SimConnect has
  no setter, so this is PDK or nothing.
* **Weather and visibility** (Atmosphere Control, id 10, and Weather Control,
  id 12) through `IWeatherSystemV500`: a synthesised METAR (`GLOB` station,
  cloud and precipitation) applied first, then the exact metric values through
  the global setters, which win wherever the METAR rounds. Operator presets:
  `Host` / Clear / Cloudy / Rain / Storm. The captured warm-up Atmosphere
  Control is the air the HOST flies; the Weather Control names precipitation
  with its `LayerID`, not the severity (`CigiBaseWeatherCtrl.h`). Air and sky
  are applied separately: the METAR only when a sky is present — a METAR always
  names a sky, so an atmosphere-only change would clear the sim's cloud nobody
  commanded — and the air setters only when the HOST has actually sent the air,
  so an operator's preset chosen before warm-up cannot push the resolved row's
  place-holder temperature and pressure into the sim. A world with a sky and no
  air keeps a "no restriction" visibility (200 km) rather than the zero a fresh
  row starts at: the visibility group belongs to the sky and is emitted with it,
  so the zero would have reached the sim as `0000` and fogged the picture in —
  measured, and the reason the line is there.
* **The cloud pattern** (Component Control, id 4) — the reference HOST's *other*
  way of describing a sky, and the only thing in the three `cloud pat*.bin`
  captures: a component `0x1001` on the entity `0x9001` it also creates, class
  Entity, whose state is the pattern's index and whose first three data floats
  are the pattern's footprint and altitude, **all in metres** (14816 = 8 × 1852
  across, 12964 = 7 × 1852 deep, 914.4 = 3000 ft up). The ICD's own class for
  weather (Global Layered Weather, 9) is *not* what the vendor sends, so the
  decode matches on the component id and the Entity class the captures carry.
  Prepar3D has cloud layers, not cloud volumes at a place, so a pattern resolves
  to a layer: base = data 3, coverage = the footprint's area in square nautical
  miles at 1 %/nmi² floored at 30 % and capped at 100 %, cumulus, ~900 m thick,
  no precipitation; state 0 is the HOST taking the cloud away and resolves to an
  explicit clear sky, or a group that is merely absent would leave the sim
  holding the last pattern. **The vendor's own mapping is not in the capture**
  and could only come from the tool that wrote it, so these choices are this
  IG's and are written down as an assumption rather than a decode. Precedent for
  what the sky becomes: pat1 56 % broken at 3000 ft, pat2 30 % scattered at
  3500 ft, pat3 100 % overcast at 3000 ft.
* **The sim's own answer** (`kVersion` 5). `EnvironmentStatus` carries the sky
  the *sim* holds, read back every tick from its weather system — the `GLOB`
  station's cloud layers through `IWeatherStationV430` and the observation it
  parsed through `GetStationMetarData` — so "the cloud reached the sim" is
  answered by the sim rather than by the region the IG wrote. The operator
  window has a `Sky` line for it and says `HOST cloud pattern N` when that is
  where the sky came from.
* **Presence gate.** No group is applied until something has spoken for it. The
  real HOST (AGENTS.md) sends an Atmosphere Control and **no** clock and no
  weather, so without the gate the 00:00 and clear-sky defaults would pin the
  sim's clock to midnight and clear its sky on the first frame. The gate holds
  on both sides: the publisher sets an apply bit only for a present-and-changed
  group, and the add-on checks presence again before it touches the sim.
* **No waves.** Nothing was found that sets sea state — `IWeatherSystemV500`
  has no sea-state setter and `SurfaceInfoV400::m_fWaveHeight` is read-only —
  so the wave path is deliberately absent.

Proven where it can be: `tools\PdkSimLink\test` decodes the messages from their
offsets, folds and resolves the world, pins the METAR strings (clear, rain and
storm, the air-less form before warm-up, and a pattern's sky alone), pins each
pattern's coverage and base, and drives the publisher's generation, per-group
apply bits and the presence gate. The bench, 2026-09-29 — `tools\test-simlink-e2e.ps1`
replays `cloud pat1/2/3.bin` into a bench CigiStarter and reads the sim back:
each pattern arrives in the region with its own footprint and altitude and is
applied generation N, and the sim's own station then holds `5CU030` / `3CU035` /
`8CU030` — three visibly different skies, the pattern's base to the metre — and
`CLR` again after the state-0 message. That also settles the rig question of
whether `WEATHER_MODE_GLOBAL` + `GLOB` renders what it reads: on this build the
observation lands in the station the sim renders from. Still owed the rig:
Zulu-vs-local, and the per-angle weather rendering under the dome. No capture
anywhere carries a Weather Control, so the id-12 path stays contract-tested and
preset-driven; the cloud-pattern path is the one a real capture exercises.

## 6. Clients (IG 2–6)

IG 1 is master and renders channel 1; IG 2–6 are receive-only and need only
`PdkSimLink`, no terrain probe. Needs a receive-only mode and a test that the
fan-out leaves them looking exactly like the HOST's own stream.

## 7. Smaller, in any order

* **Per-entity-type titles.** The HOST sends CIGI EntityType 1 for entity 0 and 2
  for entity 3; one `title` currently covers every entity that is *drawn*. The
  one exception is the entity the camera rides, which has its own container
  (`own_ship_title`) because it must not be drawn at all. What is left is a map
  from EntityType to title for the visible entities, which is content, not link.
* **Camera.** Done on the bench: `camera=<entity id>` puts every 3D view onto a
  Fixed-origin camera definition and applies the HOST's own eye for that entity
  each frame, and the sim reads the camera back on the commanded pose
  (`PdkSimLink/README.md`, *The camera*). The eye is the HOST's View Control
  (`X 2.8`, `Y 0.05`, `Z -0.5`, level, yaw 359, attached to entity 0 in every
  capture), carried across on the shared region (`view`, `kVersion` 3) and
  rotated out of the entity's body frame by `SimLinkMap.h`'s `viewEyePose()`, so
  the picture is drawn from the pilot's seat rather than from the entity's
  reference point — mid-cabin on the AS332. With no View Control, or one attached
  to another entity, the camera stays on the reference point as before. The
  view's own aircraft is created from `own_ship_title` — a model-less container —
  so the eye is not inside a drawn model; measured 2026-09-29 against an
  entity's container that had the AS332's body junctioned into it, where the
  same parked pose showed the inside of a fuselage before that and the airfield
  after. What is left is the dome — the per-channel angles in each PC's
  `viewgroup.xml` are only provable on the rig — and whether the airframe should
  be drawn around the eye after all, which is a content decision and a one-line
  change (`own_ship_title=`).
* **A CigiStarter that is killed, not exited, leaves its objects in the sim.**
  The rows stay enabled and nothing in the region says the IG is gone — the
  heartbeat there is the add-on's, not the IG's — so the last pose of every entity
  stays on screen until Prepar3D is restarted. Only the EXIT button retires the
  rows. The contract has room for an IG-side heartbeat (`kVersion` guards it), and
  the add-on taking a stale one as "retire everything" is the honest fix; on the
  bench it shows up as a second e2e run finding the objects already there.
* **Wire the contract tests into the build** (optional; today they are scripts,
  deliberately not in `CigiStarter.sln`).

## 8. Rig confirmations still owed

Everything the local harness cannot reach:

* `CreateObject` / `SetPosition` / **`GetPosition`** on v6 **Professional**, and
  `GetLicenseType()` — the writer is only proven on v6 Personal.
* The **order** of the **View Control**'s three angles. The Entity Control's own
  sign and order are settled: its yaw matches the capture's own ground track, so
  `+roll` is a right bank, which is why the sim link negates the bank for
  Prepar3D's opposite axis (`SimLinkMap.h`'s `toPdkPose()`). The picture agrees —
  with the negation in and the banked pair replayed on the bench (2026-09-29),
  the aircraft leans into its turn both ways. The view's three angles have the
  same doubt and less to go on: every capture carries one non-zero value (359, a
  degree) in the slots the Entity Control calls roll/pitch/yaw, and it is read
  that way here. A level aircraft cannot tell a 1° bank from a 1° turn, and the
  ICD's own View Control table may list them yaw-first.
* That `PAUSE_ON_LOST_FOCUS=0` on every IG PC. An unattended IG that pauses when
  it loses focus would freeze the display. (It is `0` on this bench.)
* **An entity-relative HAT/HOT answer with the HOST's own Entity Control as its
  anchor** now has no automated check. The emulator bench cannot produce that
  anchor — the Host Emulator is not a flight model and sends no Entity Control —
  so `tools\test-hathot-emulator.ps1` asserts the opposite rule there: no anchor
  means `Valid = 0`, never an answer from another aircraft. The anchored case
  wants a wire check built on a replayed capture (dumpcap plus
  `tools\capture-hat-hot.ps1`, the way the emulator test reads the answers) or a
  sighting on the rig, where the failure it guards against — the ~550 ft step at
  the destination when the anchor was left behind at the departure field — was
  first seen.
* Whether a **cloud pattern** reads as the right sky on the rig. This IG
  resolves a pattern to one layer over the whole world — the mapping is an
  assumption, item 5 — while the vendor's tool wrote a footprint and a *place*
  (the cloud entity's own pose), so if its cloud stood only over that entity the
  difference would show there and nowhere else. It wants a look at the rig's
  own cloud presets rather than another capture.
* Retiring the vendor program that holds the deployed sim paused.

---

## Open questions for you

* **Item 3: which light path?** (FSUIPC / PDK / a bridging gauge.) Nothing has
  been built for this yet.
* Do the entities that are *not* the aircraft need to exist as objects at all?
  (The camera is answered: it rides entity 0 unless `camera=` names another.)
* **Should the airframe be drawn around the eye?** Answered for now: no. The
  camera flies the HOST's View Control eyepoint (2.8 m forward of entity 0, half
  a metre up, 5 cm right, turned a degree — the pilot's seat), and the entity
  the eye is inside is created from the model-less `own_ship_title`, so the
  picture is the HOST's world and not a modelled cabin drawn around it. Giving
  the own ship a model is a one-line change (`own_ship_title=`) and a content
  decision: the eye is *inside* the airframe, so a cabin model only helps if it
  is the right cabin for that aircraft type.
* **Item 5: is the cloud pattern's mapping close enough?** A pattern is a
  footprint and an altitude; it becomes cloud with the base taken from the
  pattern and the coverage from its footprint (item 5 list, the three
  `cloud pat*.bin` captures). The three captures now give three different skies
  in the sim, but the vendor's own mapping is not in the capture, so a rig look
  is what would settle whether the *amount* is right — or a note from whoever
  wrote the vendor's tool.

---

## The local loop

```powershell
# build the two things that matter
& "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe" `
    "tools\PdkSimLink\PdkSimLink.vcxproj" /p:Configuration=Release /p:Platform=x64
& "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe" `
    "CigiStarter.vcxproj" /p:Configuration=Release /p:Platform=x64

# the SDK-free contract harnesses (writer + terrain probe)
.\tools\test-all.ps1

# the only test that reaches inside the sim -- needs Prepar3D running
.\tools\test-simlink-e2e.ps1

# the same replay by hand, for watching the sortie rather than checking it
.\tools\play-bin.ps1
```

Deploying a new writer build (the add-on is loaded once, at sim startup). The
DLL, `add-on.xml` and the `SimObjects\` container all go together now; the title
lives in the deployed `PdkSimLink.ini`:

```powershell
$dst = "$env:USERPROFILE\Documents\Prepar3D v6 Add-ons\PdkSimLink"
Copy-Item "tools\PdkSimLink\bin\Release-v6\PdkSimLink.dll" $dst -Force
Copy-Item "tools\PdkSimLink\add-on.xml"                    $dst -Force
Copy-Item "tools\PdkSimLink\SimObjects"                    $dst -Recurse -Force
# set title=CigiStarter.Entity in $dst\PdkSimLink.ini, then start Prepar3D
```

The e2e test fails up front if the loaded DLL is not this tree's build, so a
stale copy cannot quietly pass.

---

## Traps that have already cost time

Read these before changing anything near them.

| Trap | What happens |
|------|--------------|
| **Entity state numbering** | CIGI is `Standby=0, Active=1, Remove/Destroyed=2`. The HOST marks the entities it drives **Active (1)**. Test the field the other way round and the writer renders **nothing at all** while every pose still arrives perfectly — which is why it was caught against the captures, never on the rig |
| **Object lifetime in the writer** | `retire()` must take the id, forget the pointer, `Release()`, and *then* `RemoveObject(id)`. The other order corrupts the sim's heap and Prepar3D dies ~a minute later in `ucrtbase.dll` (`0xc0000409`), reading as `created 2, removed 0` |
| **A loaded DLL cannot be replaced** | Deploy, then restart Prepar3D. The e2e test's hash check enforces it |
| **Pause and slew kill the rotor** | Proven on the bench. A live rotor needs an unpaused, non-slew sim |
| **A stock object refuses a pose below its ground** | A stock sim object is not placed far below the terrain under it: the pose is refused and the object is held above it. Measured directly, the hold altitude is a property of the *place*, not the surface — 176.6 m over 174.3 m terrain, and +26.7 m over water at 0.0 m — and a command only a little below the ground is followed. It is not what earlier notes called a "277 m clamp on `gnd-fly-bank.bin`": that capture never commands below the ground and shows ~0.2 m of drift. The read-back reports any refusal; compare the commanded altitude before blaming the object. The physics-free object (item 2) is placed below it — that is what it is for |
| **A container's `CategoryId` must match the DLL's GUID** | `CigiEntity.h` registers one GUID and `SimObjects\CigiEntity\sim.cfg` names it as `[General] CategoryId`. Ship the two out of step and every `CreateObject()` fails: the object is never ours, and the e2e test shows it as create failures rather than as a wrong pose |
| **SimConnect cannot write `L:` vars** | The AS332's lights live in `L:switch ...` and the panel's own servo reverts everything else |
| **Only `ROTOR RPM PCT:1` is writable** | The other rotor variables are read-only |
| **The HOST's rotor arrives as ids 6 and 8** | Rate Control (id 8, entity 0, artpart 15, `0x01`, `YawRate` slot) is the rotor speed in rev/s and Articulated Part Control (id 6, entity 0, part 1, `0x31`) is the disk tilt — ordinary per-frame traffic that looks like everything else, except `cigi.h` defines neither id, so both land in `main.cpp`'s `default:` and are dropped without a counter or a word in the log. `rotor.bin` is the capture that shows them. Item 4 |
| **The add-on owns the shared region** | CigiStarter only opens it, and its counters persist across IG runs — compare **deltas**, never absolutes |
| **Bench ports are 8010/8011** | Prepar3D holds 8000 for its own CIGI |
| **The view is a camera, not an object** | Moving an entity object does not move the picture: the sim's view belongs to a window's **active camera**, and the objects this writer creates are non-user objects. The eye has to be moved with `SetLLA`/`SetPBH` on a Fixed-origin definition, per window, and the entity the eye is inside must not be drawn or the picture is the inside of its hull |
| **A CIGI body offset is not a lat/lon offset** | The View Control's `X`/`Y`/`Z` are body offsets (+X nose, +Y right wing, +Z down), so they have to be rotated by the entity's attitude before they become a position. Skip the rotation and the eye is right on a north heading and wrong on every other one — which reads as a camera that is subtly behind, not as a bug |
| **Entity Control layout** | state bits in byte 4, roll@12, pitch@16, yaw@20, lat@24, lon@32, alt@40 |
| **A wrong bank *sign* still reads back exactly** | `GetPosition`/`GetPBH` hand back the number they were given, so a sign error round-trips perfectly and no read-back can show it. CIGI's `+roll` is a right bank (the captures' ground track says so) and Prepar3D's bank axis is the opposite, so the writer negates it — `SimLinkMap.h`'s `toPdkPose()`/`toCigiPose()`, and the camera's `SetPBH` so the horizon and the models agree. It reads as "the aircraft moves right but banks left", and only looking settles it |

Anything that changes the shared-memory layout must bump `SimLinkIpc.h`'s
`kVersion`, so a stale counterpart is refused at attach rather than misread.
