# 05 - Non-Functional Requirements

## Safety

- The system must not execute unreviewed vendor SDK code.
- The system must avoid accidental tag writes.
- Write operations must require explicit user action.
- RF inventory must be stoppable from the app and firmware.
- Region and power settings must be handled conservatively.
- Module wiring must document the power source and common ground before reader operation.
- The YRM100 must not be powered from an ESP32 GPIO.

## Reliability

- BLE disconnects must be detected and shown to the user.
- Firmware must recover cleanly from reader timeouts.
- Firmware must avoid getting stuck in continuous inventory without an escape path.
- App state must remain coherent when the reader disconnects, resets, or stops responding.

## Performance

- The app should show tag reads with low enough latency to feel live during manual scanning.
- The firmware should handle the vendor-claimed inventory rate of more than 50 tags per second, at least by buffering, throttling, or summarizing events.
- BLE notifications should be batched or encoded efficiently enough to avoid overloading the iOS connection.
- Duplicate tag handling should keep the UI responsive during repeated reads.

## Compatibility

- The first mobile target is iPhone.
- The iOS app should use native Swift and CoreBluetooth.
- The ESP32 firmware should avoid assumptions tied to one specific ESP32 development board until the board is selected.
- The RFID layer should target EPCglobal UHF Class 1 Gen 2 / ISO 18000-6C.
- The RF region target is US 902-928 MHz unless changed explicitly.

## Maintainability

- Vendor protocol handling should be isolated in a firmware module.
- BLE application commands should be higher-level than raw YRM100 UART frames.
- Documentation should distinguish confirmed facts from assumptions.
- Open questions should remain visible until answered.
- Hardware pin choices and power assumptions should be documented before implementation is considered stable.

## Security and Privacy

- BLE access should not allow arbitrary unauthenticated control in a future production version.
- Early prototypes may use a simple BLE service, but the security model must be revisited before real asset use.
- The app should not collect unnecessary personal data.
- Asset/tag exports, if added later, should avoid leaking sensitive inventory data unintentionally.

## Usability

- The app should make the current reader state obvious.
- Scanning and stopping scans should be one-action operations.
- Errors should be specific enough to guide bench debugging.
- The tag list should remain readable during repeated inventory updates.

## Testability

- BLE protocol encoding and decoding should be testable independently of the phone and ESP32.
- YRM100 UART frame parsing should be testable with captured or documented sample frames.
- Hardware tests should be possible without requiring the iPhone app as the only control surface.
- iOS BLE behavior should be validated on a real iPhone, not only in simulator.
- Wiring validation should be possible in stages: power, EN, UART response, then RF inventory.
