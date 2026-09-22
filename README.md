# CigiStarter

A minimal **CIGI 3.3 Image Generator (IG)** for a new IG PC.

The HOST in a CIGI flight-simulator link stays silent until the IG transmits
**Start-of-Frame (SOF)** messages. `CigiStarter` plays that IG role: it sends
SOF at a configurable rate to wake up the HOST and begin the flight-data
stream, and answers HAT/HOT requests with the real terrain height under the
aircraft, read from the local Prepar3D via FSUIPC — see
[HAT/HOT answering](#hathot-answering).

## What it does

1. Binds a UDP socket on the downstream port (`8000`) where the HOST sends
   control/flight data.
2. Sends **SOF** (24-byte Start-of-Frame) datagrams to the HOST upstream port
   (`8001`) at the configured frame rate — this is the "request" that starts
   the data flow.
3. Parses inbound CIGI messages (framed by `[id][size]`) and shows a live status
   block: datagrams/s, total datagrams, entity updates, the SOF rate (requested
   and achieved), SOF frames sent, the HOST frame counter and the terrain sample.
4. Answers HAT/HOT requests, riding the responses along on the SOF stream as the
   reference IG did. This is on by default (the window's check box switches it
   off, as does `--no-respond-hathot`), and the terrain height is read from the
   local Prepar3D via FSUIPC — see [HAT/HOT answering](#hathot-answering).
5. Runs as a small window with **START** / **STOP** / **EXIT**: it starts paused
   and stays silent until START is clicked — see
   [Operator window](#operator-window).

## Requirements

- Windows 10/11, x64.
- A **windowed** (GUI-subsystem) executable: there is no console window. A
  startup error or `--help` is reported in a message box instead of on stdout.
  The project links with `SubSystem=Windows` plus `EntryPointSymbol=mainCRTStartup`,
  which produces a GUI binary while still allowing a plain `int main(argc, argv)`.
- Windows system libraries only, plus the **FSUIPC SDK** while HAT/HOT answering
  is switched on (`Ws2_32.lib` for sockets and `Winmm.lib` for timing;
  `User32.lib` for the window; `FSUIPCuser64.lib` for the terrain read). With
  answering switched off the program never touches FSUIPC.
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

  --fps <rate>        initial SOF transmit rate in Hz   (default 100)
  --ig-mode <0..3>    IG mode reported until the HOST sends its first
                      IG Control (0=Standby 1=Operate)  (default 0)
  --respond-hathot    answer HAT/HOT requests using the terrain height read
                      from the local Prepar3D via FSUIPC       (default on)
  --no-respond-hathot do not answer HAT/HOT requests, and leave FSUIPC
                      untouched
  --help              show this help (in a message box)
```

The options only set the *starting* state. The SOF rate and HAT/HOT answering
can both be changed from the window while the program runs — see
[Operator window](#operator-window).

Example:

```
CigiStarter.exe --fps 112
```

## HAT/HOT answering

The HOST drives its aircraft altitude to the **Height Of Terrain (HOT)** that
the IG reports, so an IG that answers `HOT = 0` flies the aircraft into the
ground. CigiStarter therefore reports the *real* terrain **by default** — turn
it off in the window, or start with `--no-respond-hathot`, if you must:

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
the SOF pacing is never delayed by an FSUIPC round trip. The poller runs only
while answering is switched on — the window's check box starts and stops it, and
while it is off this program makes no FSUIPC calls at all. If Prepar3D is not
running, CigiStarter still starts: it keeps retrying, answers `Valid = 0` until
the sim appears, and shows the current terrain height in the window's status
block.

## Operator window

CigiStarter is a windowed program, so it is driven with the mouse rather than
from a console. The window is a fixed-size plain Win32 dialog:

```
+--------------------------------------------------------------------------+
|  HOST 192.168.70.2:8001   ->  SOF destination                            |
|  IG   :8000   (this IG is 192.168.70.70)                                 |
|                                                                          |
|  Stream    : RUNNING                                                     |
|  SOF rate  : 100 Hz requested, 100/s measured                            |
|  SOF frames: 101                                                         |
|  Received  : 0 datagrams (0/s), 0 entity updates                         |
|  IG mode   : 0 (Standby)  <- commanded by the HOST                       |
|  HOST frame: 0                                                           |
|                                                                          |
|  HAT/HOT   : answering from FSUIPC                                       |
|  Terrain   : 27.0 m under the aircraft, 30.1 m aircraft                  |
|                                                                          |
|  [x] Answer HAT/HOT (terrain from FSUIPC)  SOF rate: [ 100 ] Hz [ Apply ] |
|                                                                          |
|  [ START ]  [ STOP ]                                              [ EXIT ] |
+--------------------------------------------------------------------------+
```

- The status block refreshes four times a second. The datagram and SOF rates are
  sampled once a second from the cumulative counters; the SOF line shows the
  rate that is *requested* next to the rate the transmit loop actually achieved.
- **START** / **STOP** start and stop the SOF stream. The program boots
  *paused*, so nothing is transmitted to the HOST until START is clicked. The
  button that would be a no-op is disabled.
- **Answer HAT/HOT** switches HAT/HOT answering on and off, and with it the
  FSUIPC poller — switching it off means this program makes no FSUIPC calls at
  all, switching it back on reconnects. It starts **on**, unless
  `--no-respond-hathot` was given.
- **SOF rate** is a whole number of Hz, from 1 to 100000. It is applied to the
  transmit loop on the next frame, so the stream can be retuned *while it runs*
  — no stop/start needed. Commit it with **Apply** or by pressing **Enter**. A
  value that is not a whole number in range is refused with a warning and the
  box snaps back to the rate actually in force; when the box merely loses focus
  a valid value is applied quietly and an unfinished one is left alone, so
  clicking EXIT never nags. `--fps` only seeds it.
- **EXIT** is the **only** way out. The title-bar **X** and **Alt+F4** are
  deliberately swallowed — `SC_CLOSE` is deleted from the window's system menu
  (Windows then draws the X greyed out and non-clickable) and `WM_CLOSE` is
  ignored — so the IG cannot be taken off the link by a stray click. This
  matches the `CigiP3dBridge` GUI, which follows the same rule.
- Since the process has no console, `Ctrl+C` cannot reach it either.
- Terrain lines appear while answering is on: `answering from FSUIPC` with the
  ground/aircraft heights, or a warning that there is no FSUIPC link to
  Prepar3D. With answering off they read `not answered (switched off)`.

## Network layout

| Direction   | From                  | To                    | Port | Payload                         |
|-------------|-----------------------|-----------------------|------|---------------------------------|
| Downstream  | HOST `192.168.70.2`   | IG `192.168.70.70`    | 8000 | IGCtrl, EntityCtrl, HAT/HOT req |
| Upstream    | IG `192.168.70.70`    | HOST `192.168.70.2`   | 8001 | SOF (+ HAT/HOT responses)       |

## Protocol notes

- **CIGI version 3.3**, little-endian on the wire.
- A message is framed as `[message-id(1)][message-size(1)][payload...]` where
  `size` is the total byte length including the two header bytes.
- The SOF is `65 18 03 00 00 <b5> 00 80` + frame number + timestamp +
  last-host-frame + pad, where byte 5 packs the CIGI minor version (bits 4-7;
  3.3 => `0x30`) together with the **IG mode** (bits 0-1).
- The IG never picks its own mode: the HOST commands it with IG Control and the
  SOF reports it back. CigiStarter follows that, so byte 5 becomes `0x31`
  (Operate) as soon as the HOST asks for Operate — which the reference HOST in
  `hlm.bin` does in every one of its 1160 datagrams. `--ig-mode` only sets the
  mode reported *before* the first IG Control arrives.
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
| `main.cpp`         | entry point, sockets, SOF + receive threads, window callbacks |
| `Gui.h`            | operator-window API: status snapshot + start/stop/HAT-HOT/rate/quit hooks |
| `Gui.cpp`          | the plain Win32 window (status block, START / STOP / EXIT, HAT/HOT check box, SOF rate box) |
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
