# 10 - Testing Strategy

> Layer: Experience | Depends on: 05-firmware-design, 07-ios-app-design, 08-rfid-protocol-design

## Test Layers

| Layer | Tests |
|---|---|
| RFID parser | Frame parsing, checksum, command responses |
| Firmware service | Scan/write state transitions and error handling |
| BLE protocol | Command/event encoding and decoding |
| iOS app | View models, saved reads, write confirmation |
| Hardware | Wiring, power, UART response, RF inventory |
| End-to-end | iPhone -> ESP32 -> YRM100 -> tag |

## Test Vectors

Test vectors are small known examples that define expected protocol behavior. They should be stored under `tools/protocol/test-vectors/` when implementation begins.

Examples:

- raw YRM100 command bytes and expected checksum
- raw YRM100 response bytes and expected parsed fields
- malformed frames and expected parser errors
- BLE command payloads and expected decoded commands
- BLE event payloads and expected decoded app events

These vectors are the shared contract between iOS, firmware, and optional host-side tools. They avoid sharing source code between Swift and ESP32 firmware while still keeping both sides aligned.

## Firmware Unit Tests

Target:

- frame builder
- frame parser
- checksum
- command mapping
- error mapping
- write validation

## Hardware Bring-Up Tests

1. Power and ground continuity.
2. EN high or GPIO-controlled enable.
3. UART get module info.
4. Single inventory with one tag.
5. Stop multiple inventory.
6. Set/get region.
7. Set/get conservative TX power.
8. Read EPC or user memory.
9. Write to sacrificial test tag only.
10. Verify write by read-back.

## iOS Tests

- BLE state transitions.
- Connection error handling.
- Saved read create/rename/delete.
- Clone source selection.
- Write confirmation prevents accidental send.
- Diagnostics log captures command/result/error.

## Manual QA Gates

- No write workflow is accepted without explicit confirmation.
- No clone workflow is accepted without clear source and target data.
- No RF scan workflow is accepted without a stop path.
- No solution is accepted if hardware testing depends only on the iPhone app.
