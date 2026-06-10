# 12 - Development and Deployment

> Layer: Operations | Depends on: 02-technology-stack

## Repository Structure

Planned structure:

```text
rfid-tools/
  docs/
    requirements/
    solution-design/
    implementation/
  firmware/
    esp32/
      src/
      include/
      test/
  ios/
    RFIDTools/
      RFIDTools/
      RFIDToolsTests/
      RFIDToolsUITests/
  tools/
    protocol/
      test-vectors/
      scripts/
```

This structure is planned, not yet implemented.

## Codebase Ownership

| Path | Owns |
|---|---|
| `ios/RFIDTools/` | Native Swift/SwiftUI app, CoreBluetooth central, saved tags, scan/write/clone UI, iOS diagnostics |
| `firmware/esp32/` | ESP32 BLE peripheral, YRM100 UART driver, RFID command service, hardware pin config, serial diagnostics |
| `tools/protocol/` | Optional host-side protocol helpers, generated fixtures, captured frame samples, and test vectors |
| `docs/` | Requirements, solution design, implementation plan, QA notes, module wiring references |

The iOS and firmware codebases should not share source code directly. Their shared contract is the documented BLE protocol plus protocol test vectors.

## Test Vectors

Test vectors are small, known input/output examples used to verify protocol code.

Examples:

| Vector | Input | Expected Output |
|---|---|---|
| YRM100 checksum | `00 22 00 00` | checksum `22` |
| Single inventory frame | command type/code | bytes `BB 00 22 00 00 22 7E` |
| Inventory notice parse | raw `BB 02 22 ... 7E` frame | EPC, PC, RSSI, CRC fields |
| Write power frame | `26 dBm` | payload `0A 28`, valid `0xB6` frame |
| BLE tag event | parsed tag read | encoded `TAG_SEEN` event |

Test vectors let the firmware parser, iOS BLE decoder, and any helper tools agree on behavior without sharing code.

## iOS Development

- Use Xcode for native Swift/SwiftUI app.
- Test BLE on a real iPhone.
- Free Apple ID signing is acceptable for early development but expires and requires redeploy.
- App Store/TestFlight are deferred unless Apple Developer Program enrollment is added.

## Firmware Development

- Final ESP32 framework is deferred until board choice.
- Firmware must be flashable locally.
- USB serial logs are required for bench testing.
- Firmware should not require iPhone app for first UART/RFID validation.

## SDK Handling

- SDK remains outside repo at `../YRM100 SDK 20240525/`.
- Do not copy vendor binaries into repo.
- Do not execute vendor binaries/scripts/installers.
- Extract protocol facts into project docs and project-owned tests.

## Local Artifacts

The `.gitignore` should continue excluding:

- Xcode derived data and signing artifacts
- firmware build outputs
- logs and serial captures
- vendor SDK binaries
- local secrets/env files

## Release Strategy

Phase-one release means a local working prototype:

- documented module wiring
- firmware flashed to ESP32
- iPhone app installed locally
- read/save/write/clone workflows validated with test tags
