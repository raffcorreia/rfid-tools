# RFID BLE Service Sketch

Arduino-compatible ESP32 sketch for `PHASE-003 - ESP32 BLE RFID Service`.

This sketch is separate from `yrm100_bringup/` so the proven UART bench tool remains
available while BLE service work progresses.

## Current Scope

- Advertises as `RFID Tools ESP32`.
- Exposes the documented RFID BLE service UUID.
- Provides command, event notification, and status characteristics.
- Handles `hello` and `status` commands.
- Returns explicit `not_implemented` errors for inventory/config commands until the
  YRM100 driver is wired into this firmware entry point.

## BLE UUIDs

| Item | UUID |
|---|---|
| Service | `63802432-69FC-406D-A538-FE33CEF32AEF` |
| Command | `DE0D7201-CC2B-46D9-8F92-564A209C37EF` |
| Events | `456F5CDA-632A-4541-A2A5-6FAEC234075E` |
| Status | `0AF05FBF-ADAA-4D94-8DCF-44A62F82332B` |

## Test Commands

Write UTF-8 JSON to the command characteristic:

```json
{"v":1,"id":1,"cmd":"hello"}
{"v":1,"id":2,"cmd":"status"}
{"v":1,"id":3,"cmd":"startInventory"}
{"v":1,"id":4,"cmd":"stopInventory"}
```

Subscribe to the events characteristic to receive responses.
