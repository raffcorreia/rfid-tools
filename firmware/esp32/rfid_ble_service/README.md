# RFID BLE Service Sketch

Arduino-compatible ESP32 sketch for `PHASE-003 - ESP32 BLE RFID Service`.

This sketch is separate from `yrm100_bringup/` so the proven UART bench tool remains
available while BLE service work progresses.

## Current Scope

- Advertises as `RFID Tools ESP32`.
- Exposes the documented RFID BLE service UUID.
- Provides command, event notification, and status characteristics.
- Handles `hello` and `status` commands.
- Starts and stops YRM100 continuous inventory.
- Emits `tagSeen` events from YRM100 inventory notices.
- Gets/sets TX power.
- Gets/sets region.
- Sends a stop-inventory frame when BLE disconnects during an active scan.

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
{"v":1,"id":5,"cmd":"getPower"}
{"v":1,"id":6,"cmd":"setPower","dbm":15}
{"v":1,"id":7,"cmd":"getRegion"}
{"v":1,"id":8,"cmd":"setRegion","region":"US"}
```

Subscribe to the events characteristic to receive responses.

## Arduino Build Note

`Yrm100DriverLink.cpp` includes the shared `../yrm100_driver/Yrm100Driver.cpp` implementation
so this sketch uses the same project-owned driver code as the host tests.
