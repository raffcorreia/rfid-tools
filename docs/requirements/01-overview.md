# 01 - Overview

## Purpose

RFID Tools is a prototype and development platform for reading, saving, writing, cloning, and managing UHF RFID tags from an iPhone.

The project is built around a YRM100 integrated UHF RFID reader module, an ESP32 controller, and a native iOS application. The first goal is to make the hardware reliable and understandable. The second goal is to build a clean mobile workflow for scanning tags, saving reads with human labels, and writing tag data when the tag and reader support it.

## System Goals

- Read nearby UHF RFID tags from an iPhone-controlled workflow.
- Save scanned tag data with a user-provided label, such as `PLA Blue TAG`.
- Write user-provided data to compatible tags.
- Support a controlled tag cloning workflow where data read from one tag can be written to another compatible tag.
- Use an ESP32 as the first planned bridge between the iPhone and the YRM100 module.
- Communicate between iPhone and ESP32 using Bluetooth Low Energy.
- Communicate between ESP32 and YRM100 using TTL UART.
- Support US UHF RFID operation in the 902-928 MHz band.
- Keep the RFID module protocol isolated behind a clean firmware API.
- Avoid executing untrusted vendor SDK code.
- Produce documentation that captures hardware assumptions, open questions, and future design decisions.

## Scope

### In Scope

- Native iOS application for iPhone.
- BLE connection from iPhone to ESP32.
- ESP32 firmware that exposes RFID actions over BLE.
- UART driver for the YRM100 RFID module.
- Tag inventory/scanning workflows.
- Displaying EPC values, RSSI when available, and scan status.
- Saving scanned tags with names/labels for later reuse.
- Writing arbitrary supported tag memory values from the app.
- Cloning supported tag data from a saved read onto another compatible writable tag.
- Basic reader configuration such as power and region, if supported by the YRM100 protocol.
- Tag write/programming workflows, if supported safely by the module and tags.
- Documentation for hardware, requirements, open questions, and later design decisions.

### Out of Scope

- App Store distribution during the early prototype phase.
- TestFlight distribution unless Apple Developer Program enrollment is added later.
- Running vendor SDK binaries, installers, demos, or scripts directly.
- Treating the Windows demo application as part of the product.
- Production asset-management backend or cloud sync in the first phase.
- Multi-user account management in the first phase.
- Regulatory certification work beyond documenting frequency and power assumptions.
- Circumventing tag security features such as passwords, locks, kill commands, or access controls.

## Users

| User | Description |
|---|---|
| Builder / Developer | Builds firmware, tests hardware, inspects logs, and validates protocol behavior. |
| Operator | Uses the iPhone app to scan tags and view tag data. |
| Asset Manager | Future user who may associate scanned tags with real-world assets. |

## Constraints

- iOS BLE behavior must comply with Apple CoreBluetooth limitations.
- iOS testing without a paid Apple Developer Program account has signing expiration friction.
- The YRM100 SDK directory is untrusted and must not be executed directly.
- The ESP32 board model is not selected yet.
- The YRM100 UART command protocol still needs to be extracted from documentation or reviewed source.
- The target operating region is the United States, so RF behavior must target 902-928 MHz.

## High-Level Flow

```text
1. User opens the iPhone app.
2. App scans for the ESP32 BLE peripheral.
3. App connects to the ESP32.
4. User starts an RFID inventory.
5. ESP32 sends UART commands to the YRM100.
6. YRM100 reads UHF tags.
7. ESP32 parses reader responses.
8. ESP32 notifies the iPhone app over BLE.
9. App displays tag EPC, RSSI/status, and scan history.
10. User may save a read with a label for later reference.
11. User may use a saved read as the source for a write/clone operation, if supported.
```

## Possible Future Extensions

- Direct iPhone-to-YRM100 Bluetooth connection if the module supports it.
- Local asset database on the iPhone.
- Export to CSV or JSON.
- Cloud sync or backend API.
- Tag commissioning workflow for writing EPCs.
- Android companion app.
- ESP32 web-based debug console.
