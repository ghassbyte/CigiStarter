# PdkSimLink

An add-on DLL for Prepar3D that **renders the HOST's entities inside the sim**,
from the poses CigiStarter publishes in shared memory.

## Why

The HOST owns the whole world. Every frame it sends an Entity Control per entity
-- roll/pitch/yaw and lat/lon/alt -- and expects the IG to show exactly that.
CigiStarter has always received that stream, but until now it only read entity
0's position out of it, to anchor its HAT/HOT answers; nothing ever moved on
screen.

The vendor program that used to put the world on screen drove the sim's **own**
aircraft over FSUIPC. That is a fight with the sim: the flight model keeps
simulating between writes, the sim's own aircraft is never where the HOST thinks
it is, and everything the sim owns -- time, weather, effects -- is left to be
reconciled by hand. It also froze Prepar3D (slew and pause) to keep the two from
diverging.

This DLL takes the other route. It runs **in-process**, on Prepar3D's own frame
thread, and moves a sim object to the pose the HOST commanded. The IG never
touches the sim: it only fills a table.

## How the two halves talk

`PdkSimLink.dll` runs **inside** Prepar3D. CigiStarter runs **outside** it. They
share one block of memory, described in `../../SimLinkIpc.h`:

```
   CigiStarter (IG)                      PdkSimLink.dll (in Prepar3D)
        |                                        |
        |  writes entities[i]  ------------>      |  on each frame tick:
        |    pose: lat/lon/alt + roll/pitch/yaw   |    creates a sim object for an
        |    (from a CIGI Entity Control)         |    entity it has not seen before
        |                                         |    and moves it to the pose
        |  <----- status words                   |
        |    alive, ticks, applied, diag          |
```

* The link is **one-way**. The CIGI stream already carries everything the sim
  needs; there is no answer to send, so there is no response table, and nothing
  crosses the wire back to the HOST.
* Each row is guarded by its own **seqlock** (`Entity::seq`), so a pose being
  rewritten underneath the add-on is never applied half-old and half-new.
* The layout is pinned in `SimLinkIpc.h` -- an explicit `pack(8)` plus a
  `static_assert` on every size and offset. Fixed-width fields are not enough on
  their own: two binaries built at different packings would agree on the first
  row and silently misread every row after it. A layout change therefore has to
  bump `kVersion` too, so a stale DLL is refused at attach instead of misread.
* Units are CIGI's (degrees, metres) in the table. The DLL converts at the sim
  boundary -- the PDK wants radians and feet -- so nothing sim-shaped reaches the
  contract and nothing metric reaches the sim.

## Object lifetime

The order of the two calls that end an object's life is not a matter of taste:

```
retire():   id = base->GetId();   forget base;   base->Release();   RemoveObject(id);
```

`RemoveObject()` takes the object out of the world, and the manager's
`RegisterOnObjectRemoveCallback` fires "just prior to destruction"
(`ISimObject.h`) -- so what the id names is already going away, and an interface
held across that call points into something the sim is tearing down. The PDK's
own `WorldObject` sample never holds an interface to the object it removes at
all; it works entirely by id. This build holds one only because it moves the
object through `IBaseObjectV400::SetPosition`, and it gives that reference up
*before* it asks for the removal. The slot is cleared before either call for the
same reason: a removal callback that found a live slot would release an object
that is already being destroyed.

Getting the order the other way round does not fail at the call. It corrupts the
simulator's heap and Prepar3D dies seconds later in `ucrtbase.dll`
(`0xc0000409`) -- which is how it was found. `tools\test-simlink-e2e.ps1`
created the objects, never saw `objectsRemoved` move, and the sim was gone a
minute later; the counter sits *after* the release, so a crashed release is
exactly what "created 2, removed 0" looks like. Reversing the order made the same
run report `created 2, removed 2` with no crash.

The removal callback also clears the slot of any entity object the sim took out
on its own -- the world culling it, or the user deleting it -- because the next
tick would otherwise `SetPosition()` through a freed object.

## First cut: what this build is and is not

This is the **spike**: create a sim object with the sim's own object manager
(`ISimObjectManagerV540::CreateObject`) and move it each frame with
`IBaseObjectV520/V400::SetPosition`. It proves the link end to end. What it is
not yet:

* **The native object still has its own simulation.** A container title is a
  normal sim object, so between the add-on's writes its flight model runs. This
  is the thing the follow-up (a custom `ISimObject` with no flight model of its
  own) removes; nothing about the shared-memory contract changes when it does.
* **Pose only.** Time of day, weather and effects are the next things the same
  channel will carry. The table has room and `kVersion` guards a half-updated
  pair.
* **The camera is a separate decision.** The created objects are non-user
  objects, so the sim's cockpit view is still attached to whatever the user
  aircraft is. Putting the camera in a HOST entity -- if the operator wants to
  fly the entities in cockpit view rather than watch them -- is done with the
  same add-on (`IControllableCamera::ActivateEntityTracking`) and does not change
  the contract. The per-channel angles of the immersive dome stay in each PC's
  own `viewgroup.xml`.

