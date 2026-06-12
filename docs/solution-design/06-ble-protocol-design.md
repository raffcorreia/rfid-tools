# 06 - BLE Protocol Design

> Layer: Core Design | Depends on: 03-architecture, 05-firmware-design

## BLE Roles

| Device | Role |
|---|---|
| iPhone | BLE central |
| ESP32 | BLE peripheral / GATT server |

## Service Shape

The ESP32 exposes one custom RFID service.

| Item | UUID | Direction | Purpose |
|---|---|---|---|
| RFID service | `63802432-69FC-406D-A538-FE33CEF32AEF` | GATT service | Custom app-facing RFID service |
| Command characteristic | `DE0D7201-CC2B-46D9-8F92-564A209C37EF` | iOS writes to ESP32 | Start/stop scan, read, write, config |
| Events characteristic | `456F5CDA-632A-4541-A2A5-6FAEC234075E` | ESP32 notifies iOS | Tag seen, operation result, errors |
| Status characteristic | `0AF05FBF-ADAA-4D94-8DCF-44A62F82332B` | iOS reads or subscribes | Firmware version, state, reader status |

These UUIDs are stable for PHASE-003 and should not change without a protocol-version bump.

## Message Model

BLE messages should be high-level and versioned. The iOS app should not send raw YRM100 frames during normal operation.

Recommended logical message groups:

| Group | Commands / Events |
|---|---|
| Connection | getInfo, status, firmware info |
| Inventory | start scan, stop scan, tag seen |
| Memory | read memory, read result |
| Write | write memory, write result, verification result |
| Config | get/set region, get/set power |
| Diagnostics | log event, raw frame summary, error |

## Encoding

PHASE-003 uses UTF-8 JSON objects as complete BLE characteristic values. This keeps early
firmware and iOS debugging simple with generic BLE tools. Each message is one JSON object;
newline delimiters are allowed in serial diagnostics but are not required over BLE.

All command messages include:

| Field | Type | Meaning |
|---|---|---|
| `v` | integer | BLE protocol version, initially `1` |
| `id` | integer | App-assigned command sequence number |
| `cmd` | string | Command name |

All event messages include:

| Field | Type | Meaning |
|---|---|---|
| `v` | integer | BLE protocol version, initially `1` |
| `id` | integer or null | Matching command id when the event completes a command |
| `evt` | string | Event name |

Early messages should stay below 180 bytes so they fit comfortably within common negotiated
BLE MTU limits. If larger read/write payloads are needed later, add chunking under a protocol
version bump instead of silently truncating.

## Command Examples

Conceptual commands:

```json
{"v":1,"id":1,"cmd":"getInfo"}
{"v":1,"id":2,"cmd":"status"}
{"v":1,"id":3,"cmd":"startInventory","mode":"continuous"}
{"v":1,"id":4,"cmd":"stopInventory"}
{"v":1,"id":5,"cmd":"getPower"}
{"v":1,"id":6,"cmd":"setPower","dbm":18}
{"v":1,"id":7,"cmd":"getRegion"}
{"v":1,"id":8,"cmd":"setRegion","region":"US"}
```

PHASE-003 does not expose raw YRM100 frames through the normal command characteristic.
Read/write memory commands are reserved for later phases once the app workflow and safety
confirmation screens exist.

## Event Examples

```json
{"v":1,"id":1,"evt":"info","name":"RFID Tools ESP32","fw":"0.1.0","caps":["inventory","power","region"]}
{"v":1,"id":2,"evt":"status","state":"idle","reader":"ready","region":"US","powerDbm":18}
{"v":1,"id":3,"evt":"scanStarted"}
{"v":1,"id":null,"evt":"tagSeen","epc":"E2000017221101441890ABCD","pc":"3000","rssi":-42,"crc":"1234","seen":1}
{"v":1,"id":4,"evt":"scanStopped","reason":"requested"}
{"v":1,"id":6,"evt":"result","ok":true}
{"v":1,"id":null,"evt":"error","code":"reader_timeout","source":"yrm100","message":"Reader did not respond"}
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
