# 06 - Business Rules

## General Principles

### Confirmed Behavior Over Assumptions

Reader behavior must be documented as confirmed only after it is verified from trusted documentation, reviewed SDK source, or controlled hardware testing.

### Vendor SDK Is Reference Only

The YRM100 SDK may be used to understand the protocol, but it must not be executed directly. Required behavior must be reimplemented in project-owned code.

### High-Level BLE API

The iPhone app should send high-level commands to the ESP32, such as start inventory or set power. It should not need to understand raw YRM100 UART frames for normal operation.

## RFID Inventory

- A scan session starts only after explicit user action.
- A scan session ends after explicit stop, timeout, disconnect, or firmware error.
- Duplicate reads of the same EPC should update the existing tag entry rather than creating confusing repeated rows.
- RSSI should be attached to a tag read only when the reader response supports it.
- Stale buffered reader data should not be presented as a fresh scan result.

## Saved Tag Reads

- A read can be saved only when it contains at least one useful tag identifier or memory value.
- A saved read must have a user-visible label.
- Labels are user-controlled and may be descriptive, such as `PLA Blue TAG`.
- Saved reads should preserve the original captured values and timestamp.
- Editing a label must not change the captured tag data.
- Saved reads are local to the iPhone in the first phase.

## Tag Writing

- Tag writing is considered higher risk than reading.
- Write support remains conditional until the YRM100 write protocol and H9 tag memory layout are confirmed.
- The app must require explicit confirmation before writing a tag.
- The app must clearly show whether the write source is a saved read or manually entered data.
- The system should verify a write by reading back the target memory when possible.
- Lock, kill, password, and reserved-memory operations are out of scope until explicitly designed.

## Tag Cloning

- Cloning means copying supported readable/writable tag data from a saved read onto another compatible writable tag.
- Cloning must not imply that every physical tag can be duplicated exactly.
- Clone workflows must identify which memory bank or value will be written before confirmation.
- Clone workflows must verify the target tag after writing when possible.
- Cloning protected, locked, passworded, killed, or access-controlled tag data is out of scope.
- The app must not present cloning as successful unless the write is acknowledged and, where possible, verified by a read-back.

## Reader Configuration

- The target operating region is US 902-928 MHz.
- The app or firmware must not silently switch to a different region.
- RF power should default to a conservative value during bring-up.
- Increasing RF power should be explicit and visible to the user in development builds.
- Region and power commands must be confirmed from protocol documentation before use.

## Connection Lifecycle

- The app must treat BLE disconnect as a stop condition for active scanning.
- Firmware must stop or safely settle reader activity if the app disconnects during an inventory.
- The ESP32 should expose a recognizable BLE name or service UUID.
- Reconnection should not assume the previous scan session is still valid.

## Hardware Handling

- The YRM100 must be powered from a supply capable of supporting peak current.
- ESP32 GPIO pins must not be used as the reader power source.
- TX/RX direction must be confirmed before wiring.
- EN pin behavior must be documented before relying on sleep or wake workflows.
- Common ground between ESP32 and YRM100 is required.
- Power wiring must be checked before enabling RF inventory.
- The final wiring reference must record logical signal names, connector orientation, and physical pin references. Wire colors are not authoritative.

## Documentation Rules

- Hardware facts should cite their source where possible.
- Assumptions must be labeled as assumptions.
- Open questions should remain in `07-open-questions.md` until resolved.
- Architecture may change if the YRM100 supports direct iPhone-compatible Bluetooth.
