# TODO

What is still open, in the order it should be picked up. `README.md` is the
manual, `agents.md` is the network view, and `tools\*\README.md` is the detail on
each tool — this file is only the work that is not finished.

Last updated 2026-09-29, branch `feature/pdk-simlink`. Items 1 (the read-back)
and 2 (the physics-free object) are done and measured; item 1's terrain figures
were re-measured on 2026-09-29 and corrected (see 1).

---

## Where it stands

The entity writer **works end to end on this PC**. A recorded HOST capture
replayed into a bench CigiStarter makes the running Prepar3D create one sim
object per driven entity and move it to the pose the HOST sent: 28/28 checks,
`created 2, removed 2`, sim still alive afterwards.

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
so nothing is drawn yet (see 2); and the aircraft's own rotor and lights are still
unsolved (see 3 and 4).

---

## 1. Prove the sim is *holding* the pose — **done, 2026-09-28**

`SetPosition` returning `S_OK` says the call was accepted, not that the object is
still there a frame later. The add-on now reads each object's pose back
(`IBaseObjectV400::GetPosition`) before it overwrites it, publishes it as a
`Held` row in `SimLinkIpc.h` (**`kVersion` is now 2**) and reports the gap to the
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
| FSUIPC | already vendored, but a de-facto 32-bit-era channel and one more moving part |
| The PDK | no `L:` write from the PDK either, as far as we have found |
| A small bridging gauge in the sim | a shared-memory → `L:switch ...` writer beside `PdkSimLink`; cleanest, but a new component |

**Blocked on two things:** your call on the path, and **a capture of the real
HOST link that actually contains a light command** — every capture we have shows
the HOST never sends one.

## 4. Rotor: find out who steps the RPM

`ROTOR RPM PCT:1` is the only *writable* rotor variable (`ROTOR ROTATION ANGLE:1`,
`DISK PITCH/BANK PCT:1` are read-only), and the model has exactly three rotor
looks: `rotor_still` under 40, `rotor_slow` 40–45, `rotor_blurred` above 45.

* Run `tools\rotor-probe.ps1 watch 40` **on the rig** during an IOS engine start.
* A staircase trace means something is writing discrete steps; a smooth S-curve
  means the stepping is the stock three-stage crossfade and is inherent to the
  model.

**Done when:** we know whether the stepped rotor is ours to fix.

## 5. Time of day, weather, visibility, effects

Pose only, today. The same shared-memory channel carries the rest.

* Time of day: **SimConnect has no setter**, so this is PDK `SetTime` or FSUIPC.
* Weather/visibility: `IWeatherSystemV500`, plus the warm-up Atmosphere Control
  messages the captures already carry.
* Open unknown: **nothing found yet that sets wave height** —
  `IWeatherSystemV500` has no sea-state setter and `SurfaceInfoV400::m_fWaveHeight`
  is read-only.

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
  Fixed-origin camera definition and applies that entity's pose each frame, and
  the sim reads the camera back on the commanded pose (`PdkSimLink/README.md`,
  *The camera*). The view's own aircraft is created from `own_ship_title` — a
  model-less container — so the eye is not inside a drawn model; measured
  2026-09-29 against an entities' container that had the AS332's body junctioned
  into it, where the same parked pose showed the inside of a fuselage before that
  and the airfield after. What is left is the dome — the per-channel angles in
  each PC's `viewgroup.xml` are only provable on the rig — and deciding whether
  CIGI View Control should place the eye too. The captures' View Control is one
  static definition repeated in every frame (`X 2.8`, `Y 0.05`, `Z -0.5`,
  roll/pitch 0, yaw 358, attached to entity 0), so it could be honoured — today
  the eye sits on the Entity Control row's own reference point, which for the
  AS332 is mid-cabin rather than at the pilot's eye.
* **Converter gaps.** Component Control (id 4) is not field-mapped; `DATE` is left
  untranslated because the captures' date fields are invalid.
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
* The sign and order of roll/pitch/yaw — a level aircraft proves nothing, so roll
  it.
* That `PAUSE_ON_LOST_FOCUS=0` on every IG PC. An unattended IG that pauses when
  it loses focus would freeze the display. (It is `0` on this bench.)
* Retiring the vendor program that holds the deployed sim paused.

---

## Open questions for you

* **Item 3: which light path?** (FSUIPC / PDK / a bridging gauge.) Nothing has
  been built for this yet.
* Do the entities that are *not* the aircraft need to exist as objects at all?
  (The camera is answered: it rides entity 0 unless `camera=` names another.)
* **Where the eye sits in the aircraft.** The camera is placed on the entity's
  own reference point — CIGI's Entity Control position — which for the AS332 is
  mid-cabin, not the pilot's eye. The HOST also sends a View Control every frame
  with an eyepoint offset from the entity (`X 2.8`, `Y 0.05`, `Z -0.5`, level,
  yaw 358) and it is not applied. Honouring it is the protocol-correct thing to
  do, and it is what would put the eye in the cockpit rather than the cabin — but
  it is only visible at all if the own ship is drawn, and today the entity the
  view rides is deliberately model-less so that the picture is the world. Say the
  word if you want the own aircraft's airframe framed around the view instead.

---

## The local loop

```powershell
# build the two things that matter
& "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe" `
    "tools\PdkSimLink\PdkSimLink.vcxproj" /p:Configuration=Release /p:Platform=x64
& "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe" `
    "CigiStarter.vcxproj" /p:Configuration=Release /p:Platform=x64

# the SDK-free contract harnesses (writer + terrain probe + converter)
.\tools\test-all.ps1

# the only test that reaches inside the sim -- needs Prepar3D running
.\tools\test-simlink-e2e.ps1
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
| **The add-on owns the shared region** | CigiStarter only opens it, and its counters persist across IG runs — compare **deltas**, never absolutes |
| **Bench ports are 8010/8011** | Prepar3D holds 8000 for its own CIGI |
| **The emulator's `.scp` loads only through its GUI** | So the e2e test replays a `.bin` directly instead |
| **Entity Control layout** | state bits in byte 4, roll@12, pitch@16, yaw@20, lat@24, lon@32, alt@40 |

Anything that changes the shared-memory layout must bump `SimLinkIpc.h`'s
`kVersion`, so a stale counterpart is refused at attach rather than misread.
