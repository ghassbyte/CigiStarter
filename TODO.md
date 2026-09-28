# TODO

What is still open, in the order it should be picked up. `README.md` is the
manual, `agents.md` is the network view, and `tools\*\README.md` is the detail on
each tool — this file is only the work that is not finished.

Last updated 2026-09-28, branch `feature/pdk-simlink` (both branches are still at
`d32d796`; **nothing from the entity-writer work is committed yet**).

---

## Where it stands

The entity writer **works end to end on this PC**. A recorded HOST capture
replayed into a bench CigiStarter makes the running Prepar3D create one sim
object per driven entity and move it to the pose the HOST sent: 22/22 checks,
`created 2, removed 2`, sim still alive afterwards.

What is **not** done: nothing has been on the rig, the object is still a stock
sim object with its own flight model, and the aircraft's own rotor and lights are
still unsolved (see 2 and 3).

---

## 1. Prove the sim is *holding* the pose — do this first

`SetPosition` returning `S_OK` says the call was accepted, not that the object is
still there a frame later. A stock object runs its own flight model between our
writes, which is the whole reason the rig's old bridge used pause or slew.

* Add `IBaseObjectV400::GetPosition` to the writer and publish the read-back pose
  into the shared region. **This bumps `SimLinkIpc.h`'s `kVersion`.**
* Then the e2e test can assert the sim agrees with what we wrote, and measure how
  far it drifts between ticks on a moving capture.

**Done when:** `tools\test-simlink-e2e.ps1` compares written vs read-back pose and
reports the drift, on a capture where the aircraft is actually moving
(`gnd-fly-bank.bin` is 194 s of ground run and bank).

**This measurement decides the next decision:** how much the native flight model
fights the writer. If it drifts, item 2 is not optional.

## 2. Replace the stock object with a custom `ISimObject` (no flight model)

The plan of record was a follow-up; the rotor work has made it required.

* A container title keeps its own physics. A custom `ISimObject` in the
  `SimpleHelicopter` pattern has none, so the pose we write is the pose that
  stays.
* **A rotor can only turn in an unpaused, non-slew sim** (proven on the bench:
  pause freezes the rotor clock; slew freezes it too while the clock runs). So
  the pose has to be owned *without* pausing or slewing — which is exactly what
  a physics-free object gives.
* Nothing in the shared-memory contract changes when this lands; it is a new
  object type plus a title/`ISimObject` switch in `PdkSimLink.ini`.

**Done when:** the e2e test runs with the custom object, `GetPosition` matches
with no drift, and the sim is never paused or in slew.

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
  for entity 3; one title currently covers every entity.
* **Camera.** One owner only: per-channel angles stay in each PC's
  `viewgroup.xml`; CIGI View Control is mapped to a camera only if the HOST
  actually moves views.
* **Converter gaps.** Component Control (id 4) is not field-mapped; `DATE` is left
  untranslated because the captures' date fields are invalid.
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
* Do the entities that are *not* the aircraft need to exist as objects at all,
  and which entity should the camera follow, if any?

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

Deploying a new writer build (the add-on is loaded once, at sim startup):

```powershell
Copy-Item "tools\PdkSimLink\bin\Release-v6\PdkSimLink.dll" `
    "$env:USERPROFILE\Documents\Prepar3D v6 Add-ons\PdkSimLink\PdkSimLink.dll" -Force
# then start Prepar3D
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
| **SimConnect cannot write `L:` vars** | The AS332's lights live in `L:switch ...` and the panel's own servo reverts everything else |
| **Only `ROTOR RPM PCT:1` is writable** | The other rotor variables are read-only |
| **The add-on owns the shared region** | CigiStarter only opens it, and its counters persist across IG runs — compare **deltas**, never absolutes |
| **Bench ports are 8010/8011** | Prepar3D holds 8000 for its own CIGI |
| **The emulator's `.scp` loads only through its GUI** | So the e2e test replays a `.bin` directly instead |
| **Entity Control layout** | state bits in byte 4, roll@12, pitch@16, yaw@20, lat@24, lon@32, alt@40 |

Anything that changes the shared-memory layout must bump `SimLinkIpc.h`'s
`kVersion`, so a stale counterpart is refused at attach rather than misread.
