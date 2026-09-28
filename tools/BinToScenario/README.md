# BinToScenario

Rewrites a recorded HOST → IG CIGI stream (a `.bin`) as a **CIGI Host Emulator
script** (`.scp`), so the same scenario can be run on the native emulator
(`external\HostEmulator_Win32_v3.3.3\Hemu3.exe`) instead of being replayed
through the `.bin` players.

## Why

There are two different HOSTs in this project and they do not read each other's
files:

|  | What it is | What it reads |
|---|---|---|
| `.bin` captures | a **recording** of the real HOST → IG stream, pulled out of a Wireshark capture by `..\..\..\TAP\pcap_to_bin.py` | `CigiP3dSimConnect.exe --bin` (SimConnect) or `CigiP3dBridge.exe --bin` (FSUIPC) |
| CIGI Host Emulator | a **live HOST** you drive, on the bench | `.def` definition tables, a `.scp` script, a `.sf3` scenario, a `.cap` capture |

`Hemu3.exe` has no `.bin` reader anywhere in it (only `*.def`, `*.scp`, `*.cap`
and `.sf3` dialogs), and the `.bin` players are not the emulator. So a capture
cannot be handed to the emulator as it is. This tool is the bridge: it decodes
the capture and writes the emulator commands that put the emulator into the same
scenario — which is what lets CigiStarter be exercised against a HOST that
reproduces a real sortie without a third program in the loop.

## What is translated

| CIGI 3.3 message | becomes | notes |
|---|---|---|
| Entity Control (2) | `ENTITY_POS` + `ENTITY_YPR` | every frame, for every entity that carries a usable pose |
| Celestial Sphere Control (9) | `TIME` | the day/dusk/night/dawn difference in the `*_replay` captures |
| Atmosphere Control (10) | `GLOBAL_AIR_TEMP`, `GLOBAL_VISIBILITY`, `GLOBAL_BARO_PRESSURE`, `GLOBAL_HUMIDITY`, `GLOBAL_WIND` | the warm-up the route captures carry |
| HAT/HOT Request (24) | `HAT_HOT_REQ_BEGIN` / `_TYPE` / `_SOURCE` / `_UPDATE_PERIOD` / `_END` | so the IG is asked for terrain exactly as the HOST asked |

### What is deliberately not, and why

| CIGI 3.3 message | why not |
|---|---|
| IG Control (1) | the emulator sends its own, every frame |
| Component Control (4) | not field-mapped here; the special-effect entity it targets *is* carried, as an entity pose |
| Articulated Part Control (6) | a pose-only replay does not need them — nothing moves a control surface on its own |
| Rate Control (8) | the poses are absolute, not integrated; rates would only fight them |
| View Control (16) | per-channel angles stay in the sim's own `viewgroup.xml` — a project decision, not an oversight |

Everything that is left out is **counted and reported** — in the console summary
and in the script's own header — so nothing disappears without the operator being
told.

## The `.bin` format

Repeating little-endian records: `uint32 deltaMicros`, `uint16 len`, then `len`
bytes of one UDP datagram. The payload is CIGI 3.3, framed `[id][size][payload]`.
This is the same format `pcap_to_bin.py` writes and the players read; the reader
here refuses a record cut short or trailing rubbish rather than converting half a
flight and stopping quietly.

## Using it

```
.\tools\bin-to-scenario.ps1 -Bin 'E:\03_project\Super Puma\Visual\Local\hlm.bin'
```

That builds the tool and writes `hlm.scp` beside the capture. Then, on the bench:

1. Start the emulator (`external\HostEmulator_Win32_v3.3.3\Hemu3.exe`). It reads
   its `.def` tables and its script from the folder it runs in.
2. Load the `.scp` from its script dialog.
3. Run the exercise.

`-Out <dir>` puts the scripts somewhere else, `-EntityType <n>` changes the
emulator entity type the `ADD_ENTITY` lines use, and `-NoCompress` writes one
`FRAME` per datagram instead of folding unchanged runs into `FRAME_WAIT`.

