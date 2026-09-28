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
        |  writes view  --------------------->     |  and puts the window's camera
        |    the HOST's own eye + its angles       |    at that eye, if `camera=` is set
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

## The object: the sim's own, or ours

`CreateObject(title)` hands back whatever the container title describes, and there
are two kinds of title, both created and moved the same way
(`IBaseObjectV400::SetPosition` each frame):

* **A stock title** (any aircraft or object the sim already has). Its own
  simulation runs between the add-on's writes. Measured on the bench (see *The
  read-back*), that simulation does **not** drift a pose it is given — but a pose
  far below the ground is refused and the object is held above it; and holding a
  pose with a stock object is what makes pause or slew tempting, which is what
  freezes a rotor.

* **`CigiStarter.Entity`, the physics-free object** (`CigiEntity.h`, shipped in
  `SimObjects\CigiEntity\sim.cfg`). Its `[General] CategoryId` is a GUID this DLL
  registers with `RegisterSimulationCategory()` at load, so Prepar3D hands the
  object's behaviour to `CigiEntity` — an `ISimObject` that registers **no
  simulation** and has no `[contact_points]`. Nothing moves it between writes and
  nothing clamps it, so the pose written is the pose that stays and a command
  below the sim's terrain is simply where the object goes. It needs no pause and
  no slew either: the sim runs at its own rate while the HOST owns the pose.

Which one is in play is reported rather than assumed from the title: each tick
the add-on asks the object for its category
(`IBaseObjectV430::GetCategoryId`) and compares the GUID, and the answer is `diag`
bit 29 (`kPhysicsFree`). A title that quietly fell back to the sim's own object
therefore shows up as one, and not as a silent mixture.

