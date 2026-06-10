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

## Tag Writing

- Tag writing is considered higher risk than reading.
- Write support remains conditional until the YRM100 write protocol and H9 tag memory layout are confirmed.
- The app must require explicit confirmation before writing a tag.
- The system should verify a write by reading back the target memory when possible.
- Lock, kill, password, and reserved-memory operations are out of scope until explicitly designed.

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

## Documentation Rules

- Hardware facts should cite their source where possible.
- Assumptions must be labeled as assumptions.
- Open questions should remain in `07-open-questions.md` until resolved.
- Architecture may change if the YRM100 supports direct iPhone-compatible Bluetooth.
