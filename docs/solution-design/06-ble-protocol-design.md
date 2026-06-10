# 06 - BLE Protocol Design

> Layer: Core Design | Depends on: 03-architecture, 05-firmware-design

## BLE Roles

| Device | Role |
|---|---|
| iPhone | BLE central |
| ESP32 | BLE peripheral / GATT server |

## Service Shape

The ESP32 exposes one custom RFID service.

| Characteristic | Direction | Purpose |
|---|---|---|
| Command | iOS writes to ESP32 | Start/stop scan, read, write, config |
| Events | ESP32 notifies iOS | Tag seen, operation result, errors |
| Status | iOS reads or subscribes | Firmware version, state, reader status |

Exact UUIDs are deferred until implementation, but they should be stable and documented.

## Message Model

BLE messages should be high-level and versioned. The iOS app should not send raw YRM100 frames during normal operation.

Recommended logical message groups:

| Group | Commands / Events |
|---|---|
| Connection | hello, status, firmware info |
| Inventory | start scan, stop scan, tag seen |
| Memory | read memory, read result |
| Write | write memory, write result, verification result |
| Config | get/set region, get/set power |
| Diagnostics | log event, raw frame summary, error |

## Command Examples

Conceptual commands:

```text
START_INVENTORY { mode, count, dedupe }
STOP_INVENTORY
READ_MEMORY { targetEpc, bank, wordOffset, wordCount, accessPassword }
WRITE_MEMORY { targetEpc, bank, wordOffset, dataHex, accessPassword, verify }
SET_REGION { region: "US" }
SET_POWER { dbm: 18 }
```

The final binary or JSON-like encoding is deferred. For early bring-up, a simple framed text or compact binary protocol is acceptable if it is documented and testable.

## Event Examples

```text
TAG_SEEN { epc, pc, rssiDbm, crc, seenCount }
SCAN_STARTED
SCAN_STOPPED { reason }
READ_RESULT { epc, bank, wordOffset, dataHex }
WRITE_RESULT { epc, bank, wordOffset, status }
ERROR { code, message, source }
```

## Transport Rules

- Every command gets either a result event or an error event.
- Long-running scan emits many `TAG_SEEN` events.
- Events may be batched if BLE notification throughput is a problem.
- Commands that can modify tags require explicit app-side confirmation before being sent.
- BLE disconnect cancels active app workflow.

## Versioning

The service should expose:

- protocol version
- firmware version
- supported capability flags

Capability flags allow the app to hide unsupported operations such as write user memory or set region.