## Configuration: `PdkSimLink.ini`

The DLL reads `PdkSimLink.ini` from its own folder:

```ini
[simlink]
title=<a Prepar3D container title>
```

There is no container title that exists in every installation, so it has to be
given. `title` is the string Prepar3D's own Vehicle Select shows -- an object's
`title=` from its `aircraft.cfg` / `sim.cfg`. Until it is set the add-on is
loaded and reports itself (the operator window says
`PdkSimLink.ini has no title to create from`), but creates nothing.

Use a simple, cheap object while bringing the link up, then change it once the
entities move acceptably.

## Building

Needs the Prepar3D PDK headers (v5 or v6); a copy of `inc\PDK` from the v6 SDK
ships in `..\..\external\Prepar3D-SDK\inc\PDK\`, and that is what the project
defaults to -- the add-on links only system libraries, so a full SDK
installation is not needed.

The object-manager interface grew a method between the SDKs and the versions do
not share a GUID (`V520` on v5, `V540` on v6), so the project takes the interface
from the SDK path -- `v5 SDK` in it selects the v5 one:

```
# for Prepar3D v6, against the vendored v6 headers (the default)
msbuild PdkSimLink.vcxproj /p:Configuration=Release /p:Platform=x64

# for Prepar3D v5, against the v5 SDK
msbuild PdkSimLink.vcxproj /p:Configuration=Release /p:Platform=x64 ^
        /p:P3DSdkDir="C:\Program Files\Lockheed Martin\Prepar3D v5 SDK 5.4.9.28482"
