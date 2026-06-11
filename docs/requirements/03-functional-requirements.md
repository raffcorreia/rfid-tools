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
| RF06a | Data format toggle | The app shall allow the user to switch tag data display between hex and text when a readable text view is available. |
| RF07 | Display RSSI | The app shall display RFID RSSI when the reader protocol provides it. |
| RF08 | Tag deduplication | The app shall avoid presenting duplicate tag reads in a confusing way during a scan session. |
| RF09 | Command log | The app shall provide a development/debug view of commands, responses, and errors. |
| RF10 | Reader settings | The app shall expose reader settings supported by firmware, such as RF power and region. |
| RF11 | Tag write workflow | The app shall support writing/programming tags if the reader protocol and tag memory behavior are confirmed. |
| RF12 | Save tag read | The app shall allow the user to save a tag read for later use. |
| RF13 | Label saved tag | The app shall allow the user to assign a human label to a saved tag read, such as `PLA Blue TAG`. |
| RF14 | View saved tags | The app shall provide a list of saved tag reads with labels and captured tag data. |
| RF15 | Edit saved label | The app shall allow the user to rename a saved tag label. |
| RF16 | Delete saved read | The app shall allow the user to delete a saved tag read from local storage. |
| RF17 | Clone from saved read | The app shall allow the user to select a saved read as the source for writing another compatible tag. |
| RF18 | Arbitrary write input | The app shall allow the user to enter supported tag memory values manually for writing. |
| RF19 | Write confirmation | The app shall require explicit confirmation before writing to a tag. |
| RF20 | Write verification | The app shall show whether a write was verified by reading back the written value when supported. |

## ESP32 Firmware

| ID | Requirement | Description |
|---|---|---|
| RF21 | BLE peripheral | The ESP32 shall advertise as a BLE peripheral for the iPhone app. |
| RF22 | Custom GATT service | The ESP32 shall expose a custom GATT service for RFID commands and events. |
| RF23 | Command characteristic | The ESP32 shall receive app commands over a writable BLE characteristic. |
| RF24 | Event characteristic | The ESP32 shall send RFID events to the app over a notifiable BLE characteristic. |
| RF25 | UART communication | The ESP32 shall communicate with the YRM100 over 3.3V TTL UART. |
| RF26 | Reader enable control | The ESP32 shall control or account for the YRM100 EN pin behavior. |
| RF27 | Reader response parsing | The ESP32 shall parse YRM100 responses into stable internal events. |
| RF28 | Error reporting | The ESP32 shall report reader, UART, command, timeout, and BLE errors to the app. |
| RF29 | Reader configuration | The ESP32 shall configure region and RF power if the YRM100 protocol supports it. |
| RF30 | Write command support | The ESP32 shall expose write commands over BLE only after YRM100 write behavior is confirmed. |

## RFID Reader Operations

| ID | Requirement | Description |
|---|---|---|
| RF31 | Inventory tags | The system shall support inventory/scanning of nearby EPC Gen2 tags. |
| RF32 | Read tag memory | The system shall support reading tag memory banks if the YRM100 protocol supports it. |
| RF33 | Write tag memory | The system shall support writing tag memory only after protocol and tag safety rules are documented. |
| RF34 | Write EPC | The system shall support writing EPC memory if supported by the reader and target tag. |
| RF35 | Write user memory | The system shall support writing user memory if the target tag exposes user memory. |
| RF36 | Clone supported data | The system shall support copying supported readable/writable data from one saved tag read to another compatible tag. |
| RF37 | Stop reader activity | The system shall provide a reliable way to stop continuous inventory or active RF operation. |
| RF38 | Buffer handling | The system shall handle reader-side tag buffers without stale or misleading tag results. |

## Saved Tag Data

| ID | Requirement | Description |
|---|---|---|
| RF39 | Local saved reads | The app shall store saved tag reads locally on the iPhone during the first phase. |
| RF40 | Saved read fields | A saved read shall include label, EPC when available, memory-bank data when available, RSSI when captured, and timestamp. |
| RF41 | Clone source selection | A saved read shall be selectable as the source for a clone/write workflow. |
| RF42 | Manual data source | A manually entered value shall be usable as the source for a write workflow. |

## Development and Diagnostics

| ID | Requirement | Description |
|---|---|---|
| RF43 | Serial debug | Firmware shall expose enough serial logging for bench testing without the iPhone app. |
| RF44 | Hardware bring-up mode | Firmware shall support a minimal hardware test path for UART, EN pin, and reader status. |
| RF45 | Protocol testability | Reader protocol parsing shall be testable without requiring live RF hardware. |
| RF46 | Documentation updates | Any confirmed protocol details shall be captured in project documentation. |
| RF47 | Module wiring reference | The project shall include a documented module wiring reference for the first ESP32-to-YRM100 prototype. |
| RF48 | Wiring validation checklist | The project shall include a checklist to validate power, ground, EN, UART TX/RX, and first reader response. |

## Explicitly Deferred

| ID | Requirement | Reason |
|---|---|---|
| DEF01 | Cloud sync | Not needed for first hardware/app prototype. |
| DEF02 | User accounts | Not needed for local prototype. |
| DEF03 | App Store release | Blocked until distribution strategy and Apple Developer Program decision. |
| DEF04 | Production asset database | Requires product workflow decisions not made yet. |
