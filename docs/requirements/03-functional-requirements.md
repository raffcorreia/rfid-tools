# 03 - Functional Requirements

## iOS Application

| ID | Requirement | Description |
|---|---|---|
| RF01 | BLE device scan | The app shall scan for compatible ESP32 RFID peripherals. |
| RF02 | BLE connection | The app shall connect to a selected ESP32 peripheral. |
| RF03 | Connection status | The app shall show disconnected, scanning, connecting, connected, and error states. |
| RF04 | Start inventory | The app shall allow the user to start an RFID inventory operation. |
| RF05 | Stop inventory | The app shall allow the user to stop an active inventory operation. |
| RF06 | Display tag reads | The app shall display tag EPC values received from the ESP32. |
| RF07 | Display RSSI | The app shall display RFID RSSI when the reader protocol provides it. |
| RF08 | Tag deduplication | The app shall avoid presenting duplicate tag reads in a confusing way during a scan session. |
| RF09 | Command log | The app shall provide a development/debug view of commands, responses, and errors. |
| RF10 | Reader settings | The app shall expose reader settings supported by firmware, such as RF power and region. |
| RF11 | Tag write workflow | The app shall support writing/programming tags if the reader protocol and tag memory behavior are confirmed. |

## ESP32 Firmware

| ID | Requirement | Description |
|---|---|---|
| RF12 | BLE peripheral | The ESP32 shall advertise as a BLE peripheral for the iPhone app. |
| RF13 | Custom GATT service | The ESP32 shall expose a custom GATT service for RFID commands and events. |
| RF14 | Command characteristic | The ESP32 shall receive app commands over a writable BLE characteristic. |
| RF15 | Event characteristic | The ESP32 shall send RFID events to the app over a notifiable BLE characteristic. |
| RF16 | UART communication | The ESP32 shall communicate with the YRM100 over 3.3V TTL UART. |
| RF17 | Reader enable control | The ESP32 shall control or account for the YRM100 EN pin behavior. |
| RF18 | Reader response parsing | The ESP32 shall parse YRM100 responses into stable internal events. |
| RF19 | Error reporting | The ESP32 shall report reader, UART, command, timeout, and BLE errors to the app. |
| RF20 | Reader configuration | The ESP32 shall configure region and RF power if the YRM100 protocol supports it. |

## RFID Reader Operations

| ID | Requirement | Description |
|---|---|---|
| RF21 | Inventory tags | The system shall support inventory/scanning of nearby EPC Gen2 tags. |
| RF22 | Read tag memory | The system shall support reading tag memory banks if the YRM100 protocol supports it. |
| RF23 | Write tag memory | The system shall support writing tag memory only after protocol and tag safety rules are documented. |
| RF24 | Stop reader activity | The system shall provide a reliable way to stop continuous inventory or active RF operation. |
| RF25 | Buffer handling | The system shall handle reader-side tag buffers without stale or misleading tag results. |

## Development and Diagnostics

| ID | Requirement | Description |
|---|---|---|
| RF26 | Serial debug | Firmware shall expose enough serial logging for bench testing without the iPhone app. |
| RF27 | Hardware bring-up mode | Firmware shall support a minimal hardware test path for UART, EN pin, and reader status. |
| RF28 | Protocol testability | Reader protocol parsing shall be testable without requiring live RF hardware. |
| RF29 | Documentation updates | Any confirmed protocol details shall be captured in project documentation. |

## Explicitly Deferred

| ID | Requirement | Reason |
|---|---|---|
| DEF01 | Cloud sync | Not needed for first hardware/app prototype. |
| DEF02 | User accounts | Not needed for local prototype. |
| DEF03 | App Store release | Blocked until distribution strategy and Apple Developer Program decision. |
| DEF04 | Production asset database | Requires product workflow decisions not made yet. |
