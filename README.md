# CigiStarter

A minimal **CIGI 3.3 Image Generator (IG)** for a new IG PC.

The HOST in a CIGI flight-simulator link stays silent until the IG transmits
**Start-of-Frame (SOF)** messages. `CigiStarter` plays that IG role: it sends
SOF at a configurable rate to wake up the HOST and begin the flight-data
stream, and answers HAT/HOT requests with the terrain height under the aircraft
— from the Prepar3D the aircraft is flying in, through a small add-on DLL, so a
runway's flattening and any added scenery are included without anything being
pre-mapped, falling back to an offline **SRTM terrain database** and then to the
sim over FSUIPC — see [HAT/HOT answering](#hathot-answering).

## What it does

1. Binds a UDP socket on the downstream port (`8000`) where the HOST sends
   control/flight data.
2. Sends **SOF** (24-byte Start-of-Frame) datagrams to the HOST upstream port
   (`8001`) at the configured frame rate — this is the "request" that starts
   the data flow. The rate defaults to **150 Hz** and the stream is paced to the
   HOST's replies by default — see
   [SOF rate and flow control](#sof-rate-and-flow-control).
3. Parses inbound CIGI messages (framed by `[id][size]`) and shows a live status
   block: datagrams/s, total datagrams, entity updates, the SOF rate (requested
   and achieved), SOF frames sent, the HOST frame counter and the terrain sample.
4. Answers HAT/HOT requests, riding the responses along on the SOF stream as the
   reference IG did. This is on by default (the window's check box switches it
   off, as does `--no-respond-hathot`). The terrain height comes from the sim the
   aircraft is actually flying in, through the `PdkTerrainProbe` add-on, so the
   surface that will be flown onto is the one reported — and a new scenery
   add-on needs no matching pass. An offline **SRTM terrain database**, and then
   the sim again over FSUIPC, are the fallbacks — see
   [HAT/HOT answering](#hathot-answering).
5. Runs as a small window with **START** / **STOP** / **EXIT**. It starts
   transmitting on launch and the window minimises itself, so a deployment needs
   no click at all before the link is live; `--no-auto-start` restores the
   paused, window-up behaviour — see
   [Operator window](#operator-window).
6. **Fans the HOST's downstream stream out to the other IG PCs** listed in
   `client.txt`, byte for byte, so several visual channels run off the one link
   the HOST already feeds — see [Fan-out](#fan-out-to-the-other-ig-pcs).

## Requirements

- Windows 10/11, x64.
- A **windowed** (GUI-subsystem) executable: there is no console window. A
  startup error or `--help` is reported in a message box instead of on stdout.
  The project links with `SubSystem=Windows` plus `EntryPointSymbol=mainCRTStartup`,
  which produces a GUI binary while still allowing a plain `int main(argc, argv)`.
- Windows system libraries only, plus the **FSUIPC SDK** while HAT/HOT answering
  is switched on (`Ws2_32.lib` for sockets, `Winmm.lib` for timing,
  `User32.lib` for the window, `Iphlpapi.lib` for the client MAC lookups, and
  `FSUIPCuser64.lib` for the aircraft position and the terrain fallback). With
  answering switched off the program never touches FSUIPC. The terrain
  database itself needs no library at all — just the `.hgt` tiles (see
  [Terrain database](#terrain-database)).
- The preferred terrain source is a Prepar3D that is running the
  **`PdkTerrainProbe`** add-on (`tools\PdkTerrainProbe`). Building it needs the
  Prepar3D PDK headers, vendored in `external\Prepar3D-SDK\inc\PDK\`; *using* it
  needs only the built DLL and its `add-on.xml`. It is a separate project rather
  than part of `CigiStarter.sln`, so a machine without the SDK still builds the
  IG. Without it the HAT/HOT answer falls back to the database exactly as before.
- **Npcap** is needed only for the [fan-out](#fan-out-to-the-other-ig-pcs). It is
  loaded at run time rather than linked, so the program still builds and runs
  without it. The Npcap *SDK* (headers) is needed to build and is vendored in
  `external\npcap-sdk-1.16\`; point the build elsewhere with
  `/p:NpcapSdkDir=<directory>`.
- The FSUIPC header/library are vendored in `external\UIPC64_SDK_C_version2\`.
  Point the build at another copy with `/p:FsuipcSdkDir=<directory>`.
- `FSUIPCuser64.lib` was compiled with **MSVC v140** and is an LTCG library, so
  this project must **not** build with `/GL` — `WholeProgramOptimization` is
  therefore `false` for Release. Building with `/GL` fails at link with
  `C1047` / `LNK1257` ("created by a different version of the compiler").
- Built with the static CRT (`/MT`, `/MTd`), so the executable runs on a fresh
  IG PC **without installing the Visual C++ redistributable**. (The FSUIPC SDK
  does not use the CRT across its boundary, so the mixed CRT is harmless —
  expect a benign `LNK4098` about `MSVCRT`.)
- The program runs **unelevated** (the manifest asks for `asInvoker`): a
  deployment starts it from a logon or startup path, where a UAC prompt would
  block an unattended IG. Nothing it does needs administrator on a normal
  install — the [fan-out](#fan-out-to-the-other-ig-pcs) opens Npcap unelevated.
  Two things, if the deployment needs them, are an administrator's job once: an
  Npcap installed to *restrict driver access to administrators* (the window then
  reports that the adapter could not be opened), and a Windows Firewall prompt
  for this program.

## Building

Using MSBuild from a Visual Studio / Build Tools developer environment:

```
msbuild CigiStarter.sln /p:Configuration=Release /p:Platform=x64
```

Or open `CigiStarter.sln` in Visual Studio and build **Release | x64**.
The output is `x64\Release\CigiStarter.exe`.

The SDKs the solution links against are vendored in `external\`, so the build
needs nothing installed beyond Visual Studio / Build Tools.

## Usage

The deployment's link addresses and ports are the defaults, hardcoded in
`main.cpp`:

| Role | Address          | Port |
|------|------------------|------|
| HOST | `192.168.70.2`   | 8001 (upstream SOF destination) |
| IG   | `192.168.70.70`  | 8000 (downstream RX bind)       |

They are overridable for a bench test with `--host-ip` / `--host-port` /
`--ig-ip` / `--ig-port` (below); the deployment passes none of them.

```
CigiStarter.exe [options]

  --fps <rate>        initial SOF transmit rate in Hz   (default 150)
                      With flow control on this is a ceiling: the stream
                      is held back while the HOST has not answered, so a
                      rate above what the HOST manages costs nothing.
  --ig-mode <0..3>    IG mode reported until the HOST sends its first
                      IG Control (0=Standby 1=Operate)  (default 0)
  --respond-hathot    answer HAT/HOT requests, reporting the terrain height
                      from the Prepar3D the aircraft is flying in (through the
                      PdkTerrainProbe add-on), or from the local terrain
                      database, or from Prepar3D directly over FSUIPC, in
                      that order                                  (default on)
  --no-respond-hathot do not answer HAT/HOT requests, and leave FSUIPC
                      untouched
  --flow-control      hold a SOF back while the HOST has not answered the
                      previous one                            (default on)
  --no-flow-control   transmit on the rate alone, without waiting for the HOST
  --auto-start        transmit as soon as the program starts, and minimise the
                      window                                  (default on)
  --no-auto-start     start with the stream stopped and the window up, so START
                      has to be clicked
  --terrain-dir <dir> folder of SRTM ".hgt" terrain tiles to answer HAT/HOT
                      from              (default: terrain\ beside the program)
  --terrain-offset <m> constant added to every database height, for calibrating
                      against the sim                             (default 0)
  --host-ip <addr>    HOST address SOFs and responses go to
                                                         (default 192.168.70.2)
  --host-port <port>  HOST port SOFs and responses go to        (default 8001)
  --ig-ip <addr>      this IG's address, as the extra IG PCs see it
                                                        (default 192.168.70.70)
  --ig-port <port>    port the HOST streams flight data to      (default 8000)
  --help              show this help (in a message box)
```

The options only set the *starting* state, with one exception: the four link
options (`--host-ip`, `--host-port`, `--ig-ip`, `--ig-port`) are read once and
have no window control, because the deployed link never changes. The SOF rate,
HAT/HOT answering and flow control can all be changed from the window while the
program runs — see [Operator window](#operator-window). Whether the stream starts
on launch is a start-up decision only, so `--no-auto-start` is the single way back
to the old behaviour: a window that waits for the button.

One input file matters too: **`client.txt`**, next to the executable, lists the
other IG PCs to copy the downstream stream to. It is optional and is read once at
startup — see [Fan-out](#fan-out-to-the-other-ig-pcs).

A folder matters as well: **`terrain\`**, next to the executable, holds the SRTM
`.hgt` tiles the HAT/HOT answer is taken from — see
[Terrain database](#terrain-database).

Examples:

```
CigiStarter.exe                        # 150 Hz, paced to the HOST, live on launch
CigiStarter.exe --no-auto-start        # wait for START instead (old behaviour)
CigiStarter.exe --no-flow-control --fps 100
CigiStarter.exe --host-ip 127.0.0.1 --ig-ip 127.0.0.1   # bench test, local HOST
```

## Bench testing

The whole link can be exercised on one PC: the **CIGI Host Emulator** plays the
HOST and CigiStarter plays the IG, over the loopback adapter, with the sim's own
terrain still behind CigiStarter if one is running. That is how the HAT/HOT
answering is tested, because neither program prints the exchange — the only
honest witness is the wire.

The emulator is vendored in `external\HostEmulator_Win32_v3.3.3\` (Boeing TSS's
CIGI Host Emulator 3.3.3, GPLv2, `license.txt` as shipped). Run `Hemu3.exe` from
that folder. It reads its link setup at startup from
`HKCU\Software\Boeing TSS\CIGI Host Emulator 3`, which its **Setup** dialog
writes, so point it at the IG there:

| Key | Value |
|-----|-------|
| `Remote IP Addr` | `127.0.0.1` — the IG |
| `Remote Port`    | CigiStarter's port (`8000`, or the bench's `--ig-port`) |
| `Local Port`     | the HOST's port (`8001`, or the bench's `--host-port`) |
| `CIGI Minor Version` / `Big Endian` | `3` / `0` — CIGI 3.3, little-endian |

and start CigiStarter against it, overriding the deployed addresses (it binds
and sends on this machine either way):

```
CigiStarter.exe --host-ip 127.0.0.1 --ig-ip 127.0.0.1
```

On this machine Prepar3D (Professional Plus) holds 8000 for its own CIGI, so the
bench link runs on **8010/8011** instead — see
[Terrain from the sim](#terrain-from-the-sim) for why. Only the four numbers
change; everything else is the same.

### Local bench, one command

Nothing has to be started by hand. `-Local` is the whole-bench-on-one-PC preset
— the loopback adapter and the 8010/8011 pair — and the script does the rest:

```
.\tools\test-hathot-emulator.ps1 -Local
```

In order, it:

1. says what the run has to work with: the link it is about to use, the capture
   device, Wireshark's `dumpcap`, whether Prepar3D is running, and whether the
   terrain probe is loaded;
2. brings the bench up. CigiStarter is started as `CigiStarter.exe --host-ip
   127.0.0.1 --ig-ip 127.0.0.1 --ig-port 8010 --host-port 8011` unless something
   is already listening, and the emulator is pointed at the same pair through its
   registry key and started unless it is already up. A running emulator saved to
   a different port is restarted, because it reads that setup once, at startup. A
   bench that is already up is used as it is;
3. runs the checks below, capturing each exchange on the loopback adapter with
   `dumpcap` and decoding it with `tshark`;
4. prints the per-check table, then the verdict.

```
================================================================
  RESULT: PASS   --   35 passed, 0 failed, 0 skipped
================================================================
```

That `PASS`/`FAIL` line is last, in green or red, and the exit code matches it —
0 for PASS, 1 for FAIL — so a run can be automated:

```
.\tools\test-hathot-emulator.ps1 -Local
if ($LASTEXITCODE -ne 0) { throw 'the HAT/HOT bench failed' }
```

A check whose prerequisite is missing is **skipped**, with the reason, and not
counted as a pass: with Prepar3D closed, the two sim-dependent checks report SKIP
and the run still ends `PASS`. `0 skipped` needs Prepar3D running *and* the
terrain probe loaded.

`-Stop` takes the bench down once the checks are done. It stops only what the run
itself started — CigiStarter through its own EXIT button, the emulator through
its window close — and leaves a bench the operator brought up alone:

```
.\tools\test-hathot-emulator.ps1 -Local -Stop
```

Prepar3D is the one thing it will not start: launch it first if the sim-backed
checks matter. The captures are left in `%TEMP%\cigi-hathot-tests` (change that
with `-Out`), and a failed check's capture is the one to open in Wireshark.

To test a bench that is already up without the script touching it, `-NoLaunch`
requires it and fails with what is missing instead of starting it. On a bench
where the deployment's 8000/8001 pair is free, drop `-Local` and give the ports:

```
.\tools\test-hathot-emulator.ps1 -NoLaunch
.\tools\test-hathot-emulator.ps1 -IgPort 8010 -HostPort 8011
```

### What the checks cover

The script drives the emulator's *Send HAT/HOT Request* dialog through a set of
requests and asserts on the answers that come back. Each check is one a wrong
answer would break: that a request is answered, and answered exactly once (or
every *n*-th frame when it carries an update period); that the request id and
type pick the right response message; that `HAT = request altitude − HOT`, which
is what catches the two doubles packed in the wrong place; that `Valid` is set
only when a terrain source could answer; that an entity-relative request is
answered at the aircraft's position; and, when the sim's terrain probe is loaded,
that the height on the wire is the probe's own reading for the very point that
was asked about.

### When the bench does not come up

| What the run says | What it means |
|---|---|
| `UDP port 8000 is held by Prepar3D … not CigiStarter` | Prepar3D's own CIGI has the port — run with `-Local`, or pass another `-IgPort` |
| `dumpcap.exe not found` | Wireshark is not installed, or not on `PATH`; its `dumpcap` and `tshark` are the capture and the decoder |
| `CigiStarter.exe not found at …` | build Release x64 first — see [Building](#building) |
| `CigiStarter did not come up on UDP port 8010` | the bind was refused, usually by the firewall, or the exe predates the `--ig-port` option |
| requests but no answers, on every check | the emulator is pointed at another port; check its `Remote Port` key, or let the script restart it rather than pairing a stale emulator with `-NoLaunch` |
| two `SKIP`s | Prepar3D is not running, or its `ACTIVE` entry is off for the probe check |
| `LNK1104 … CigiStarter.exe` while rebuilding | a CigiStarter from an earlier run still holds the file — stop it with `-Stop`, or from its own window |

### Watching the wire by hand

`tools\capture-hat-hot.ps1` is the same decode without the assertions. It
captures the link with Wireshark's `dumpcap`, decodes it with `tshark`'s CIGI
dissector, and prints every HAT/HOT request and every answer to it — pairing
several answers in one datagram with the requests that asked for them:

```
.\tools\capture-hat-hot.ps1                      # take a capture, then decode it
.\tools\capture-hat-hot.ps1 -Capture cap.pcapng  # decode one already taken
```

Both tools need Npcap (which the fan-out needs anyway) and Wireshark's
`dumpcap`/`tshark`.

## SOF rate and flow control

The HOST owns the frame rate: it answers each SOF with one datagram of flight
data, and it cannot produce frames faster than its own simulation runs. The
reference capture (`Host-Master.pcapng`) shows what that means in practice — the
HOST emits a datagram every ~10.2 ms (~98 Hz) while its own frame counter only
advances ~53 times a second, i.e. the Master IG was asking for roughly twice as
many replies as the HOST had frames to give.

That is why:

- the SOF rate defaults to **150 Hz**: every answer carries a fresh attitude (the
  position comes on every other one), so how often the IG asks is how smoothly the
  aircraft moves, and
- **flow control is on by default**: the transmit loop holds a SOF back while the
  previous one is still unanswered (`SofPacer.h`), so the stream fits whatever the
  HOST on site can absorb, whatever number is typed in the rate box.

With flow control on the configured rate is an upper bound, not a promise: when
the HOST keeps up the stream runs at the rate in the box, and when it cannot the
SOF stream follows the HOST down instead of piling requests up on it. A frame
that is held back is dropped, never replayed, so no mode of operation can turn
the stream into a burst. Two cases keep the link usable on their own: a HOST that
has never answered — at startup, before it has been woken — is transmitted to at
the full rate, and a SOF that has gone unanswered for 100 ms is followed by
another one anyway, so a HOST that has dropped out still receives something to
wake up on.

That limit is a wall-clock one rather than a number of frames, and that is what
stops a mistyped rate from becoming a load: a HOST answering more slowly than
once per 100 ms sees at most ~10 requests a second, whether the rate box says 150
or 6000. Counting frames instead would make the retries scale with the rate,
i.e. exactly the runaway this is here to prevent.

`--no-flow-control`, or clearing the window's check box, transmits on the rate
alone — what the original Master IG did, and the mode to use when measuring what
a HOST really tolerates:

```
CigiStarter.exe --no-flow-control --fps 100
```

The window's `Stream` line always names the mode in force, and says when it is
the HOST that is setting the pace: `paced to the HOST, which is the limit`.

**Why the rate matters at all.** The SOF has exactly one live field, the *HOST's*
frame counter echoed back (see [Protocol notes](#protocol-notes)). An IG that
counted its own transmitted SOFs in that slot instead would drift away from the
HOST a little further every second — at 100 Hz against a 53 Hz HOST, about 47
frames a second — which is what used to break the HOST when the rate was set
above 60 Hz. Echoing the HOST's counter keeps the outgoing SOF byte-identical to
the packet the reference Master sent.

## HAT/HOT answering

The HOST drives its aircraft altitude to the **Height Of Terrain (HOT)** that
the IG reports, so an IG that answers `HOT = 0` flies the aircraft into the
ground. CigiStarter therefore reports the terrain **by default** — turn it off
in the window, or start with `--no-respond-hathot`, if you must:

| Request                | Response                                      |
|------------------------|-----------------------------------------------|
| Type 0 (HAT) / 1 (HOT) | HAT/HOT Response (id 102, 16 bytes)           |
| Type 2 (Extended)      | HAT/HOT Extended Response (id 103, 40 bytes)  |

There are three terrain sources, in this order:

1. **Prepar3D's own terrain**, through the `PdkTerrainProbe` add-on
   ([below](#terrain-from-the-sim)) — the PDK's surface query service, asked for
   the point the HOST is asking about. It is the same terrain the sim will
   actually fly the aircraft onto, so a runway's flattening and any added
   scenery are included, and nothing has to be pre-mapped or re-baked when the
   scenery changes.
2. **The terrain database** ([below](#terrain-database)) — an offline SRTM
   `.hgt` tile set. It answers a request from the latitude and longitude alone,
   so it stays correct for the whole flight too, but it is a copy of the world
   and cannot know about a flattening or changed scenery.
3. **The local Prepar3D via FSUIPC** — the ground under the *player* aircraft
   (`0x0020`). It stands in for any request neither of the above can cover, and
   it supplies the aircraft's own position and altitude either way.

Why a database exists here at all: a program that drives the aircraft into
Prepar3D by writing its position over FSUIPC (CigiP3dBridge, or the vendor
program that replaces it) has to hold the sim in **slew mode**, and Prepar3D
**freezes its ground model in slew**. The FSUIPC ground altitude then stops
following the aircraft and stays at wherever the last reposition left it, so an
IG answering from FSUIPC alone keeps reporting the same terrain however far the
aircraft flies — and the HOST flies it on until it meets rising ground. The
original Master IG answered from its own database for that same reason. Both
sources 1 and 2 are answers about the *world* rather than about where the
aircraft is sitting, so neither can freeze.

The fields themselves:

- `HOT` = the height from the first source that can answer: the sim's surface
  query at the request's point, else the database height there (bilinear between
  the four surrounding samples), else FSUIPC `0x0020` with a geodetic request
  accepted only within 500 m of the aircraft.
- `HAT` = test-point altitude − `HOT`. Entity-relative requests use the
  aircraft's altitude minus the request's Z offset (CIGI entity axes are local
  NED, so **+Z points down**); geodetic requests carry the altitude themselves.
- Entity-relative requests are answered at the **aircraft's** position: the
  HOST's probe points all sit within ~10 m of it (the reference capture's six
  points reach 10.4 m back and 3 m forward), far below any terrain source's
  resolution. The sim's own query is asked for the exact point all the same.
- `Valid` is set to 0 — never a fabricated 0 m — when no source can produce an
  answer: a request for an entity other than the ownship (entity 0), or a point
  no source can cover.
- `updatePeriod = 0` (one-shot) requests are answered once; a non-zero period is
  honoured by re-answering every *n*-th frame until a fresh request for the same
  HAT/HOT ID replaces it — that one is answered at once and its count restarts.

The FSUIPC link is polled by a background thread that owns every FSUIPC call, so
the SOF pacing is never delayed by an FSUIPC round trip. The poller runs only
while answering is switched on — the window's check box starts and stops it, and
while it is off this program makes no FSUIPC calls at all. With a terrain
database present a missing Prepar3D is no longer fatal to the answer: the
database still answers, and `Valid = 0` is returned only for points it cannot
cover.

### Terrain from the sim

`tools\PdkTerrainProbe` is a small **PDK add-on DLL** that runs *inside*
Prepar3D. The PDK exposes a free-standing surface query
(`ISurfaceQueryManagerV400`) — not tied to any object, so no probe object has to
be created or teleported around the world — and the add-on asks it for each
point CigiStarter wants, then publishes the answers through a block of shared
memory (`TerrainProbeIpc.h`, `TerrainProbe.h`). No FSUIPC, no per-scenery
mapping, and a newly added airport is answered the moment the sim can see it.

It answers HAT/HOT whenever it is loaded, because it is the surface Prepar3D
will actually place the aircraft on.

- **Installing it** (on the IG PC): copy `PdkTerrainProbe.dll` and its
  `add-on.xml` into Prepar3D's default add-on discovery folder, one level deep —
  `%USERPROFILE%\Documents\Prepar3D v6 Add-ons\PdkTerrainProbe\` (or
  `Prepar3D v5 Add-ons` on v5) — and restart Prepar3D. That Documents folder is
  what Prepar3D discovers; the `add-ons` folder under
  `%PROGRAMDATA%\Lockheed Martin\Prepar3D v6\` is **not** scanned. Then
  `tasklist /m PdkTerrainProbe.dll` should list Prepar3D — if it does not, the
  package is probably there but switched **off**: Prepar3D keeps an `ACTIVE`
  entry per package in `%APPDATA%\Lockheed Martin\Prepar3D vN\add-ons.cfg` and
  shows the same switch in its **Options → Add-ons** dialog, and a disabled
  package is never loaded however right the folder looks. See
  `tools\PdkTerrainProbe\README.md`.
- **On a Professional Plus sim, the sim itself holds 8000.** Prepar3D's CIGI
  feature ships with the **Professional Plus** edition only, so the deployment's
  Prepar3D Professional never has it: the IG owns 8000/8001 there with nothing to
  contest it. A Professional Plus sim does have it — this bench machine's v5 is
  one — and its CIGI is already pointed at those ports (`SendPort=8000`,
  `ReceivePort=8001` in
  `%APPDATA%\Lockheed Martin\Prepar3D v5\CigiConfig.cfg`), so `Prepar3D.exe`
  holds `0.0.0.0:8000` from launch whether or not a CIGI session was started
  (`netstat -ano -p udp | findstr :8000` shows it). Point the sim's CIGI
  elsewhere in that file, or leave the sim alone and run the bench on other ports
  (`--ig-port` / `--host-port`, see [Usage](#usage)).
- **It has only ever run on the bench's sim.** The add-on is a PDK add-on, and
  the only sim it has been loaded into is the bench's Professional Plus v5. The
  deployment's Prepar3D Professional has not been tried yet, so whether that
  edition accepts it is still open; if it is refused, the answers come from the
  database ([below](#terrain-database)) and nothing else changes.
- **Not having it** is not an error: CigiStarter opens the shared region lazily
  and re-tries every 2 s, so the two may be started in either order, and with no
  add-on the answer simply comes from the database as before.
- **Building it** needs the Prepar3D SDK (v5 or v6 — the surface query is the
  same in both, but the object manager it ticks from is `V520` on v5 and `V540`
  on v6, and the project picks the right one from the SDK path), but *running*
  it does not, which is why it is a separate project rather than part of
  `CigiStarter.sln`.
- **`--terrain-offset` is not applied to a height that came from the sim.** That
  option calibrates the *database's* datum against the sim; a value from the sim
  needs no calibration, and applying it would correct it twice.
- An answer is only ever used for the point it was taken at: every reading
  echoes the latitude and longitude it was measured for, so a request that has
  moved in the meantime is refused and falls back rather than being answered
  with the terrain of wherever the aircraft used to be.
- **A point the sim has not answered yet is waited for, briefly.** The add-on
  answers on its own tick, so the first request for a point lands 30–60 ms
  behind the query (79 ms worst seen on v5). Rather than drop that one reply
  onto the database, CigiStarter waits up to 120 ms for it and stops the moment
  it arrives. It is paid once per point — every later reply for that point is
  already in the table — so the steady state never waits.

### Terrain database

The first fallback, when the sim's own probe cannot answer, is an offline
**SRTM** (Shuttle Radar Topography Mission) tile set: public-domain radar
terrain, 1 or 3 arc-second, one 1° × 1° cell per file, dropped in a folder next
to the program.

```
CigiStarter.exe
terrain\
    S07E106.hgt     covers  7..6 S, 106..107 E   (Jakarta)
    S08E112.hgt     covers  8..7 S, 112..113 E   (Surabaya)
client.txt
```

- **Which files.** Raw SRTM `.hgt`, named by the integer-degree corner the tile
  starts at: `N06E106` covers 6..7 N / 106..107 E, `S08E112` covers -8..-7 /
  112..113 E. Both **SRTMGL1** (1 arc-second, ~30 m, `3601 × 3601`) and
  **SRTMGL3** (3 arc-second, ~90 m, `1201 × 1201`) are accepted; the sample
  count is taken from the file size. Samples are signed 16-bit big-endian
  metres, row 0 the northern edge, column 0 the western edge, `-32768` marking a
  void (interpolated around, or reported invalid when a point has nothing but
  voids).
- **Getting them — two options.**
  1. **Plain SRTM.** `tools\get_terrain.ps1` downloads the tiles you name, or
     works them out from positions, from the public AWS *skadi* mirror and
     gunzips them:

     ```powershell
     .\tools\get_terrain.ps1 -Points "-6.13,106.66","-7.38,112.79"
     .\tools\get_terrain.ps1 -Tiles S07E106,S08E112 -Out D:\ig\terrain
     ```

     Each 1 arc-second tile is about 25 MB uncompressed, so fetch only the box
     the aircraft flies in. Tiles already present are left alone. This is the
     generic world model: quick, and nothing but a download — but SRTM predates
     the scenery, so it knows nothing about a strip of ground Prepar3D levelled
     for a runway.

  2. **Sim-exact bake.** `tools\TerrainBaker` overlays the *simulator's own*
     ground heights on the SRTM base, exactly where the aircraft flies. It walks
     a grid over the chosen boxes, asks Prepar3D for its own surface height at
     each point through the same `PdkTerrainProbe` add-on the program uses live,
     and writes the result back out as an ordinary `.hgt` tile: SRTM everywhere,
     sim-exact at the fields, runway flattening and all. This program reads the
     output unchanged.

     ```powershell
     .\TerrainBaker plan      # tiles, point counts, rough cost -- no sim needed
     .\TerrainBaker probe     # with Prepar3D up: is the add-on answering?
     .\TerrainBaker survey    # the sweep; interruptible and resumable
     .\TerrainBaker compose   # SRTM base + samples -> terrain-sim\*.hgt
     ```

     Run `probe` first: it asks about a few spread-out points, twice, and prints
     sim against SRTM. No FSUIPC and no repositioning are involved — the surface
     query is answered about the world, so the whole sweep takes seconds — but
     `probe` and `survey` need Prepar3D running with the add-on loaded. `plan`,
     `compose` and `--help` need neither. A bake builds on option 1 — fetch the
     SRTM tiles first, since they are its base. `tools\TerrainBaker\README.md` is
     the full procedure, with the expected output of each step and the failure
     modes.

  Either way, a tile is read into memory the first time it is asked about and
  kept; the window's `TerrainDB` line shows how many are indexed and how many
  are held.
- **Where.** `terrain\` next to the executable by default, or **`--terrain-dir
  <dir>`**. With no tiles at all the program simply falls back to FSUIPC, and
  the window says so.
- **Calibration.** SRTM heights are orthometric (EGM96) and the sim's datum is
  its own; the two matched to ~2 m at Juanda in testing. If a site shows a
  constant bias, **`--terrain-offset <m>`** adds a fixed number of metres to
  every database height. It affects the database only, never the FSUIPC
  fallback.

## Operator window

CigiStarter is a windowed program, so it is driven with the mouse rather than
from a console. The window is a fixed-size plain Win32 dialog:

The stream is already running by the time the window is up, and the window
minimises itself straight away: a deployment site switches the IG PC on rather
than operating it, so there should be nothing to click before the link is live.
The window waits in the taskbar, and START / STOP / EXIT work as soon as it is
brought back — EXIT is still the only way out. `--no-auto-start` starts stopped
with the window up instead, which is the behaviour the sketch below shows.

```
+--------------------------------------------------------------------------+
|  HOST 192.168.70.2:8001   ->  SOF destination                            |
|  IG   :8000   (this IG is 192.168.70.70)                                 |
|                                                                          |
|  Stream    : RUNNING (paced to the HOST)                                 |
|  SOF rate  : 150 Hz requested, 150/s measured                            |
|  SOF frames: 101                                                         |
|  Received  : 0 datagrams (0/s), 0 entity updates                         |
|  IG mode   : 0 (Standby)  <- commanded by the HOST                       |
|  HOST frame: 0                                                           |
|                                                                          |
|  Relay     : 4 clients, 4 reachable, 1567 datagrams forwarded            |
|  Relay NIC : Killer E2600 Gigabit Ethernet Controller                    |
|                                                                          |
|  TerrainDB : 2 tile(s), 1 in memory, 25 MB                               |
|  HAT/HOT   : answering from the sim's own terrain                       |
|  Terrain   : 27.0 m under the aircraft, 30.1 m aircraft                  |
|                                                                          |
|  [x] Answer HAT/HOT                       SOF rate: [ 150 ] Hz [ Apply ]   |
|  [x] Pace SOF to HOST replies (flow control)                             |
|                                                                          |
|  [ START ]  [ STOP ]                                              [ EXIT ] |
+--------------------------------------------------------------------------+
```

- The status block refreshes four times a second. The datagram and SOF rates are
  sampled once a second from the cumulative counters; the SOF line shows the
  rate that is *requested* next to the rate the transmit loop actually achieved,
  and the `Stream` line names the pacing mode — `paced to the HOST` or
  `free-running` — adding `which is the limit` when it is the HOST that is
  setting the pace rather than the rate box.
- **START** / **STOP** start and stop the SOF stream. The stream is already
  running when the window appears (see above), so START is the disabled button at
  launch and STOP is the live one. The button that would be a no-op is always the
  disabled one.
- **Answer HAT/HOT** switches HAT/HOT answering on and off, and with it the
  FSUIPC poller — switching it off means this program makes no FSUIPC calls at
  all, switching it back on reconnects. It starts **on**, unless
  `--no-respond-hathot` was given. (The terrain database is indexed at startup
  regardless, so the window can always report it.)
- **Pace SOF to HOST replies** is the [flow
  control](#sof-rate-and-flow-control) switch. It starts **on**, unless
  `--no-flow-control` was given. Switch it off to transmit on the rate alone —
  for instance to measure what the HOST really tolerates.
- **SOF rate** is a whole number of Hz, from 1 to 100000; it starts at **150**.
  It is applied to the transmit loop on the next frame, so the stream can be
  retuned *while it runs* — no stop/start needed. Commit it with **Apply** or by
  pressing **Enter**. A value that is not a whole number in range is refused with
  a warning and the box snaps back to the rate actually in force; when the box
  merely loses focus a valid value is applied quietly and an unfinished one is
  left alone, so clicking EXIT never nags. `--fps` only seeds it. With
  [flow control](#sof-rate-and-flow-control) on, this is the ceiling: the stream
  follows the HOST down when the HOST cannot keep up.
- **EXIT** is the **only** way out. The title-bar **X** and **Alt+F4** are
  deliberately swallowed — `SC_CLOSE` is deleted from the window's system menu
  (Windows then draws the X greyed out and non-clickable) and `WM_CLOSE` is
  ignored — so the IG cannot be taken off the link by a stray click. This
  matches the `CigiP3dBridge` GUI, which follows the same rule. The window
  disappears at once; the process itself is gone a moment later at the latest,
  since shutting down waits only for the [fan-out](#fan-out-to-the-other-ig-pcs)
  thread to finish the client-address lookup it is already in the middle of.
- Since the process has no console, `Ctrl+C` cannot reach it either.
- Terrain lines appear while answering is on. `HAT/HOT` names the source in
  force, in the order the answer is taken from: `answering from the sim's own
  terrain` when the `PdkTerrainProbe` add-on is loaded in the local Prepar3D,
  else `answering from the terrain database` when tiles are indexed, else
  `answering from FSUIPC`, else `switched on, but nothing to answer from`.
  `TerrainDB` names the database behind that fallback — `N tile(s), M in
  memory, X MB`, or `none` with the reason (for instance no `terrain\` folder)
  — and `Terrain` shows the live FSUIPC reading (ground and aircraft heights)
  or, with no sim, `FSUIPC : no link to Prepar3D`. With answering off the block
  collapses to `not answered (switched off)`.
- `Relay` and `Relay NIC` report the [fan-out](#fan-out-to-the-other-ig-pcs):
  how many clients are listed, how many of them answer, how many datagrams have
  been copied, and which adapter the copies leave by. There is no button for it
  — it is on whenever `client.txt` says so.

## Fan-out to the other IG PCs

CigiStarter can copy the HOST's downstream stream to extra IG PCs, so several
visual channels can be driven from the one link the HOST already feeds. The list
of extra PCs is a plain text file, **`client.txt`, next to the executable**. It
is read once at startup — restart the program to pick up an edit:

```
# one address per line, with an optional port (default: the IG port, 8000)
192.168.70.102:8000
192.168.70.103
192.168.70.104:8000   # left visual
```

- Case by case: blank lines, `#` comments (whole-line or trailing) and a Notepad
  byte-order mark are all tolerated; IPv4 addresses only.
- Unusable lines — junk, a port of 0 or above 65535, a duplicate, this PC's own
  addresses — are skipped and **counted**, and the window says how many were
  ignored. The list is capped at 32 clients.
- The file is optional. Without it (or with nothing usable in it) nothing is
  relayed, the window says which of those it is, and the capture driver is never
  opened.

Every datagram the HOST sends is copied **exactly as it arrived**, on the same
UDP port, to each listed client — one Ethernet frame per client per datagram.
The copies carry the **HOST's address and port as their source**
(`192.168.70.2:8001`), so a client's bridge needs no reconfiguration at all: it
sees the same stream it would have seen if it were the IG on the link. Nothing
is ever sent back up the link, and clients never talk to CigiStarter.

Writing the copies needs **Npcap** (`wpcap.dll`) and nothing else. It is loaded
at run time — deliberately *not* linked — so CigiStarter still builds and runs as
a plain IG on a PC without it (the window then reports the relay as off, and
why). The program asks for no elevation, and on a normal Npcap install (the
*restrict driver access to administrators* option left unchecked at install
time) the adapter opens unelevated; if it cannot be opened, the window reports
why and nothing is forwarded, while the SOF link to the HOST is unaffected.
Npcap has to be installed on the IG PC for the fan-out to work.

How it works, and why:

- **The adapter is found automatically**: the one that owns this IG's address
  (`192.168.70.70`) — or, on a machine that does not have it, such as a test
  bench, whichever adapter the route to the HOST leaves by. No flag; the window
  names the adapter it picked.
- **Each copy is a hand-built Ethernet + IPv4 + UDP frame**, injected with
  `pcap_sendpacket`. Ethernet destination = the client's MAC; Ethernet source =
  this adapter's *own* MAC (spoofing that as well would teach every switch on
  the way that the HOST lives on this port); IP source = the HOST; IP
  destination = the client; UDP source = the HOST's port; UDP destination = the
  client's. Both the IP and the UDP checksum are computed, over the IP
  pseudo-header as the UDP one must be, so a copy is a complete, valid datagram
  rather than something only a capture driver would show.
- **Unicast, one frame per client** — not broadcast — so only the PCs named in
  `client.txt` see the copies.
- **Client MACs are resolved with `SendARP` on a background thread**, re-resolved
  every few seconds, so a client that is switched on or replaced later is picked
  up on its own. One that stops answering is taken out of the fan-out until it
  answers again; the window counts how many are reachable.
- **Datagrams that would have to be fragmented are dropped and counted**
  (anything over 1472 bytes of payload). A client's capture filter matches one
  datagram, not a fragment chain, so sending the first fragment of a truncated
  CIGI stream would be worse than not sending it.
- **The receive path is untouched**: only datagrams whose source address is the
  HOST's are copied at all, and the copy is taken before anything is parsed, so
  clients get precisely what the HOST sent.

The `Relay` line in the window is the whole state, and it never needs a button:

```
  Relay     : 5 clients, 4 reachable, 1250431 datagrams forwarded, 2 dropped, 1 client.txt line ignored
  Relay NIC : Killer E2600 Gigabit Ethernet Controller
```

`forwarded` counts datagrams that reached at least one client; `dropped` counts
datagrams that reached none (all of them too big, or every send failing while a
client was reachable). Clients that have not answered yet are neither.

## Network layout

The nodes and the full diagram are in `agents.md`; this is the traffic
CigiStarter puts on the link:

| Direction   | From                  | To                    | Port | Payload                         |
|-------------|-----------------------|-----------------------|------|---------------------------------|
| Downstream  | HOST `192.168.70.2`   | IG `192.168.70.70`    | 8000 | IGCtrl, EntityCtrl, HAT/HOT req |
| Upstream    | IG `192.168.70.70`    | HOST `192.168.70.2`   | 8001 | SOF (+ HAT/HOT responses)       |
| Fan-out     | HOST `192.168.70.2`   | each `client.txt` PC  | 8000 | one copy of each downstream datagram |

The fan-out row is the only traffic this program puts on the link labelled with
somebody else's address: the copies claim to come from the HOST on purpose, so
the clients need no reconfiguration. See
[Fan-out](#fan-out-to-the-other-ig-pcs). The addresses shown are the deployment's
defaults; see [Usage](#usage) for the bench-test overrides.

## Protocol notes

- **CIGI version 3.3**, little-endian on the wire.
- A message is framed as `[message-id(1)][message-size(1)][payload...]` where
  `size` is the total byte length including the two header bytes.
- The SOF is `65 18 03 00 00 <b5> 00 80` + a 4-byte field + 12 zero bytes, where
  byte 5 packs the CIGI minor version (bits 4-7; 3.3 => `0x30`) together with the
  **IG mode** (bits 0-1). The `0x00 0x80` at offsets 6-7 is the byte-swap flag
  (little-endian).
- That 4-byte field at offset 8 is the **HOST's frame counter**, echoed out of
  the IG Control it arrived in — *not* a count of the SOFs this program has sent.
  It is the only SOF field that ever changes, and in `Host-Master.pcapng` it
  advances ~0.52 per SOF, i.e. at the HOST's own frame rate (~53 Hz) rather than
  at the SOF rate (~100 Hz, an 8/12 ms alternation). Offsets 12 and 16
  (timestamp, last-host-frame) were left at zero by the reference Master and are
  still zero here, which makes the outgoing SOF byte-identical to the captured
  one — see [SOF rate and flow control](#sof-rate-and-flow-control) for why the
  field matters.
- The IG never picks its own mode: the HOST commands it with IG Control and the
  SOF reports it back. CigiStarter follows that, so byte 5 becomes `0x31`
  (Operate) as soon as the HOST asks for Operate — which the reference HOST
  does in every one of its datagrams. `--ig-mode` only sets the mode reported
  *before* the first IG Control arrives.
- SOF pacing uses the 1 ms system timer resolution (`timeBeginPeriod(1)`).
  Windows' default granularity is 15.6 ms (~64 Hz), so without it any rate above
  ~60 Hz is really paced at ~64 Hz with an occasional burst. That is worth
  knowing when comparing rates against another tool: a replay loop built on
  `Sleep(8)`/`Sleep(12)` asks for 100 Hz and gets ~64. CigiStarter raises the
  timer so that the rate it is asked for is the rate it sends, and
  [flow control](#sof-rate-and-flow-control) then keeps that stream inside what
  the HOST can absorb.
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
| `SofPacer.h`       | flow-control rule: whether a due SOF may be sent |
| `Gui.h`            | operator-window API: status snapshot + start/stop/HAT-HOT/flow-control/rate/quit hooks |
| `Gui.cpp`          | the plain Win32 window (status block, START / STOP / EXIT, HAT/HOT + flow-control check boxes, SOF rate box) |
| `cigi.h`           | CIGI 3.3 message IDs, sizes, pack/parse API    |
| `cigi.cpp`         | explicit little-endian message packing         |
| `TerrainSource.h`  | FSUIPC-backed aircraft/terrain snapshot API (aircraft position + fallback terrain) |
| `TerrainSource.cpp`| the FSUIPC poller thread (owns all FSUIPC use) |
| `TerrainDb.h`      | offline SRTM `.hgt` terrain database API — the HAT/HOT fallback when the sim's own probe cannot answer |
| `TerrainDb.cpp`    | the `.hgt` reader: tile index, lazy load, bilinear lookup |
| `TerrainProbe.h`   | API to the sim's own terrain, through the `PdkTerrainProbe` add-on — the preferred HAT/HOT source |
| `TerrainProbe.cpp` | CigiStarter's half of the shared-memory link to `PdkTerrainProbe.dll` |
| `TerrainProbeIpc.h`| the shared-memory contract between CigiStarter and the add-on |
| `Relay.h`          | fan-out API: start/stop, per-datagram copy, status snapshot |
| `Relay.cpp`        | the fan-out itself: `client.txt`, frame building, Npcap injection, client MAC lookup |
| `client.txt`       | optional, next to the executable: the extra IG PCs to copy the stream to |
| `terrain\`         | optional, next to the executable: the SRTM `.hgt` tiles the HAT/HOT fallback is answered from |
| `external\`        | vendored third-party code: the build-time SDKs (FSUIPC, Npcap, Prepar3D PDK headers) and the CIGI Host Emulator the bench tests drive |
| `external\HostEmulator_Win32_v3.3.3\` | the CIGI Host Emulator 3.3.3, the HOST side of a bench link (not linked by anything) |
| `tools\PdkTerrainProbe\` | the PDK add-on that asks Prepar3D's own terrain for the height (preferred source, and the sampler `TerrainBaker` uses) |
| `tools\get_terrain.ps1` | option 1: downloads generic SRTM tiles for `terrain\` |
| `tools\capture-hat-hot.ps1` | bench aid: captures the CIGI link and prints every HAT/HOT request and answer |
| `tools\test-hathot-emulator.ps1` | bench aid: drives the Host Emulator through the HAT/HOT checks and prints a PASS/FAIL verdict; `-Local` is the whole bench on one PC |
| `tools\TerrainBaker\` | option 2: bakes Prepar3D's own ground heights over an SRTM base |
| `app.manifest`     | UAC `asInvoker` manifest (runs unelevated)   |
| `agents.md`        | description of the HOST / IG / CigiStarter agents |
| `CigiStarter.sln`  | Visual Studio solution (x64 only)              |
| `CigiStarter.vcxproj` | VS project (x64 only, static CRT, ws2_32, FSUIPC) |
| `CigiStarter.vcxproj.filters` | project file organization            |
| `.gitignore`       | ignores build output and VS artifacts          |

See `agents.md` for a description of each agent and the message flow.