Or call the tool directly:

```
msbuild tools\BinToScenario\BinToScenario.vcxproj /p:Configuration=Release /p:Platform=x64
x64\Release\BinToScenario.exe <capture>.bin [<output>.scp] [--entity-type 118] [--no-compress]
```

## What the script looks like

```
ADD_ENTITY 0 118
ADD_ENTITY 3 118

HAT_HOT_REQ_BEGIN 5
HAT_HOT_REQ_TYPE EXTENDED
HAT_HOT_REQ_SOURCE ENTITY 0.00000000 0.00000000 10.00000000 0
HAT_HOT_REQ_UPDATE_PERIOD 0
HAT_HOT_REQ_END
ENTITY_POS 0 -6.13411008 106.65523438 15.488
ENTITY_YPR 0 37.2000 1.0000 0.0000
ENTITY_POS 3 -6.01383889 106.88920278 -0.338
ENTITY_YPR 3 0.0000 0.0000 0.0000
FRAME
FRAME_WAIT 12
```

Pose and scene lines are written only when they change, so a level or parked
capture costs one line, not one per frame, and runs of silent frames fold into a
single `FRAME_WAIT`. A frame-by-frame sortie (`gnd-fly-bank.bin`, 19 437
datagrams) still makes a large script, because every frame really does move; the
emulator runs it at its own frame rate. The HOST also asks its HAT/HOT question
about a point *on the aircraft*, so that request changes almost every frame of a
moving capture and the folding rarely helps there — it is carried faithfully
anyway, which is the point: CigiStarter's answers get exercised exactly as the
real HOST would exercise them.

## Things worth knowing

* **`ADD_ENTITY` needs the type in your `.def`.** The default 118 is the `f18c`
  the emulator's shipped `Default\Entities.def` defines. Point `-EntityType` at
  another type for another configuration; an id that already exists is left
  alone, so running a script over a scenario that already defines the entities is
  safe.
* **Every command was checked against the emulator's own script reference**
  (the help shipped beside `Hemu3.exe`): the names, the argument order and the
  units. They line up with CIGI's — `ENTITY_POS` takes degrees and metres as
  MSL, the same numbers the Entity Control carried, and `ENTITY_YPR` really is
  yaw, pitch, roll. `HAT_HOT_REQ_UPDATE_PERIOD 0` is written out rather than
  omitted, because the reference defines a zero as a one-shot request and does
  not say what the default would be.
* **The HOST's own aircraft is entity 0**, and it is created like any other. An
  all-zero pose is "no position", not the Gulf of Guinea, so such an entity is
  neither created nor moved (the reference captures' entity 5 is exactly that).
* **Scene commands land where the capture had them.** The `*_replay` captures
  set the time of day a few hundred datagrams in, so the script changes it
  there. Move the `TIME` line to the top if you want it from the start.
* **The date is not carried over** — it is not valid in these captures. Only
  `TIME hh:mm` is written; the emulator's own `Environment.def` supplies the date.
* **Entity attitude** is written in the order `ENTITY_YPR` wants — yaw, pitch,
  roll — from the CIGI layout's roll, pitch, yaw. The sign and order the HOST
  really means is one of the things a bench run is for (`AGENTS.md`); this tool
  carries what is on the wire and guesses nothing.

## Testing

The converter touches no simulator, so the whole contract is driven without one:

```
.\tools\test-all.ps1                      # every SDK-free contract test
.\tools\test-all.ps1 -Only BinToScenario  # just this one
```

`test\main.cpp` builds synthetic `.bin` images by hand — field by field, so it
pins the wire layout instead of agreeing with the tool's own decoders — and
checks the container framing, the pose and attitude order, entity 0, the
all-zero-pose rule, the time of day, the atmosphere, HAT/HOT requests, both
frame-pacing modes, and that untranslated messages are reported rather than
silently dropped.
