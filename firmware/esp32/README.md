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

The firmware framework is deferred until the board pinout and development workflow are confirmed.

The firmware must be testable over USB serial without requiring the iPhone app for initial YRM100 bring-up.
