# ESP32 Firmware

This directory will contain the ESP32 firmware.

Ownership:

- BLE peripheral / GATT server
- high-level RFID command handling
- YRM100 UART protocol driver
- reader configuration
- module pin configuration
- USB serial diagnostics

Selected board: ESP32-S3 SuperMini development board.

The firmware framework is deferred until the development workflow is confirmed.

The firmware must be testable over USB serial without requiring the iPhone app for initial YRM100 bring-up.

## Bring-Up Tools

| Tool | Purpose |
|---|---|
| `yrm100_bringup/` | Arduino-compatible sketch for PHASE-001 UART wiring validation |
| `rfid_ble_service/` | Arduino-compatible sketch for PHASE-003 BLE service bring-up |
| `yrm100_driver/` | Project-owned YRM100 frame builder, parser, command builders, and decoders |

## First Bring-Up Wiring

Use the board-labeled external pins from the selected ESP32-S3 SuperMini:

| YRM100 Pin | YRM100 Signal | ESP32-S3 SuperMini Connection | Notes |
|---|---|---|---|
| 1 | `GND` | `GND` | Common ground |
| 2 | `EN` | `3V3` | Tie high for first bring-up |
| 3 | `RXD` | `TX` | ESP32 transmits to reader |
| 4 | `TXD` | `RX` | ESP32 receives from reader |
| 5 | `VCC` | `5V` | USB-powered rail first; external regulated 5V fallback if unstable |

Do not power the YRM100 from a GPIO. If there is no UART response, verify TX/RX direction before changing firmware assumptions.

The YRM100 board also has an unpopulated `J2` footprint. Continuity testing currently confirms only the top square pad as `VCC/5V` and the bottom pad as `GND`. Do not use the three middle `J2` pads for UART or `EN` until they are identified.
