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
- Sends SOF at a configurable rate (`--fps`, default 100 Hz).
- Optionally answers HAT/HOT requests (`--respond-hathot`).
- Parses inbound CIGI (IG Control, Entity Control, …) for status reporting.
- Controlled from the console (case-insensitive): `S` start, `X` stop, `Q` quit
  — the SOF stream starts paused.

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

## Key facts

| Property          | Value                        |
|-------------------|------------------------------|
| Protocol          | CIGI 3.3, UDP, little-endian |
| HOST address      | 192.168.70.2 (hardcoded)     |
| IG address        | 192.168.70.70 (hardcoded)    |
| Downstream port   | 8000 (HOST → IG)             |
| Upstream port     | 8001 (IG → HOST)             |
| SOF message id    | 101 (`0x65`)                 |
| HAT/HOT resp id   | 103 (`0x67`)                 |
| Reference SOF rate| ~112 Hz (~9 ms)              |
