# Agents

This document describes the software/hardware **agents** (nodes) in the CIGI
flight-simulator network and how they interact. The reference behaviour was
reverse-engineered from `Host-Master.pcapng`.

## Topology

```
                     Downstream (control / flight data)   port 8000
   ┌──────────────┐ ───────────────────────────────────────────►  ┌────────────────────┐
   │              │                                               │                    │
   │     HOST     │                                               │  IG (image gen.)   │
   │ 192.168.70.2 │                                               │  192.168.70.70     │
   │              │ ◄───────────────────────────────────────────  │                    │
   └──────────────┘   Upstream (SOF + HAT/HOT responses) port 8001└────────────────────┘
```

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

### Master IG — `192.168.70.70`
The original Image Generator (the one being replaced). In the capture it acts
as the frame-timing master: it emits a **Start-of-Frame (SOF)** at ~112 Hz and,
after a ~100-frame warm-up, appends **HAT/HOT Extended Response** messages to
its SOF stream.

### CigiStarter (this program) — the new IG PC
A drop-in replacement for the Master IG. It replicates the upstream behaviour
that the HOST requires to start streaming:

- Binds `0.0.0.0:8000` (all interfaces) and sends SOF to `192.168.70.2:8001`;
  the HOST addresses the IG as `192.168.70.70` (addresses/ports are hardcoded
  in `main.cpp`).
- Sends SOF at a configurable rate (`--fps`, default 100 Hz; the window's rate
  box retunes the running stream).
- Reports the IG mode the HOST commands. Every HOST datagram carries an IG
  Control asking for Operate, and the SOF's mode byte (bits 0-1 of byte 5)
  follows it to `0x31` instead of staying at the `--ig-mode` default. The flag
  only seeds the mode used before the first IG Control arrives.
- Answers HAT/HOT requests by default (the window's check box, or
  `--no-respond-hathot`, switches it off — which also stops the FSUIPC poller
  with it). The Master IG used to answer these from its own terrain database;
  CigiStarter has no database, so it reports the terrain under the aircraft as
  seen by the local Prepar3D, read over FSUIPC (`TerrainSource.cpp`, offset
  `0x0020`). This is what keeps the HOST's altitude loop honest — the HOST
  commands the aircraft to the aHOT it is given, so a missing/wrong HOT flies
  the aircraft into the ground.
- Parses inbound CIGI (IG Control, Entity Control, …) for the live status block
  in its operator window.
- Says nothing to the HOST until **START** is clicked. The window (`Gui.cpp`) has
  START / STOP / EXIT plus the HAT/HOT check box and the SOF rate box; the SOF
  stream starts paused. EXIT is the only way out — the title-bar X and Alt+F4
  are swallowed, so a stray click cannot take the IG off the link mid-run.

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

With HAT/HOT answering on (the default), there is one more edge to that
picture: before answering a HAT/HOT request, CigiStarter reads the terrain
height under the aircraft from the local Prepar3D over FSUIPC. The poller owning
that link is described in `TerrainSource.cpp`; it exists only while answering is
on.

## Key facts

| Property          | Value                        |
|-------------------|------------------------------|
| Protocol          | CIGI 3.3, UDP, little-endian |
| HOST address      | 192.168.70.2 (hardcoded)     |
| IG address        | 192.168.70.70 (hardcoded)    |
| Downstream port   | 8000 (HOST → IG)             |
| Upstream port     | 8001 (IG → HOST)             |
| SOF message id    | 101 (`0x65`)                 |
| IG mode           | commanded by the HOST via IG Control; the SOF echoes it back (Operate throughout the reference capture) |
| HAT/HOT resp ids  | 102 (`0x66`) / 103 (`0x67`)  |
| Terrain source    | FSUIPC `0x0020` of the local Prepar3D (ground altitude under the player aircraft) |
| Reference SOF rate| ~112 Hz (~9 ms)              |
| Operator UI       | plain Win32 window (`Gui.cpp`): START / STOP / EXIT, a HAT/HOT check box and a SOF rate box |
| Runtime settings  | SOF rate (1..100000 Hz, applied on the next frame) and HAT/HOT answering (drives the FSUIPC poller); `--fps` / `--no-respond-hathot` only seed them |
| Exit path         | EXIT button only — the title-bar X / Alt+F4 / `WM_CLOSE` are swallowed |
| Process type      | GUI subsystem: no console; `--help` and startup errors use a message box |
