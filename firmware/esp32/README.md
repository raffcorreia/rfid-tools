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