The physics-free object ships with **no model folder**, so it is not drawn: pose,
read-back and placement all work without one. Give its container a `model\` folder
to make it visible — a copy of the model, or a directory junction to it when the
model lives in another package. The writer sees no difference. On the bench, for
example, every entity is given the AS332's body this way:

```powershell
$c = "$env:USERPROFILE\Documents\Prepar3D v6 Add-ons\PdkSimLink\SimObjects\CigiEntity"
$a = "D:\Addon P3D\Nemeth Designs - Eurocopter AS332 MK2\Simobjects\Rotorcraft\ND_AS332"
New-Item -ItemType Junction -Path "$c\model"       -Target "$a\Model"
New-Item -ItemType Junction -Path "$c\Texture.red" -Target "$a\Texture.red"
# then set texture=red in "$c\sim.cfg"
```

Junctions need no elevation, and keeping the shipped container generic leaves the
question of *which* model each entity type shows to the content side (TODO.md item
7) rather than baking one aircraft into the link.

## What this build is and is not

This **works end to end on the bench**: a replayed capture makes Prepar3D create
one object per driven entity and hold every one of them on the commanded pose,
below the sim's own ground included. What it is not yet:

* **Pose only.** Time of day, weather and effects are the next things the same
  channel will carry. The table has room and `kVersion` guards a half-updated
  pair.
* **The camera follows an entity, with `camera=`.** The created objects are
  non-user objects, so on their own they move without moving the picture: in
  Prepar3D the view belongs to a *window*, and an object the add-on created is
  not that window's camera. `[simlink] camera=<entity id>` puts every 3D view
  onto a Fixed-origin camera definition and applies the HOST's own eye for that
  entity to it
  every frame — `SetLLA` (lat/lon in degrees, altitude in metres) and `SetPBH`
  (pitch, bank, heading) — so the HOST's own aircraft flies the picture. The
  per-channel angles of an immersive dome are untouched: a ViewGroup renders its
  channels from the window's *active* camera and applies each channel's frustum
  offsets on top of it, so the dome keeps its angles and gains a moving origin.
  The entity the camera rides is created from `own_ship_title` instead of
  `title`, so the picture is the world rather than the inside of the model that
  entity was given (see *The camera*). Off by default; see *Configuration*.

## Configuration: `PdkSimLink.ini`

The DLL reads `PdkSimLink.ini` from its own folder:

```ini
[simlink]
title=<a Prepar3D container title>
own_ship_title=CigiStarter.Entity.OwnShip
camera=<entity id the sim's view should fly from, or blank>
camera_view=Cigi View
camera_log=0
```

There is no container title that exists in every installation, so it has to be
given. `title` is the string Prepar3D's own Vehicle Select shows -- an object's
`title=` from its `aircraft.cfg` / `sim.cfg`. Until it is set the add-on is
loaded and reports itself (the operator window says
`PdkSimLink.ini has no title to create from`), but creates nothing.

Two titles are meaningful, and it is the *object* that differs, not this file:

| `title` | The object |
|---|---|
| `CigiStarter.Entity` | the physics-free object that ships with this add-on (see *The object* above) — no flight model, no ground contact |
| any stock title | the sim's own object, with its own simulation; it refuses a pose far below the ground |

The default for a working link is `CigiStarter.Entity`. Point it at a stock title
only for a like-for-like comparison on the bench: it is the object whose own
simulation runs between the writes, and it shows what refusing a below-ground
pose costs.

`own_ship_title` is the container for the **view's own aircraft** — the one entity
the camera rides, and the only entity created from a different title. It defaults
to `CigiStarter.Entity.OwnShip`, the second model-less container that ships with
this add-on (`SimObjects\CigiEntity.OwnShip\sim.cfg`), because the eye is *inside*
that entity: a model there is a hull across the entire picture. Everything else
about the own ship is as usual — it is created, moved to the HOST's pose every
tick and read back like any other entity, so the link is proved on it too; only
the drawing is gone. Setting the key **empty** draws the own ship from `title`
instead, which is only wanted where the camera is not riding the picture that
matters. With `camera` blank there is no own ship and every entity uses `title`.

### The camera

| Key | Meaning |
|---|---|
| `camera` | the CIGI entity id the sim's view should ride — normally `0`, the HOST's own aircraft. The eye is the HOST's own (its View Control), not the entity's reference point: see *The eye is the HOST's* below. Blank or absent leaves the sim's view alone, which is the default. |
| `camera_view` | the camera definition every 3D view is put onto before it is driven. Must be Fixed-origin. Default `Cigi View`. |
| `camera_log` | `1` writes `PdkSimLink.camera.log` beside the DLL: about a line a second per driven view, each with the entity's pose and the eye that becomes the camera (both in CIGI's roll/pitch/yaw, labelled `rpy`), the pose the camera is then told in Prepar3D's own pitch/bank/heading (`told pbh`), and what it reports back (`reads pbh`). The two orders are labelled because they are not the same order and the bank is not the same sign. Default `0`. |

**Why it is needed at all.** An entity object the add-on created is a *non-user*
object, so moving it does not move the picture. Prepar3D's view belongs to a
window; a ViewGroup renders its channels from that window's **active camera** and
applies each channel's own frustum offsets on top. The dome's angles live in the
ViewGroup, and they are the reason the camera is the thing to drive rather than
the window: drive the camera and every channel keeps its own angle and gains a
moving origin.

**Why a Fixed camera.** `SetLLA` — the only PDK call that positions a camera in
the world rather than relative to something else — applies to cameras whose
`Origin` is `Fixed`, and to no others. Prepar3D already ships one for this job:
`Cigi View` in `cameras.cfg` (`Origin = Fixed`, `MenuHidden`, `CycleHidden`,
`HideUserAircraft`) is its own CIGI out-the-window camera. `camera_view` puts
each 3D view onto it and then applies the HOST's eye for the entity every frame.

**How it is driven.** In the window plugin's `OnPreCameraUpdate` — the documented
place for values the camera's own update then uses for its position and
orientation — `SetLLA(lat, lon, alt)` and `SetPBH(pitch, bank, heading)` are
called every frame. CIGI carries roll/pitch/yaw and the PDK wants
pitch/bank/heading, so roll and heading swap places — and the bank is
**negated**, because Prepar3D's bank axis is the opposite of CIGI's roll. That is
the same negation an entity object's pose gets (`SimLinkMap.h`'s `toPdkPose()`
carries the measurement), and it has to be the same one: a camera that banked the
other way from the aircraft it rides would draw the model and the horizon
disagreeing. The contract test covers the conversion so the two halves cannot
disagree. The definition switch itself
happens on the frame event and not inside that callback, because
`SetCameraDefinition` replaces the window's camera system mid-flight and
re-entering it from the pre-camera callback is not something the SDK promises.
Each window is switched once, and a window that renames itself when its
definition changes (a view is named after its definition, so `Cockpit - View 00`
becomes `Cigi View - View 00`) is recognised as the same window rather than a new
one.

**The eye is the HOST's, and it is not the entity's reference point.** The HOST
says where its own viewpoint is in a CIGI **View Control**, and in every capture
that is one static definition: 2.8 m forward of entity 0's reference point, 5 cm
to the right, half a metre up, level, turned a degree (yaw 359) — the pilot's seat. CigiStarter
decodes it and publishes it beside the entity rows (`SimLink.h`; the `view` row
that `kVersion` 3 added), and the add-on applies it instead of the entity's own
pose whenever the view is attached to the entity the camera rides. The offsets
are *body* offsets (`+X` out the nose, `+Y` out the right wing, `+Z` out the
bottom — the frame the entity-relative HAT/HOT requests use), so they are rotated
out of the body frame by the entity's own attitude before they become a latitude
and longitude, and the view's angles are added to the entity's with the heading
wrapping: `SimLinkMap.h`'s `viewEyePose()`, driven by the contract test without a
simulator. With no View Control, or one attached to another entity, the camera
stays on the reference point exactly as it did before the eye was carried.
Because the eye is then inside the aircraft, the entity it rides must not be
drawn — see below.

**The view's own aircraft is not drawn.** The entity the camera rides has the eye
*inside* it, and Prepar3D draws an entity object the add-on created like any other
object: a non-user object has no reason to be treated as an own ship. Left alone,
the same pose that flies the picture sits inside whatever model the entities'
container was given — the operator's words for that were "the view is inside the
cabin of aircraft, not the cockpit view like usual". Prepar3D hides the sim's
*own* aircraft for this camera already (`HideUserAircraft` on the `Cigi View`
definition) and this is the same rule for the entity this add-on created: the
camera's entity is made from `own_ship_title`, a container that carries no model,
while every other entity still draws from `title`.

**Measured on the bench, 2026-09-29** (`camera=0`, `camera_log=1`, replays of the
banked pair, `right.bin` and then `left.bin`): every sample read the camera back
exactly on the pose it had been given. The log prints both orders and labels them
-- `rpy` is CIGI's roll/pitch/yaw, `pbh` is the PDK's pitch/bank/heading --
because they are neither the same order nor, for one field, the same sign. One
sample of `right.bin`: the entity at `-6.473351 106.725805 465.26 | 15.51 1.56
253.38` became the eye `-6.473356 106.725780 465.81 | 15.51 1.56 252.38` -- half a
metre up, and the view's degree taken off the heading -- and the camera was told
`pbh 1.56 -15.51 252.38`, pitch then the bank *negated* then the heading, and
reported exactly that (a heading of 252.38 comes back as the equivalent
`-107.62` once it crosses 180°). `left.bin` mirrors it: CIGI roll `-18.87` is
told to the camera as bank `18.87`. An earlier run, before the bank was negated,
read the same sort of pose with the bank column equal to the eye's roll rather
than its negative. The first sample read `0.0 0.0 1.00`,
the definition's own fixed point, and every sample after the first `SetLLA`
matched the eye, so the pose is the HOST's and not the definition's. The window
reports `CAMERA_SYSTEM_ORIGIN_FIXED` after the switch. The read-back proves the
call landed and nothing more -- it returns whatever number it was given, so it
can never say which way the bank leans; that sign is settled by eye and lives in
one place (`SimLinkMap.h`'s `toPdkPose()`). The picture was looked at the same
time: the parked pose shows the apron, the hangars and the mountains — the world
from the eye, with no hull across it — and a mid-sortie pose at 292 m shows the
terrain below the horizon from the pilot's seat. What the bench cannot show is
the dome itself — one monitor, no Immersive Display Pro — so the per-channel
angles are proved only where the ViewGroup is, and a level aircraft cannot tell a
1° bank from a 1° turn, which is why the *order* of the View Control's angles is
still on the rig list (`..\..\TODO.md`, 8).

**What was deliberately not done.** An "AI-object camera" (the pilot's eye inside
another aircraft) was the first choice and is not reachable from a PDK add-on:
the PDK can create *instances* only from a definition that already exists
(`CreateCameraInstance`) and has no call that creates a definition, and an
instance it did create is not a window's camera, so nothing would render it. The
only exposed handler for `ActivateEntityTracking` is on `IControllableCamera`,
which is a **look-at** — it aims a camera at an entity rather than putting the
eye inside it — which is exactly wrong for a dome. The Fixed camera above is
Prepar3D's own answer to the same problem, and it puts the eye in the same place.

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

1. Put the DLL, `add-on.xml`, `PdkSimLink.ini` and the `SimObjects\` folder (the
   physics-free object's containers) in Prepar3D's default add-on discovery
   folder, one level deep:

   ```
   %USERPROFILE%\Documents\Prepar3D v6 Add-ons\PdkSimLink\
       PdkSimLink.dll
       add-on.xml
       PdkSimLink.ini
       SimObjects\CigiEntity\sim.cfg
       SimObjects\CigiEntity.OwnShip\sim.cfg
   ```

   Both containers are needed: `CigiEntity` is the one `title` creates from, and
   `CigiEntity.OwnShip` is the model-less one the view's own aircraft is created
   from when `camera` is set. Copying only the first is a common slip and shows
   up as a `createFails` count in the operator window — the e2e test names the
   missing container rather than letting it pass.

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
| `positionReads` / `getFails` | poses read back with `GetPosition`, and reads it refused |
| `heldValid` | rows carrying a fresh read-back on the last tick |
| `driftMaxMeters` / `driftMaxDeg` | worst gap between where an object *was* and where it was last written, over the run |
| `diag` bit 24 | the sim-object manager was obtained |
| `diag` bit 25 | a tick has been delivered |
| `diag` bit 26 | `PdkSimLink.ini` has no title |
| `diag` bit 27 | the newest pass came from the add-on's own thread (unused here) |
| `diag` bit 28 | a `GetPosition` read-back has been taken |
| `diag` bit 29 | the objects are the add-on's physics-free ones (asked of each object's category, not assumed from the title) |
| `diag` bit 30 | at least one object reports itself paused or in slew |

## The read-back

`SetPosition` returning `S_OK` means the sim accepted the pose, not that it kept
it. So before each write the add-on asks the object where it actually is
(`IBaseObjectV400::GetPosition`), publishes that as a `Held` row (`SimLinkIpc.h`,
its own table and seqlock), and measures the gap to the pose it last wrote into
`driftMaxMeters` and `driftMaxDeg`. `held[]` is the add-on's; CigiStarter only
reads it.

Measured on the bench against the AS332, feeding the capture's own positions:

* a fixed pose, and a 30 m/s climb, are held to **0.02 m** — for as long as the
  command is fed and after it stops;
* roll, pitch and yaw are held to **<0.1°** on every axis, the heading wrap
  included;
* a pose far below the ground is **refused**, and the object is held above it.
  Publishing below-ground poses directly through the SimLink region (2026-09-29):
  over land (terrain 174.3 m) commands of 50–160 m were held at 176.6 m, and over
  water (0.0 m) commands of −10 to −50 m were held at +26.7 m. The hold altitude
  is a property of the place, not the terrain height, and a command only a little
  below the ground is followed (land 168 m → 168.2 m, water −7 m → −6.8 m).

So the native flight model does **not** drift a pose it is given; replaying a
moving capture, the worst measured drift was 0.19 m. What it does instead is
refuse a pose far below the ground — and that is what the physics-free object
removes.

Note that `gnd-fly-bank.bin` never commands below the ground: entity 0 stays
between 178.9 m and 913 m over terrain of 174.3 m, and entity 3 is 0.3 m under
water. So it exercises no refusal and shows ~0.2 m of drift — **not** the 277 m /
211 m an earlier note recorded, which came from terrain figures (456.4 m and
210.9 m) that are not the terrain at those entities (it is 174.3 m and 0.0 m).

Measured the same way against `CigiStarter.Entity`, on the same
`gnd-fly-bank.bin` capture:

* the pose is held **exactly** — 0.00 m and 0.00° over every comparison, static
  and moving;
* **a command below the ground is where the object goes**: it has no contact
  points, so it is not refused a below-ground pose the way the stock object is,
  and entity 3 sits at −0.3 m under the water surface at 0.0 m, with no drift;
* the sim was never paused or in slew (`diag` bit 30 stayed clear), so a rotor
  has an unpaused, non-slew sim to turn in (TODO.md item 4).

The write-before-read race a fast replay could show is gone with it: nothing
moves the object between the write and the next tick's read, so there is no gap
to measure. That is the whole of item 2 — the pose is owned without fighting the
sim, and without pausing it.

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
the heading slot, the bank as CIGI's roll **negated** -- Prepar3D's bank axis is
the other way round); that a row caught mid-rewrite is never believed, read flat
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
* the sim's object is read back (`GetPosition`), on the pose that was written, so
  "the sim accepted it" becomes "the sim is holding it";
* which object is in play is reported (`diag` bit 29), and when it is the
  physics-free one the drift is **asserted** to be zero rather than merely
  reported — a below-ground pose, which the stock object refuses, is the proof;
* no object is paused or in slew (`diag` bit 30);
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

The objects it creates come from the container title in `PdkSimLink.ini`; with
`CigiStarter.Entity` they are the physics-free ones. It reads each object's pose
back and reports the drift against what was written (see *The read-back* above),
so `-Bin '..\gnd-fly-bank.bin' -Speed 20` is the run that measures whether the sim
is holding the pose on a moving aircraft rather than merely accepting it. (That
capture never commands below the ground, so the stock object's refusal is not
exercised by it; see *The read-back* for how it was measured directly.)

## What to check on the rig

The bench answers the link; these need the sim, the HOST and the flight model:

* how the native flight model behaves between writes -- answered on the bench for
  the AS332 (no drift in the air; a pose far below the ground is refused and held
  above it), so what is left for the rig is whether the same holds for its own
  model and whether any object is ever commanded far below the sim's terrain in
  the deployed sorties. The physics-free object makes the refusal moot where it is
  used, but the rig still has to show that the custom category registers and its
  objects create and hold on v6 **Professional** (TODO.md item 8);
* that a banked entity on the rig leans into its turn. The sign is settled --
  CIGI's roll is right-bank-positive (read off the captures) and Prepar3D's bank
  axis is the opposite, which the predecessor measured on the bench and which is
  exactly what "the aircraft moves right but banks left" was showing when a
  capture was replayed into this writer; `SimLinkMap.h`'s `toPdkPose()` negates
  it. It was then looked at on the bench with the negation in (2026-09-29): a
  replay of the banked pair, `right.bin` then `left.bin`, leans right through the
  right turn and left through the left one. A sign is only ever proved by looking,
  though, and the rig's own model is the one that has to look right;
* how many entities the HOST actually drives, and whether the entities that are
  not the aircraft should exist as objects at all;
* whether the dome keeps its per-channel angles with the camera driven -- the
  bench has one monitor and no Immersive Display Pro, so a ViewGroup's frustum
  offsets are only proved on the rig. The mechanism itself is proved here: the
  sim reports the camera back on the commanded pose every frame;
* the **order** of the View Control's three angles. Every capture carries one
  non-zero value -- a degree (yaw 359) in the slot the CIGI View Control table
  calls `yaw` -- and a level aircraft cannot tell a 1° bank from a 1° turn. It is
  read as `roll`/`pitch`/`yaw`, the same order as the Entity Control, which is
  what the one non-zero value reads as; the ICD's own View Control table may list
  them yaw-first, and reading it the other way would bank the picture a degree
  instead of turning it.
