# Changelog

All notable changes to CigiStarter. The version in each heading is the
annotated git tag on `main`; a build's own string comes from `git describe`
and is reported by `--version` (see README → *Versioning*).

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and the project uses [Semantic Versioning](https://semver.org/spec/v2.0.0.html):
**MAJOR** for a change to what the IG puts on the wire or answers, **MINOR**
for a new message mapping, terrain source or feature, **PATCH** for a fix that
leaves every mapping as it was.

## [Unreleased]

## [1.0.0] - 2026-10-11

The first tagged build. It is the state of `main` as deployed on the IG PC, plus
the versioning and layout work that gave that state a name; it is tagged on
`main` by hand because it is the baseline the automatic tagging then builds on.
No link behaviour changed.

### Added
- The build names itself: `--version`, the operator window's title, and the
  generated build stamp. The string is derived from
  `git describe --tags --always --dirty` at build time by
  `tools\gen_buildinfo.ps1` (a pre-build step in `CigiStarter.vcxproj`) and
  compiled in through the generated, git-ignored `BuildInfo.h`.
  Source: `src\Version.h`, `src\Version.cpp`.
- `README.md` → *Versioning*, and this changelog.
- Auto-tagging on merge: the committed `.githooks/post-merge` cuts the next
  SemVer tag when a branch lands on `main`, with the bump read from the branch
  name (`feature/*` MINOR, `fix/*` PATCH, `major/*` MAJOR). The rule lives in
  `tools\bump-version.ps1`; opt in once per clone with
  `git config core.hooksPath .githooks`.
- Generated documentation, so the release facts cannot rot: the README's
  current-build line now lives between markers and is rewritten from the tags by
  `tools\sync-docs.ps1`, and the release step renames this changelog's
  `[Unreleased]` section to the new version — both committed into the tagged
  commit. A `.githooks/pre-push` backstop refuses to publish a stale region.

### Changed
- The C++ sources and headers moved from the repository root into `src\`, so the
  root holds only the project files, the docs and the `docs\` / `external\` /
  `tools\` folders.

### The link
- **IG side of a CIGI 3.3 link** — binds UDP `8000` for the HOST's flight data
  and sends Start-of-Frame to `8001`, which is what wakes the HOST. The SOF
  echoes the HOST's own frame counter, and flow control (on by default) holds a
  SOF back until the previous one is answered.
- **HAT/HOT answering** — from the local Prepar3D's own terrain engine through
  the `PdkTerrainProbe` add-on, falling back to an offline SRTM database and
  then to the sim over FSUIPC. Entity-relative requests are anchored to the
  HOST's own Entity Control (entity 0) rather than the sim's aircraft.
- **Fan-out** — every downstream datagram is copied to the extra IG PCs listed
  in `client.txt`, source-labelled as the HOST by default (`--no-relay-spoof`
  labels them as this IG).
- **Operator window** — a plain Win32 window with START / STOP / EXIT, a HAT/HOT
  check box, a flow-control check box and a SOF rate box. Windowed subsystem:
  there is no console, so `--help`, `--version` and startup errors use a message
  box.

### Not yet
- The entity/environment writer (the `PdkSimLink` add-on — poses, time of day,
  weather, cloud pattern), the rotor and the lights are the open work; they live
  on the `feature/pdk-simlink` branch, not here. On `main` the sim's own
  aircraft is still positioned over FSUIPC (`src\TerrainSource.h`).
