# Agents

The **agents** (nodes) in the CIGI flight-simulator network and how they
interact. The reference behaviour was reverse-engineered from
`Host-Master.pcapng`.

For how CigiStarter is built, configured and operated, see `README.md`; this
document is the network view.

## Topology

```
                     Downstream (control / flight data)   port 8000
   ┌──────────────┐ ───────────────────────────────────────────►  ┌────────────────────┐
   │              │                                               │                    │
   │     HOST     │                                               │  IG (image gen.)   │
   │ 192.168.70.2 │                                               │  192.168.70.70     │
   │              │ ◄───────────────────────────────────────────  │                    │
   └──────────────┘   Upstream (SOF + HAT/HOT responses) port 8001└────────────────────┘
                                                                          │
   ┌──────────────────────────────────────────────────────────────────────┘
   │  the same downstream datagrams, copied by the IG for the extra IG PCs
   ▼  listed in client.txt (they are passive: nothing ever comes back)
 ┌─────────────────────────────────┐
 │ 192.168.70.102 ... .106  :8000  │
 └─────────────────────────────────┘
```

The copies keep the HOST's address and port as their source, so each extra IG PC
runs the ordinary bridge with no reconfiguration and sees exactly the stream the
HOST sent. `--no-relay-spoof` sends this IG's own address and port instead, for a
client-side driver that follows the IG rather than the HOST.

## Agents

### HOST — `192.168.70.2`
The simulation host. It runs the aircraft/flight model and owns the
authoritative entity state. It **does not send anything until the IG first
sends a Start-of-Frame**, so it is effectively a request/response slave.

Per frame, in response to a SOF, it transmits a single UDP datagram to the IG
(`192.168.70.70:8000`) containing:

| Message                | Count | Purpose                                   |
|------------------------|-------|-------------------------------------------|
| IG Control             | 1     | IG mode (Operate) + frame counter         |
| Entity Control         | 3     | entity pose (roll/pitch/yaw, lat/lon/alt) |
| HAT/HOT Request        | 1     | terrain height request                    |
| View Control           | 1     | view definition                           |
| Articulated Part Ctrl  | 5     | articulated parts                         |
| Rate Control           | 8     | motion rates                              |
| Atmosphere Control     | 1     | first frames only (warm-up)               |

