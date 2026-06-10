# 02 - Technology Stack

> Layer: Foundation | Depends on: 01-solution-overview

## iOS App

| Area | Choice | Rationale |
|---|---|---|
| Language | Swift | Native iOS language with first-class CoreBluetooth support. |
| UI | SwiftUI | Fast iteration and modern iOS UI patterns. |
| BLE | CoreBluetooth | Direct control over scanning, connection, characteristics, writes, and notifications. |
| Local storage | SwiftData or SQLite-backed persistence | Suitable for saved reads, labels, timestamps, and clone sources. Final choice deferred. |
| Logging | OSLog plus in-app debug log | Useful for BLE diagnostics during hardware bring-up. |

## ESP32 Firmware

| Area | Choice | Rationale |
|---|---|---|
| Platform | ESP32 | Available hardware, BLE support, UART support. |
| BLE role | Peripheral / GATT server | iPhone acts as BLE central. |
| RFID interface | Hardware UART | YRM100 exposes 3.3V TTL UART. |
| Protocol implementation | Project-owned C/C++ firmware module | Avoids vendor runtime dependency. |
| Debug interface | USB serial logs | Required for bench testing without iPhone. |

The exact ESP32 framework is deferred until board selection. Candidate frameworks are Arduino-ESP32 for fast bring-up or ESP-IDF for stronger long-term control.

## RFID Reader

| Area | Choice |
|---|---|
| Module | YRM100 integrated UHF RFID reader |
| Protocol | EPCglobal UHF Class 1 Gen 2 / ISO 18000-6C |
| Host interface | 3.3V TTL UART |
| Default baud | 115200 bps |
| Region | USA, command code `0x02` |
| Power range | 15-26 dBm, encoded as dBm x 100 |

## Development Tools

| Area | Tooling |
|---|---|
| iOS IDE | Xcode |
| Firmware IDE | PlatformIO, Arduino IDE, or ESP-IDF tooling after framework choice |
| Documentation | Markdown in `docs/` |
| Version control | Git |
| Vendor SDK | Read-only reference material |

## Deferred Choices

- Exact ESP32 board.
- ESP32 firmware framework.
- iOS local persistence technology.
- BLE message encoding details.
- Final module wiring.
