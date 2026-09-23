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
HOST sent.

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
as the frame-timing master: it emits a **Start-of-Frame (SOF)** at ~100 Hz (an
8/12 ms alternation) and, after a ~100-frame warm-up, appends **HAT/HOT Extended
Response** messages to its SOF stream. The SOF's frame-number field advances only
~53 times a second, not ~100 — it carries the HOST's own counter rather than a
count of SOFs (see CigiStarter below and the protocol notes in `README.md`).

### CigiStarter (this program) — the new IG PC
A drop-in replacement for the Master IG. It replicates the upstream behaviour
that the HOST requires to start streaming:

- Binds `0.0.0.0:8000` (all interfaces) and sends SOF to `192.168.70.2:8001`;
  the HOST addresses the IG as `192.168.70.70` (addresses/ports are hardcoded
  in `main.cpp`).
- Sends SOF at a configurable rate (`--fps`, default 60 Hz; the window's rate
  box retunes the running stream), and **flow control is on by default**: the
  transmit loop holds a SOF back while the HOST has not answered the previous
  one, so the stream fits whatever frame rate the HOST can actually produce
  instead of piling requests up on it. `--no-flow-control` transmits on the rate
  alone, which is what the original Master IG did.
- The SOF's only live field is the **HOST's frame counter**, echoed out of the
  IG Control it arrived in: that is what the reference Master put there, and it
  advances with the HOST's frame rate (~53 Hz in the reference capture) rather
  than with the SOF rate (~100 Hz). An IG-side count of transmitted SOFs in that
  slot would drift away from the HOST by (rate − 53) frames every second, which
  is what used to break the HOST above ~60 Hz. The unit tests in
  `SofPacer.h`'s harness pin the outgoing SOF to the captured bytes.
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
- **Fans the downstream stream out** to the extra IG PCs listed in `client.txt`
  (`Relay.cpp`): every datagram the HOST sends is copied, byte for byte, into one
  injected Ethernet frame per client, labelled with the HOST's address and port
  so that those clients need no reconfiguration. They are passive — they never
  send anything back to CigiStarter, and nothing is sent up the link on their
  behalf. The copy needs Npcap on the IG PC; it is loaded at run time, so the
  program still runs as a plain IG without it. No clients listed (or no
  `client.txt`) simply means the fan-out is off.
- Says nothing to the HOST until **START** is clicked. The window (`Gui.cpp`) has
  START / STOP / EXIT plus the HAT/HOT check box, the flow-control check box and
  the SOF rate box; the SOF stream starts paused. EXIT is the only way out — the
  title-bar X and Alt+F4 are swallowed, so a stray click cannot take the IG off
  the link mid-run.

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

[Flow control](#sof-rate-and-flow-control) — on by default — is what keeps that
loop self-limiting. When the HOST cannot produce the next frame as fast as the
rate box asks for, the IG holds the next SOF back instead of sending it, so the
loop settles at the HOST's pace rather than asking the HOST for frames it has not
made. `SofPacer.h` owns that rule; switched off, the IG transmits on the schedule
alone, as the original Master did.

With the fan-out on, every downstream datagram in that picture also leaves the
IG again — unaltered — towards each client in `client.txt`. From the clients'
point of view that edge looks exactly like the HOST's own, which is the point.

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
| Extra IG PCs      | listed in `client.txt` next to the executable (`address[:port]`, `#` comments, optional, read once at startup) |
| Fan-out           | one copy of each downstream datagram per listed client, injected with Npcap, source address/port spoofed as the HOST's |
| Fan-out adapter   | found automatically: the adapter owning this IG's address, else the one the route to the HOST leaves by |
| Reference SOF rate | ~100 Hz (~10 ms, an 8/12 ms alternation) |
| HOST frame rate   | ~53 Hz — its IG Control counter advances 0.54 per datagram while a datagram arrives every ~10.2 ms, so the HOST is asked for about twice the frames it has |
| SOF live field    | the HOST's frame counter (out of IG Control), echoed at offset 8; offsets 12/16 stay 0, as in the capture |
| SOF rate here     | 60 Hz default (1..100000 Hz from the window), paced to the HOST's replies unless flow control is off |
| Operator UI       | plain Win32 window (`Gui.cpp`): START / STOP / EXIT, a HAT/HOT check box, a flow-control check box and a SOF rate box |
| Runtime settings  | SOF rate, HAT/HOT answering (drives the FSUIPC poller) and flow control, all applied on the next frame; `--fps` / `--respond-hathot` / `--flow-control` only seed them |
| Exit path         | EXIT button only — the title-bar X / Alt+F4 / `WM_CLOSE` are swallowed |
| Process type      | GUI subsystem: no console; `--help` and startup errors use a message box |
