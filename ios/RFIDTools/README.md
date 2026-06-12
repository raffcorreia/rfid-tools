# RFIDTools iOS App

This directory contains the native iOS application.

Ownership:

- Swift / SwiftUI app shell
- CoreBluetooth central implementation
- reader connection state
- live scan UI
- saved tag reads and labels
- write and clone workflows
- iOS diagnostics

The app communicates with the ESP32 through the documented BLE protocol. It should not depend on raw YRM100 UART frames for normal operation.

## Current Phase Scope

`PHASE-004` provides the SwiftUI/CoreBluetooth workflow app:

- auto-connect to the documented RFID BLE service
- reconnect after connection loss
- discover command, events, and status characteristics
- subscribe to event/status updates
- read tags and display EPC as text or hex
- save named tags locally
- apply a saved tag EPC to the current target tag
- write typed text through the same EPC write command
- set reader power with a slider

Build from this directory with Xcode, or from the repository root with:

```sh
DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer xcodebuild \
  -project ios/RFIDTools/RFIDTools.xcodeproj \
  -scheme RFIDTools \
  -sdk iphonesimulator \
  CODE_SIGNING_ALLOWED=NO \
  build
```
