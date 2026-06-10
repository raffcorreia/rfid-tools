# 05 - Firmware Design

> Layer: Core Design | Depends on: 03-architecture, 04-hardware-design

## Responsibilities

The ESP32 firmware owns:

- BLE advertising and connection lifecycle.
- High-level command handling from iPhone.
- YRM100 UART frame building and parsing.
- Reader command sequencing.
- RF safety defaults.
- Error mapping from YRM100 to app events.
- USB serial diagnostics.

## Firmware Modules

| Module | Responsibility |
|---|---|
| `BleTransport` | GATT service, command writes, event notifications |
| `AppCommandHandler` | Validates high-level commands and routes work |
| `Yrm100Driver` | Builds/parses YRM100 frames and tracks pending commands |
| `RfidService` | Implements scan/read/write/clone sequencing |
| `ReaderConfig` | Region, power, channel, FHSS settings |
| `Diagnostics` | Serial logs, counters, debug snapshots |
| `HardwarePins` | UART pins, EN pin, board-specific definitions |

## State Model

| State | Meaning |
|---|---|
| `BOOTING` | Firmware starting and hardware not ready |
| `IDLE` | Ready, no active reader operation |
| `SCANNING` | Inventory active |
| `READING` | Read memory operation active |
| `WRITING` | Write operation active |
| `ERROR` | Recoverable error requiring reset/stop/clear |

Only one active YRM100 operation should run at a time.

## UART Driver

The YRM100 driver parses byte streams into frames:

```text
BB type command len_msb len_lsb payload... checksum 7E
```

Validation:

- Find header `0xBB`.
- Read 2-byte payload length.
- Require expected frame length.
- Require end byte `0x7E`.
- Validate checksum as low byte sum from Type through Parameter.
- Emit parsed response, notice, or error.

## Reader Operations

| Operation | YRM100 Commands |
|---|---|
| Single inventory | `0x22` |
| Start scan | `0x27` multiple inventory |
| Stop scan | `0x28` |
| Read memory | optional select `0x0C`/`0x12`, then `0x39` |
| Write memory | optional select `0x0C`/`0x12`, then `0x49`, then optional read-back |
| Set region | `0x07` |
| Set power | `0xB6` |

## Error Strategy

- Preserve raw YRM100 command and error code in debug logs.
- Send normalized error events to iOS.
- Treat malformed frames as firmware diagnostics, not user-level tag failures.
- Stop active scan on BLE disconnect.
- Do not allow lock, kill, or protected-tag commands in first implementation.

## Debug Surface

Firmware serial logs should support:

- boot status
- selected pins
- BLE state
- UART TX/RX frame summaries
- parsed tag events
- reader errors
- power/region settings
