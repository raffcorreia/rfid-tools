# RFIDTools iOS App

This directory will contain the native iOS application.

Ownership:

- Swift / SwiftUI app shell
- CoreBluetooth central implementation
- reader connection state
- live scan UI
- saved tag reads and labels
- write and clone workflows
- iOS diagnostics

The app communicates with the ESP32 through the documented BLE protocol. It should not depend on raw YRM100 UART frames for normal operation.
