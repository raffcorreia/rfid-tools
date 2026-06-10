# Solution Design - RFID Tools

> This directory contains the technical solution design for RFID Tools. The documents are progressive: each layer depends on the earlier foundation docs.

## Document Structure

| # | Document | Layer | Description |
|---|---|---|---|
| 00 | index.md | - | This index |
| 01 | solution-overview.md | Foundation | Problem, solution, stakeholders, and scope |
| 02 | technology-stack.md | Foundation | iOS, firmware, hardware, and tooling choices |
| 03 | architecture.md | Foundation | System components and data flow |
| 04 | hardware-design.md | Core Design | YRM100, ESP32, module wiring, power, and bring-up |
| 05 | firmware-design.md | Core Design | ESP32 firmware modules and responsibilities |
| 06 | ble-protocol-design.md | Core Design | iPhone-to-ESP32 BLE service contract |
| 07 | ios-app-design.md | Experience | SwiftUI app structure, screens, and local data |
| 08 | rfid-protocol-design.md | Core Design | YRM100 UART protocol model and command mapping |
| 09 | safety-security-design.md | Experience | Tag write safety, BLE access, and SDK safety |
| 10 | testing.md | Experience | Unit, integration, hardware, and iOS testing strategy |
| 11 | observability.md | Operations | Logs, diagnostics, counters, and debug surfaces |
| 12 | deployment.md | Operations | Local development, signing, flashing, and repo layout |
| 13 | qa-process.md | Operations | Manual QA flow and acceptance criteria |

## Requirement Traceability

| Requirement Area | Requirements Docs | Design Docs |
|---|---|---|
| iOS BLE app | requirements/01, 03, 04, 05 | 02, 03, 06, 07, 09, 10 |
| ESP32 firmware | requirements/01, 03, 05, 06 | 02, 03, 05, 06, 08, 11 |
| YRM100 protocol | requirements/02, 07, 08 | 04, 05, 08, 10 |
| Read/save/write/clone | requirements/03, 04, 06 | 03, 06, 07, 08, 09 |
| Module wiring | requirements/02, 03, 05, 06, 07 | 04, 10, 13 |
| SDK safety | requirements/02, 06, 08 | 08, 09, 12 |

## Architectural Decisions

| Decision | Choice | Rationale |
|---|---|---|
| iOS app technology | Native Swift / SwiftUI | BLE is central, and CoreBluetooth gives direct iOS control. |
| iPhone-to-reader path | iPhone <BLE> ESP32 <UART> YRM100 | SDK Bluetooth sample appears classic SPP, not iPhone-friendly BLE. |
| SDK usage | Reference only | Vendor binaries/libs are untrusted and platform-mismatched. |
| RFID protocol ownership | Reimplement in ESP32 firmware | Keeps Windows/C#/Android vendor code out of runtime. |
| iOS API level | High-level BLE commands | App should not depend on raw YRM100 UART frames for normal use. |
| First persistence | Local iPhone storage | Cloud/backend is out of scope for first phase. |
| Module wiring docs | Module-to-module wiring only | Physical layout is managed outside project docs. |

## Dependencies Between Documents

```text
01-solution-overview
       |
02-technology-stack
       |
03-architecture
    /      |       \
04-hw   05-fw    07-ios
   |       |        |
   +---- 06-ble ----+
           |
        08-rfid
           |
   09-safety-security
           |
10-testing -> 11-observability -> 12-deployment -> 13-qa-process
```