```

Each lands in its own folder, named for the simulator, so both can be built and
kept side by side:

| Built for | Headers | Output |
|---|---|---|
| Prepar3D v6 | the vendored copy, or a v6 SDK via `/p:P3DSdkDir` | `bin\Release-v6\PdkSimLink.dll` |
| Prepar3D v5 | a v5 SDK via `/p:P3DSdkDir` | `bin\Release-v5\PdkSimLink.dll` |

Deploy the one that matches the Prepar3D on the machine. Both are called
`PdkSimLink.dll`, because `add-on.xml` names it that way.

Note: this project is deliberately **not** in `CigiStarter.sln`. It needs the
sim's SDK, and a machine that only builds the IG should not have its build broken
by that. Build it where the SDK is, and ferry the DLL.

## Installing on the IG PC

1. Put the DLL, `add-on.xml` and `PdkSimLink.ini` in Prepar3D's default add-on
   discovery folder, one level deep:

   ```
   %USERPROFILE%\Documents\Prepar3D v6 Add-ons\PdkSimLink\
   ```

   (use `...\Prepar3D v5 Add-ons\` on v5). The Documents folder is the one
   Prepar3D discovers; the `add-ons` folder beside `add-ons.cfg` under
   `%PROGRAMDATA%\Lockheed Martin\Prepar3D vN\` is **not** scanned. For a
   machine-wide install instead of a per-user one, register the package:

   ```
   Prepar3D.exe "-Configure: Category=Add-on Package, Operation=Add, Path=<the folder>"
   ```

2. Edit `PdkSimLink.ini` and set `title`.

3. Restart Prepar3D. The add-on registers itself; CigiStarter starts feeding it
   as soon as its tick is running.

Check that it loaded:

```
tasklist /m PdkSimLink.dll
```

Prepar3D should be listed. If it is not, look at the enable flag first: Prepar3D
keeps one `ACTIVE` entry per package in
`%APPDATA%\Lockheed Martin\Prepar3D v6\add-ons.cfg` (v5 likewise), its
**Options → Add-ons** dialog shows the same switch, and a package with
`ACTIVE=false` is not loaded at all.

No elevation is needed, and unlike the fan-out it needs no Npcap.

## Behaviour when it is not there

Everything degrades quietly, so an IG PC without the add-on still runs the link:

* CigiStarter opens the shared region lazily and re-tries every 2 s, so the two
  may be started in either order.
* No add-on, or no frame tick yet, or a tick that has stopped (the sim exited
  and left a mapping behind) → the window's `SimLink` line says so and CigiStarter
  carries on. `--no-sim-link` leaves the sim alone entirely.
* The add-on needs a **running** sim: a paused Prepar3D is not rendering anyway,
  and unlike the terrain probe there is no fallback thread of the add-on's own.
  This is deliberate -- the writer has nothing to do while the sim is frozen.

## The diagnostics

The status words and the `diag` word in `SimLinkIpc.h` exist so the operator can
tell *why* nothing is moving, not just that it is not:

| Word | Meaning |
|---|---|
| `alive` | the add-on has its object manager |
| `ticks` / `frameTicks` | heartbeat, and prepar3D's frame callbacks seen |
| `applied` | entities positioned on the add-on's last tick |
| `objectsCreated` / `objectsRemoved` | objects made for entities, and taken back out |
| `createFails` / `setFails` | creates the sim refused, and moves it refused |
| `diag` bit 24 | the sim-object manager was obtained |
| `diag` bit 25 | a tick has been delivered |
| `diag` bit 26 | `PdkSimLink.ini` has no title |
| `diag` bit 27 | the newest pass came from the add-on's own thread (unused here) |

CigiStarter's window shows the summary on its `SimLink` line, and says which of
the above it is when the writer is off.

## Testing without Prepar3D

`test\PdkSimLinkTest` compiles the code either side of the contract and drives
it without the sim: CigiStarter's half (`SimLink.cpp`) and this add-on's
PDK-free half (`SimLinkMap.h` -- the very functions the DLL calls, not a copy of
them). It needs no Prepar3D and no SDK.

It covers the mistakes that would otherwise be flown by the HOST: that entity 0
-- the HOST's own aircraft -- is rendered rather than skipped for being id 0;
that an Active (1) entity is rendered, which is the state the reference HOST
really sends, while a disabled row, an all-zero pose, a Standby (0) or a
Remove/Destroyed (2) entity all mean "take the object back out"; that the pose
arrives in the PDK's units and order (degrees to radians, metres to feet, yaw in
the heading slot); that a row caught mid-rewrite is never believed, read flat
out with the IG rewriting underneath; and that a publisher with no add-on loaded
does nothing, while a region left behind by a sim whose tick has stopped is not
believed.

```
msbuild test\PdkSimLinkTest.vcxproj /p:Configuration=Release /p:Platform=x64
test\x64\Release\PdkSimLinkTest.exe
```

`tools\test-simlink.ps1` does both steps and exits 0 or 1, so it is the loop to
run while editing; `tools\test-all.ps1` runs this harness and the terrain
probe's together. From the repository root:

```
.\tools\test-simlink.ps1
.\tools\test-all.ps1
```

What it cannot cover is Prepar3D's side, which is the next section.

## Testing with Prepar3D

`tools\test-simlink-e2e.ps1` is the other half, and the one that says whether the
writer works at all. It needs no HOST and no emulator: the `.bin` captures
beside this repository *are* the real HOST's downstream stream, so it replays one
into a CigiStarter started on the bench ports, then reads the add-on's status
words and entity rows out of the running sim and checks them.

```
.\tools\test-simlink-e2e.ps1                    # hlm.bin, 1x, PASS/FAIL
.\tools\test-simlink-e2e.ps1 -Speed 20 -KeepRunning
```

It fails, before starting anything, on the two things that would make a run
meaningless: a Prepar3D whose loaded `PdkSimLink.dll` is not the build in this
tree (the add-on is loaded once, at sim startup, so a stale copy tests stale
code), and a `title=` that is not in that installation. Then it proves, in the
sim:

* the object manager and the sim's own frame event are what drives the add-on,
  and the frame event is advancing -- so the sim is running, not paused;
* an object is created for each entity the HOST drives, and every one of them is
  positioned on the tick the test reads;
* no create and no `SetPosition` call failed;
* the entity rows hold the pose the capture carried, row by row and double by
  double, which proves CigiStarter's publish path end to end and not just
  compiled;
* the aircraft's pose changes over the run rather than being written once;
* stopping the IG retires every row and the add-on takes its objects back out
  (`2 created, 2 removed`), so a stopped IG does not leave containers parked in
  the sim -- and, because the removal path is where the sim's heap was once
  corrupted, that the sim is *still up* afterwards is part of the check rather
  than a courtesy;
* nothing on the way through leaves the simulator dead: the run ends with
  Prepar3D still running and no new dump in `%LOCALAPPDATA%\CrashDumps`.

This is the test that found the object-lifetime bug under *Object lifetime*
above. The first run it ever made created its objects and carried them on the
capture's poses perfectly, and failed on the way out -- which is the only kind of
failure that a contract harness, running without the sim, can never see.

The objects it creates come from the container title in `PdkSimLink.ini`, so keep
that set to something small while bringing the link up. What it still cannot say
is whether the sim *holds* the pose between writes: the counters say
`SetPosition` succeeded, not that the object stayed there. The PDK's
`IBaseObjectV400::GetPosition` exists, and reading the object's own pose back
into the region is the next thing this test should check; until then that part
remains the rig's question.

## What to check on the rig

The bench answers the link; these need the sim, the HOST and the flight model:

* how the native flight model behaves between writes -- the drift the read-back
  above is meant to measure;
* that the sign and order of roll/pitch/yaw are right -- a level aircraft proves
  nothing, so roll it;
* how many entities the HOST actually drives, and whether the entities that are
  not the aircraft should exist as objects at all;
* whether the camera should follow a HOST entity, and if so which one.