On a bench there is no flight model: a **CIGI Host Emulator**
(`external\HostEmulator_Win32_v3.3.3\`) runs instead, on the same PC as
CigiStarter, to send the requests and watch the answers. It stands in for the
HOST only as far as this program is concerned — it produces none of the flight
data above — and `README.md`, [Bench testing](README.md#bench-testing), has its
setup and the automated tests built on it.

### Master IG — `192.168.70.70`
The original Image Generator that CigiStarter replaces. In the capture it is the
frame-timing master: it emits a **SOF** at ~100 Hz (an 8/12 ms alternation) and,
after a ~100-frame warm-up, appends **HAT/HOT Extended Response** messages. Its
SOF frame-number field advances only ~53 times a second — it carries the HOST's
own counter, not a count of SOFs.

### CigiStarter (this program) — the new IG PC
A drop-in replacement for the Master IG. It plays the IG side of the link:

- **Wakes the HOST and answers it** — binds `0.0.0.0:8000` for the flight data,
  sends SOF to `192.168.70.2:8001` at a configurable rate, and rides its HAT/HOT
  responses along on the SOF stream. Those are the hardcoded defaults in
  `src\main.cpp`; `--host-ip` / `--ig-ip` and the matching ports repoint the link for
  a bench test, and the deployment passes none of them.
- **Answers HAT/HOT** from the terrain the aircraft will actually fly onto —
  Prepar3D's own engine, through the `PdkTerrainProbe` add-on — falling back to
  an offline SRTM database and then to FSUIPC. On by default. Entity-relative
  requests are anchored to the HOST's own Entity Control pose (entity 0), not to
  the FSUIPC read-back; FSUIPC is only the fallback before one has arrived. The
  rig holds Prepar3D paused throughout, and on v6 a pause stops Prepar3D's own
  ticks, so there the add-on answers from a thread of its own.
- **Fans the downstream stream out** to the extra IG PCs in `client.txt`, byte
  for byte, source-labelled as the HOST (`--no-relay-spoof` labels them as this
  IG instead). Optional, Npcap-backed, passive clients only.
- **Starts on its own** — streams from launch, minimises its window, and leaves
  EXIT as the only way off the link. `--no-auto-start` restores the paused
  window.

Three design points are where it differs from a naive IG, and all of them matter
to the HOST:

- **Flow control (default on)** holds a SOF back while the HOST has not answered
  the previous one, so the stream fits the frame rate the HOST can actually
  produce instead of piling requests up on it. The rate box is a ceiling, not a
  promise.
- **The SOF echoes the HOST's frame counter** (offset 8), not an IG-side count of
  SOFs. That is what the reference Master did; an IG-side count would drift away
  from the HOST by (rate − 53) frames every second, which is what used to break
  it above ~60 Hz.
- **Entity-relative HAT/HOT answers are anchored to the HOST's Entity Control**
  (entity 0), the pose the HOST's flight model is flying, rather than to the
  sim's FSUIPC read-back. The requests are the same six offsets wherever the
  aircraft is, so an anchor left over another airfield cannot be seen on the
  wire — it just answers the wrong terrain, and the HOST flies the aircraft to
  meet it. That is the rig's "repositions, then parks at the departure ground"
  failure: the sim link had frozen at the departure field while the HOST flew
  the aircraft to the destination.

`README.md` has the mechanism behind all of it: the operator window, every
option, the terrain sources, the fan-out and the wire layout.

## Message flow

```
  IG (CigiStarter)                          HOST
        │                                     │
        │──── SOF (frame N) ────────────────► │   ← the "request" that wakes the host
        │                                     │
        │◄─── IGCtrl + EntityCtrl + ... ──────│   ← flight data for frame N
        │                                     │
        │──── SOF (frame N+1) [+ HAT/HOT] ──► │
        │◄─── IGCtrl + EntityCtrl + ... ──────│
        │                ...                  │
```

The loop is self-limiting: flow control stops the IG asking for a frame the HOST
has not made. With HAT/HOT answering on, the IG asks the sim's own terrain engine
for each request's height before it replies; with `client.txt` populated, every
downstream datagram also leaves the IG again — unaltered — towards each client,
looking to them exactly like the HOST's own.

## Versioning

`main` carries annotated SemVer tags (`v1.0.0`, …), and a build discovers its
own version from git at build time — there is no hand-maintained version
constant to drift.

* **Tag `main` only, never a feature branch, and do it from the merge** — the
  committed `post-merge` hook cuts it for you (`## Release workflow` below). A
  branch that is an experiment needs no tag; `git describe` still names it
  against the nearest one.
* **The string is derived, not typed.** The `PreBuildEvent` in
  `CigiStarter.vcxproj` runs `tools\gen_buildinfo.ps1`, which turns
  `git describe --tags --always --dirty` into `BuildInfo.h` in the (git-ignored)
  intermediate directory — `v1.0.0` on the tag, `v1.0.0-3-g31d8f4b` three
  commits later, a bare short hash with no tag anywhere, and a `-dirty` suffix
  on an unclean tree. `src\Version.{h,cpp}` compile it in; that pair is the only
  interface, and nothing else may include `BuildInfo.h`.
* **Two readers**, because this program is windowed and has no console: the
  `--version` message box (`src\main.cpp`) and the operator window's **title**
  (`src\Gui.cpp`). There is no crash log here. Nothing on a hot path reads it.
* **Do not add a version number to a file name.** It breaks includes, the
  `.vcxproj` and `git log --follow`; the version belongs in metadata and in the
  release artefact's name. `CHANGELOG.md` holds the prose history.

## Release workflow

Branch from `main`, prove the work on the branch, merge into `main` — and
landing on `main` is the definition of proven, so the version tag is cut at
that moment, automatically. There is no hand-chosen number and no separate
release step.

1. **Branch from `main`** — never from another feature branch, and never work on
   `main` directly:

   ```
   git checkout main
   git pull
   git checkout -b feature/<name>
   ```

   `feature/` is the intent the bump later reads: a new message mapping, terrain
   source or feature (MINOR). `fix/` is a fix that changes no mapping (PATCH);
   `major/` or `breaking/` is a change to what the IG puts on the wire or
   answers (MAJOR). A branch cut from `v1.0.0` and three commits in builds as
   `v1.0.0-3-g<hash>` for free, so a bench build self-identifies without a tag.

2. **Work, test, commit on the branch.** The offline contract harness is
   `tools\PdkTerrainProbe\test\` (the terrain probe, built and run with no
   Prepar3D); `tools\test-hathot-emulator.ps1` is the bench check for what only
   a running sim can see (`README.md` → [Bench testing](README.md#bench-testing)).
   Put the changelog entry in `[Unreleased]` here; the release renames it
   (step 3).

3. **Merge into `main`.** The committed `post-merge` hook then cuts the tag —
   its message lists the commits it covers — after refreshing the generated
   documentation (see *Generated documentation* below):

   ```
   git checkout main
   git merge --ff-only feature/<name>   # or a plain merge if main has moved
   #   [docs] README.md current-build -> v1.1.0
   #   [tag] merged 'feature/<name>' -> v1.1.0
   #   [tag] publish it with:  git push origin main v1.1.0
   git push origin main v1.1.0          # the tag is local until you push it
   ```

   The hook is committed but **not active in a clone until you opt in**: hooks
   live in `.git/hooks`, so `core.hooksPath` points at the tracked `.githooks/`:

   ```
   git config core.hooksPath .githooks    # once per clone
   ```

   It never pushes: the tag is yours to review and publish, and it does nothing
   on a non-`main` branch, on a `git pull`, on a remote-tracking merge, or on a
   `--squash` merge (which commits nothing to stand a tag on).

4. **The rule is one function**, `tools\release\release.ps1`, so the hook is only
   a dispatcher and the mapping lives in one place.

   That file is **not in this repository**: it is the shared release tooling,
   mounted here as the git submodule `tools/release` and configured by
   `tools\release.config.psd1` (the only per-project file, and it holds nothing
   but words). Fix the tool once there and every project picks it up:

   ```
   git update-tooling
   git add tools/release && git commit -m "chore: update release tooling"
   ```

   The tooling's own README is `tools/release/README.md`.

Do **not**: tag a feature branch, tag by hand on `main` (the hook owns that),
choose the number by hand (name the branch instead), or move a tag that has
already been pushed.

### Generated documentation

Release facts that git already knows are **generated, not typed**, so they
cannot drift:

* `tools\release\sync-docs.ps1` rewrites the README's current-build line, which
  lives between `<!-- BEGIN GENERATED: current-build -->` and its `END` marker.
  Everything outside the markers is untouched. It reports the newest tag name
  alone — deliberately not the date or the commit count — so the release can run
  it *before* the new tag exists and still produce what a later `-Check` expects.
* The release step (`tools\release\release.ps1`, run by `post-merge`) calls it,
  then renames `CHANGELOG.md`'s `[Unreleased]` heading to `[X.Y.Z] - <date>`
  leaving a fresh empty `[Unreleased]`, and commits **only those two paths**
  (`git commit --only`), so unrelated work in the tree is never swept in. The tag
  lands on that commit, so the tagged commit is never stale.
* `.githooks/pre-push` runs `sync-docs.ps1 -Check` and refuses to publish a stale
  region (bypass with `--no-verify`). It is the backstop for a tag made by hand.

This covers only what is *derivable*. Prose — a feature's own README section,
this file, and any `TODO.md` — cannot be generated and stays a human duty: when a
feature lands, update its section (and that list, where one exists) in the same
branch.

## Key facts

| Property          | Value                        |
|-------------------|------------------------------|
| Protocol          | CIGI 3.3, UDP, little-endian |
| HOST address      | `192.168.70.2` default (`--host-ip`) |
| IG address        | `192.168.70.70` default (`--ig-ip`)  |
| Downstream port   | 8000 (HOST → IG)             |
| Upstream port     | 8001 (IG → HOST)             |
| SOF message id    | 101 (`0x65`)                 |
| IG mode           | commanded by the HOST via IG Control; the SOF echoes it back |
| HAT/HOT resp ids  | 102 (`0x66`) / 103 (`0x67`)  |
| SOF live field    | the HOST's frame counter, echoed at offset 8; offsets 12/16 stay 0 |
| Sim at the IG PC  | Prepar3D Professional (v6) — its CIGI feature is a Professional Plus extra, so the IG's 8000/8001 are not contested there |
| Sim driver        | a 3rd-party vendor program holds Prepar3D **paused** throughout, with **slew off**, positioning the sim's aircraft over FSUIPC; the sim's own aircraft altitude can stay at Prepar3D's startup value, so it is not the HOST's — the anchor is the HOST's Entity Control |
| Terrain source    | P3D's own terrain via the `PdkTerrainProbe` add-on; then an SRTM `.hgt` tileset; then FSUIPC `0x0020` |
| Entity anchor     | entity-relative HAT/HOT is anchored to the HOST's Entity Control (entity 0), a pose at offset 24/32/40 (lat/lon/alt, CIGI 3.3 layout); FSUIPC only until one has arrived |
| Terrain probe     | `tools\PdkTerrainProbe`, a PDK DLL loaded by Prepar3D; free-standing query, no SimObject; optional, and loaded on the bench's Professional Plus v5 and the rig's Professional v6. The answer carries the sim's own surface classification (hard/soft/water, and whether it is an object's *platform*); the answering policy needs no more than the full surface query already gives it — its height carries a platform's deck where the elevation-only call returned sea level — and `TerrainBaker probe` and the window's `Terrain` line show the classification |
| Terrain database  | `.hgt` tiles in `terrain\` by default or `--terrain-dir`; from `tools\get_terrain.ps1` or a `tools\TerrainBaker` bake |
| Extra IG PCs      | `client.txt` beside the executable, `address[:port]`, read once at startup |
| Fan-out           | one Npcap-injected copy per client, source address/port spoofed as the HOST by default (`--no-relay-spoof` sends this IG's) |
| Fan-out adapter   | found automatically: this IG's adapter, else the one the route to the HOST leaves by |
| Bench HOST        | the CIGI Host Emulator in `external\`, on the IG PC, in place of the flight model; `tools\test-hathot-emulator.ps1` drives it |
| Elevation         | none requested (`asInvoker`); the fan-out needs Npcap, not elevation |
| Reference SOF rate | ~100 Hz (~10 ms, an 8/12 ms alternation) |
| HOST frame rate   | ~53 Hz — its IG Control counter advances 0.54 per datagram, i.e. the HOST is asked for about twice the frames it has |
| SOF rate here     | 150 Hz default (1..100000 Hz), paced to the HOST unless flow control is off |
| Operator UI       | plain Win32 window: START / STOP / EXIT, a HAT/HOT check box, a flow-control check box, a SOF rate box |
| Start-up          | streams from launch, window minimised; `--no-auto-start` starts paused |
| Exit path         | EXIT button only — the title-bar X / Alt+F4 are swallowed |
| Process type      | GUI subsystem: no console; `--help`, `--version` and startup errors use a message box |
| Build identity    | `git describe` at build time (`tools\gen_buildinfo.ps1`), compiled in by `src\Version.{h,cpp}`; reported by `--version` and the window title |
| Version tag       | annotated SemVer on `main` (`v1.0.0`, …), cut by `.githooks/post-merge` when a branch lands; `CHANGELOG.md` holds the history |

`README.md`, [Protocol notes](README.md#protocol-notes), has the byte layout
behind the ids and ports.
