# 12 - Development and Deployment

> Layer: Operations | Depends on: 02-technology-stack

## Repository Structure

Planned structure:

```text
rfid-tools/
  docs/
    requirements/
    solution-design/
  firmware/
    esp32/
  ios/
    RFIDTools/
  tools/
    protocol/
```

This structure is planned, not yet implemented.

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
