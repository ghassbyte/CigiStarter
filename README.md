# CigiStarter

A minimal **CIGI 3.3 Image Generator (IG)** for a new IG PC.

The HOST in a CIGI flight-simulator link stays silent until the IG transmits
**Start-of-Frame (SOF)** messages. `CigiStarter` plays that IG role: it sends
SOF at a configurable rate to wake up the HOST and begin the flight-data
stream, and (optionally) answers HAT/HOT requests with the real terrain height
under the aircraft, read from the local Prepar3D via FSUIPC.

## What it does

1. Binds a UDP socket on the downstream port (`8000`) where the HOST sends
   control/flight data.
2. Sends **SOF** (24-byte Start-of-Frame) datagrams to the HOST upstream port
   (`8001`) at the configured frame rate — this is the "request" that starts
   the data flow.
3. Parses inbound CIGI messages (framed by `[id][size]`) and prints a one-line
   heartbeat showing datagrams/s, total datagrams, entity updates and the
   HOST frame counter.
4. Optionally (`--respond-hathot`) answers HAT/HOT requests, riding the
   responses along on the SOF stream as the reference IG did. The terrain
   height is read from the local Prepar3D via FSUIPC — see
   [HAT/HOT answering](#hathot-answering).
5. Runs interactively with a live in-place status line: starts paused, then
   `S`/`X`/`Q` start, stop, and quit the SOF stream.

## Requirements

- Windows 10/11, x64.
- Windows system libraries only, plus the **FSUIPC SDK** when `--respond-hathot`
  is used (`Ws2_32.lib` and `Winmm.lib` for sockets/timing; `user32.lib` and
  `FSUIPCuser64.lib` for the terrain read). Without `--respond-hathot` the
  program never touches FSUIPC.
- The FSUIPC header/library are the ones already in this repository, shared with
  `CigiP3dBridge`:
  `..\CigiP3dBridge\external\UIPC64_SDK_C_version2\`. If your checkout lays the
  two projects out differently, point the build at another copy with
  `/p:FsuipcSdkDir=<directory>`.
- `FSUIPCuser64.lib` was compiled with **MSVC v140** and is an LTCG library, so
  this project must **not** build with `/GL` — `WholeProgramOptimization` is
  therefore `false` for Release. Building with `/GL` fails at link with
  `C1047` / `LNK1257` ("created by a different version of the compiler").
- Built with the static CRT (`/MT`, `/MTd`), so the executable runs on a fresh
  IG PC **without installing the Visual C++ redistributable**. (The FSUIPC SDK
  does not use the CRT across its boundary, so the mixed CRT is harmless —
  expect a benign `LNK4098` about `MSVCRT`.)
- The embedded manifest requests **administrator** privileges (the app launches
  elevated for network/firewall access in the simulator environment).

## Building

Using MSBuild from a Visual Studio / Build Tools developer environment:

```
msbuild CigiStarter.sln /p:Configuration=Release /p:Platform=x64
```

Or open `CigiStarter.sln` in Visual Studio and build **Release | x64**.
The output is `x64\Release\CigiStarter.exe`.

## Usage

The link addresses and ports are **fixed by the deployment** and hardcoded:

| Role | Address          | Port |
|------|------------------|------|
| HOST | `192.168.70.2`   | 8001 (upstream SOF destination) |
| IG   | `192.168.70.70`  | 8000 (downstream RX bind)       |

```
CigiStarter.exe [options]

  --fps <rate>        SOF transmit rate in Hz           (default 100)
  --ig-mode <0|1>     SOF IG mode 0=Standby 1=Operate   (default 0)
  --respond-hathot    answer HAT/HOT requests using the terrain height read
                      from the local Prepar3D via FSUIPC       (default off)
  --help              show help
```

Example:

```
CigiStarter.exe --fps 100 --respond-hathot
```

## HAT/HOT answering

The HOST drives its aircraft altitude to the **Height Of Terrain (HOT)** that
the IG reports, so an IG that answers `HOT = 0` flies the aircraft into the
ground. `--respond-hathot` makes CigiStarter report the *real* terrain:

| Request                | Response                                      |
|------------------------|-----------------------------------------------|
| Type 0 (HAT) / 1 (HOT) | HAT/HOT Response (id 102, 16 bytes)           |
| Type 2 (Extended)      | HAT/HOT Extended Response (id 103, 40 bytes)  |

- `HOT` = FSUIPC `0x0020`, the ground altitude under the player aircraft (m).
- `HAT` = test-point altitude − `HOT`. Entity-relative requests use the
  aircraft's FSUIPC altitude minus the request's Z offset (CIGI entity axes are
  local NED, so **+Z points down**); geodetic requests carry the altitude
  themselves.
- `Valid` is set to 0 — never a fabricated 0 m — when no answer can be produced:
  no FSUIPC link, a request for an entity other than the ownship (entity 0), or
  a geodetic point more than 500 m from the aircraft. FSUIPC only exposes the
  terrain under the *player* aircraft, so that is the one test point this
  program can answer for.
- `updatePeriod = 0` (one-shot) requests are answered once; a non-zero period is
  honoured by re-answering every *n*-th frame.

The FSUIPC link is polled by a background thread that owns every FSUIPC call, so
the 100 Hz SOF pacing is never delayed by an FSUIPC round trip. If Prepar3D is
not running, CigiStarter still starts: it keeps retrying, answers `Valid = 0`
until the sim appears, and shows the current terrain height in the status line.

## Runtime keyboard controls

The program starts with SOF transmission **paused**. Control it from the
console (case-insensitive):

| Key | Action          |
|-----|-----------------|
| `S` | start sending SOF |
| `X` | stop sending SOF  |
| `Q` | quit             |

`Ctrl+C` also quits.

## Network layout

| Direction   | From                  | To                    | Port | Payload                         |
|-------------|-----------------------|-----------------------|------|---------------------------------|
| Downstream  | HOST `192.168.70.2`   | IG `192.168.70.70`    | 8000 | IGCtrl, EntityCtrl, HAT/HOT req |
| Upstream    | IG `192.168.70.70`    | HOST `192.168.70.2`   | 8001 | SOF (+ HAT/HOT responses)       |

## Protocol notes

- **CIGI version 3.3**, little-endian on the wire.
- A message is framed as `[message-id(1)][message-size(1)][payload...]` where
  `size` is the total byte length including the two header bytes.
- The SOF sent by this program matches the reference capture:
  `65 18 03 00 00 30 00 80` + frame number + timestamp + last-host-frame + pad.
  Byte `0x30` is IG mode 0 (Standby); pass `--ig-mode 1` for `0x31` (Operate).
- SOF pacing uses the 1 ms system timer resolution (`timeBeginPeriod(1)`).
  Windows' default granularity is 15.6 ms (~64 Hz), which makes any rate above
  ~60 Hz fire in back-to-back bursts that overwhelm the HOST.
- HAT/HOT responses use the CIGI 3.3 layouts: the 16-byte Response (102) carries
  a single `Height` double at offset 8, and the 40-byte Extended Response (103)
  carries `HAT` at offset 8 and `HOT` at offset 16. **Both** have a 3-byte
  reserved gap between the flags byte and the first double, which is what aligns
  that double to an 8-byte boundary. (CIGI 3.2 packed them immediately after the
  flags; getting this wrong makes the HOST read `HOT` out of the material-code
  bytes — i.e. zero — and fly the aircraft down to the terrain.)

## Project layout

| File               | Purpose                                        |
|--------------------|------------------------------------------------|
| `main.cpp`         | entry point, sockets, SOF + receive threads    |
| `cigi.h`           | CIGI 3.3 message IDs, sizes, pack/parse API    |
| `cigi.cpp`         | explicit little-endian message packing         |
| `TerrainSource.h`  | FSUIPC-backed terrain/aircraft snapshot API    |
| `TerrainSource.cpp`| the FSUIPC poller thread (owns all FSUIPC use) |
| `app.manifest`     | UAC `requireAdministrator` manifest            |
| `agents.md`        | description of the HOST / IG / CigiStarter agents |
| `CigiStarter.sln`  | Visual Studio solution (x64 only)              |
| `CigiStarter.vcxproj` | VS project (x64 only, static CRT, ws2_32, FSUIPC) |
| `CigiStarter.vcxproj.filters` | project file organization            |
| `.gitignore`       | ignores build output and VS artifacts          |

See `agents.md` for a description of each agent and the message flow.
