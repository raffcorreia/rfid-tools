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

`PHASE-004` starts with a SwiftUI/CoreBluetooth app shell:

- scan for the documented RFID BLE service
- connect and disconnect from the ESP32 peripheral
- discover command, events, and status characteristics
- subscribe to event/status updates
- send development commands such as `getInfo`, `status`, `startInventory`, and `stopInventory`
- display live `tagSeen` events and a diagnostics log

Build from this directory with Xcode, or from the repository root with:

```sh
DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer xcodebuild \
  -project ios/RFIDTools/RFIDTools.xcodeproj \
  -scheme RFIDTools \
  -sdk iphonesimulator \
  CODE_SIGNING_ALLOWED=NO \
  build
```
