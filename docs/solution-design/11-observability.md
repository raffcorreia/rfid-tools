# 11 - Observability

> Layer: Operations | Depends on: 05-firmware-design, 06-ble-protocol-design

## Firmware Observability

USB serial logs should include:

- boot version
- selected ESP32 pins
- BLE connection state
- YRM100 UART TX command summaries
- YRM100 parsed response summaries
- tag seen events
- scan start/stop reason
- write result and verification result
- error code and source

## iOS Observability

The iOS app should include a diagnostics screen with:

- BLE scan/connect/disconnect events
- command writes
- event notifications
- reader errors
- firmware version and capabilities
- recent tag event summaries

## Counters

Firmware should maintain lightweight counters:

| Counter | Purpose |
|---|---|
| BLE connects/disconnects | BLE stability |
| UART frames received | Reader communication |
| UART checksum failures | Parser/power/noise diagnosis |
| tags seen | Scan behavior |
| write attempts | Write safety audit |
| write successes/failures | Clone reliability |

## Debug Modes

| Mode | Description |
|---|---|
| Normal | User-facing events only |
| Verbose | Raw frame summaries and timing |
| Bench | Extra UART and hardware state diagnostics |

Raw frame logging should be available for development but not required in normal app workflows.
