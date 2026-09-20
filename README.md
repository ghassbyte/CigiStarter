# CigiStarter

A minimal, dependency-free **CIGI 3.3 Image Generator (IG)** for a new IG PC.

The HOST in a CIGI flight-simulator link stays silent until the IG transmits
**Start-of-Frame (SOF)** messages. `CigiStarter` plays that IG role: it sends
SOF at a configurable rate to wake up the HOST and begin the flight-data
stream, and (optionally) answers HAT/HOT requests.

## What it does

1. Binds a UDP socket on the downstream port (`8000`) where the HOST sends
   control/flight data.
2. Sends **SOF** (24-byte Start-of-Frame) datagrams to the HOST upstream port
   (`8001`) at the configured frame rate — this is the "request" that starts
   the data flow.
3. Parses inbound CIGI messages (framed by `[id][size]`) and prints a one-line
   heartbeat showing datagrams/s, total datagrams, entity updates and the
   HOST frame counter.
4. Optionally (`--respond-hathot`) appends HAT/HOT Extended Responses to the
   SOF stream, mirroring the reference capture.

## Requirements

- Windows 10/11, x64.
- **No third-party libraries** — only Windows sockets (`Ws2_32.lib`).
- Built with the static CRT (`/MT`, `/MTd`), so the executable runs on a fresh
  IG PC **without installing the Visual C++ redistributable**.
- The embedded manifest requests **administrator** privileges (required to
  bind low ports and traverse the firewall).

## Building

Using MSBuild from a Visual Studio / Build Tools developer environment:

```
msbuild CigiStarter.sln /p:Configuration=Release /p:Platform=x64
```

Or open `CigiStarter.sln` in Visual Studio and build **Release | x64**.
The output is `x64\Release\CigiStarter.exe`.

## Usage

```
CigiStarter.exe [options]

  --host-ip <ip>      HOST address to send SOF to       (default 192.168.70.2)
  --host-port <port>  HOST upstream port (IG -> HOST)   (default 8001)
  --bind-ip <ip>      local address to bind for RX      (default 0.0.0.0)
  --bind-port <port>  local port to bind for RX         (default 8000)
  --fps <rate>        SOF transmit rate in Hz           (default 100)
  --ig-mode <0|1>     SOF IG mode 0=Standby 1=Operate   (default 0)
  --respond-hathot    answer HAT/HOT requests           (default off)
  --help              show help
```

Example:

```
CigiStarter.exe --host-ip 192.168.70.2 --fps 100 --respond-hathot
```

Press `Ctrl+C` to stop cleanly.

## Network layout

| Direction   | From                  | To                    | Port | Payload                         |
|-------------|-----------------------|-----------------------|------|---------------------------------|
| Downstream  | HOST `192.168.70.2`   | IG (`CigiStarter`)    | 8000 | IGCtrl, EntityCtrl, HAT/HOT req |
| Upstream    | IG (`CigiStarter`)    | HOST `192.168.70.2`   | 8001 | SOF (+ HAT/HOT responses)       |

## Protocol notes

- **CIGI version 3.3**, little-endian on the wire.
- A message is framed as `[message-id(1)][message-size(1)][payload...]` where
  `size` is the total byte length including the two header bytes.
- The SOF sent by this program matches the reference capture:
  `65 18 03 00 00 30 00 80` + frame number + timestamp + last-host-frame + pad.
  Byte `0x30` is IG mode 0 (Standby); pass `--ig-mode 1` for `0x31` (Operate).

## Project layout

| File               | Purpose                                        |
|--------------------|------------------------------------------------|
| `main.cpp`         | entry point, sockets, SOF + receive threads    |
| `cigi.h`           | CIGI 3.3 message IDs, sizes, pack/parse API    |
| `cigi.cpp`         | explicit little-endian message packing         |
| `app.manifest`     | UAC `requireAdministrator` manifest            |
| `agents.md`        | description of the HOST / IG / CigiStarter agents |
| `CigiStarter.sln`  | Visual Studio solution (x64 only)              |

See `agents.md` for a description of each agent and the message flow.
