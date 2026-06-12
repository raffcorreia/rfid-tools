# Implementation Plan - RFID Tools

**Status**: Draft
**Created**: 2026-06-10

---

## Index

- [Scope Summary](#scope-summary)
  - [What Is Being Built](#what-is-being-built)
  - [What Is Explicitly Out of Scope](#what-is-explicitly-out-of-scope)
  - [Key Requirements](#key-requirements)
  - [Technology Stack](#technology-stack)
  - [Key Rules](#key-rules)
- [Phase Index](#phase-index)
- [Phases](#phases)
  - [PHASE-000 - Repository Scaffolding and Tooling](#phase-000---repository-scaffolding-and-tooling)
  - [PHASE-001 - Module Wiring and Hardware Bring-Up](#phase-001---module-wiring-and-hardware-bring-up)
  - [PHASE-002 - YRM100 UART Protocol Driver](#phase-002---yrm100-uart-protocol-driver)
  - [PHASE-003 - ESP32 BLE RFID Service](#phase-003---esp32-ble-rfid-service)
  - [PHASE-004 - iOS App Shell and BLE Connection](#phase-004---ios-app-shell-and-ble-connection)
  - [PHASE-005 - Inventory, Read, and Save Labeled Tags](#phase-005---inventory-read-and-save-labeled-tags)
  - [PHASE-006 - Write and Clone Workflows](#phase-006---write-and-clone-workflows)
  - [PHASE-007 - Diagnostics, Testing, and QA Hardening](#phase-007---diagnostics-testing-and-qa-hardening)
- [Bugfix Phases](#bugfix-phases)
- [Notes](#notes)

---

## Scope Summary

RFID Tools is a native iPhone + ESP32 + YRM100 UHF RFID project. The goal is to read, save, write, and clone compatible UHF RFID tags without depending on vendor Windows software or executing vendor SDK code.

### What Is Being Built

- Native iOS app in Swift / SwiftUI.
- CoreBluetooth central implementation for iPhone.
- ESP32 firmware acting as BLE peripheral and RFID controller.
- YRM100 UART protocol driver implemented in project-owned firmware.
- Module-to-module wiring reference for ESP32 and YRM100.
- RFID inventory and tag read workflows.
- Saved tag reads with user labels such as `PLA Blue TAG`.
- Manual write workflow for compatible tags.
- Clone workflow from saved read to compatible writable tag.
- Diagnostics and test vectors for protocol validation.

### What Is Explicitly Out of Scope

- Executing vendor SDK binaries, installers, demos, scripts, APKs, or JARs.
- Using vendor Windows/C#/Android binaries as runtime dependencies.
- App Store or TestFlight distribution in the first implementation phase.
- Cloud sync or production backend.
- Multi-user accounts.
- Circumventing locked, passworded, killed, or protected tag features.
- Physical layout documentation beyond module-to-module wiring.

### Key Requirements

| ID | Requirement | Source |
|---|---|---|
| RF01-RF03 | BLE scan/connect/status | requirements/03 |
| RF04-RF08 | Inventory and live tag display | requirements/03 |
| RF12-RF20 | Saved reads, labels, write, clone, verification | requirements/03 |
| RF21-RF30 | ESP32 BLE peripheral and YRM100 UART control | requirements/03 |
| RF31-RF38 | RFID inventory/read/write/stop/buffer handling | requirements/03 |
| RF43-RF48 | Diagnostics, protocol tests, module wiring reference | requirements/03 |
| SDK Safety | Vendor SDK reference-only policy | requirements/08 |

### Technology Stack

| Layer | Technology |
|---|---|
| iOS app | Swift, SwiftUI, CoreBluetooth |
| Firmware | ESP32-S3 SuperMini, firmware framework deferred until development workflow validation |
| RFID module | YRM100 over 3.3V TTL UART |
| RFID protocol | EPCglobal UHF Class 1 Gen 2 / ISO 18000-6C |
| Local storage | SwiftData or SQLite-backed persistence, final choice deferred |
| Protocol fixtures | `tools/protocol/test-vectors/` |
| Documentation | Markdown |

### Key Rules

| Rule | Description |
|---|---|
| R-01 | Vendor SDK is reference material only. |
| R-02 | iOS app talks to ESP32 using high-level BLE commands, not raw YRM100 UART frames. |
| R-03 | ESP32 owns YRM100 UART frame generation/parsing. |
| R-04 | Writes require explicit user confirmation. |
| R-05 | Clone means copying supported writable data to a compatible tag, not bypassing protection. |
| R-06 | Lock, kill, ReadProtect, EAS, and similar commands are disabled until deliberately designed. |
| R-07 | Module wiring must document power, ground, EN, UART TX/RX, and connector orientation. |
| R-08 | Hardware bring-up must be possible without the iPhone app. |

---

## Phase Index

| Phase | Name | Maps To | Depends On |
|---|---|---|---|
| PHASE-000 | Repository Scaffolding and Tooling | docs/solution-design/12 | none |
| PHASE-001 | Module Wiring and Hardware Bring-Up | docs/solution-design/04, 10, 13 | PHASE-000 |
| PHASE-002 | YRM100 UART Protocol Driver | docs/solution-design/05, 08 | PHASE-001 |
| PHASE-003 | ESP32 BLE RFID Service | docs/solution-design/05, 06 | PHASE-002 |
| PHASE-004 | iOS App Shell and BLE Connection | docs/solution-design/07 | PHASE-003 can be stubbed |
| PHASE-005 | Inventory, Read, and Save Labeled Tags | requirements/03 RF04-RF18 | PHASE-003, PHASE-004 |
| PHASE-006 | Write and Clone Workflows | requirements/03 RF17-RF20, RF33-RF36 | PHASE-005 |
| PHASE-007 | Diagnostics, Testing, and QA Hardening | docs/solution-design/10, 11, 13 | PHASE-006 |

---

## Phases

---

### PHASE-000 - Repository Scaffolding and Tooling

#### Phase Identity

**Phase ID**: PHASE-000
**Name**: Repository Scaffolding and Tooling
**Requirement Mapping**: Foundation

#### Objective

The repository has the planned code directories, project placeholders, test-vector location, and documentation links needed for app and firmware work to begin without mixing concerns.

#### Dependencies

None.

#### Deliverables

- Create planned directories:
  - `ios/RFIDTools/`
  - `firmware/esp32/`
  - `tools/protocol/test-vectors/`
  - `tools/protocol/scripts/`
- Add placeholder README files in each top-level code/tool area explaining ownership.
- Decide initial firmware framework after ESP32-S3 SuperMini development workflow validation, or document it as deferred.
- Add a root-level repo map if README becomes too long.
- Ensure `.gitignore` covers Xcode, firmware, local capture, and vendor artifacts.

#### Acceptance Criteria

- `ios/`, `firmware/`, and `tools/protocol/` exist.
- No vendor SDK files are copied into the repo.
- README links to requirements and solution design.
- Empty directories are represented with purposeful placeholder docs.

#### QA Notes

- Confirm `git status --ignored` shows `.DS_Store` ignored.
- Confirm no `.exe`, `.dll`, `.apk`, `.jar`, `.rar`, or vendor binaries are staged.

---

### PHASE-001 - Module Wiring and Hardware Bring-Up

#### Phase Identity

**Phase ID**: PHASE-001
**Name**: Module Wiring and Hardware Bring-Up
**Requirement Mapping**: RF25, RF26, RF44, RF47, RF48

#### Objective

The ESP32 and YRM100 are wired with a documented module-to-module mapping, powered safely, and able to return at least one valid YRM100 UART response.

#### Dependencies

- PHASE-000.
- ESP32-S3 SuperMini board selected.
- YRM100 module and connector cable available.

#### Deliverables

- Module wiring reference with:
  - ESP32 board model.
  - YRM100 connector pin mapping.
  - ESP32 UART TX/RX GPIOs.
  - EN handling: tied high or GPIO-controlled.
  - Power source for YRM100 VCC.
  - Common ground confirmation.
  - Connector orientation and module pin 1 reference.
- Hardware bring-up notes.
- First valid UART response capture from YRM100.

#### Suggested Bring-Up Order

1. Confirm power rail and ground.
2. Confirm EN state.
3. Confirm UART pins.
4. Send get-module-info or single-inventory command.
5. Verify frame starts with `0xBB` and ends with `0x7E`.
6. Record final wiring.

#### Acceptance Criteria

- YRM100 does not brown out or reset under basic communication.
- UART returns at least one valid response frame.
- TX/RX direction is confirmed.
- Module wiring reference is updated from assumptions to confirmed values.

#### QA Notes

- Do not attempt tag writes in this phase.
- Use conservative/default RF behavior.

---

### PHASE-002 - YRM100 UART Protocol Driver

#### Phase Identity

**Phase ID**: PHASE-002
**Name**: YRM100 UART Protocol Driver
**Requirement Mapping**: RF25, RF27, RF28, RF31-RF38, RF45

#### Objective

The firmware can build, send, receive, parse, and validate YRM100 frames using project-owned code and documented test vectors.

#### Dependencies

- PHASE-001.
- SDK audit findings in `requirements/08-sdk-audit.md`.

#### Deliverables

- `Yrm100Driver` firmware module.
- Frame builder:
  - header `0xBB`
  - type
  - command
  - 2-byte payload length
  - payload
  - checksum
  - end `0x7E`
- Stream parser:
  - finds frame header
  - handles split frames
  - validates length
  - validates checksum
  - emits parsed response/notice/error
- Core command builders:
  - single inventory `0x22`
  - multiple inventory `0x27`
  - stop inventory `0x28`
  - read memory `0x39`
  - write memory `0x49`
  - set/get region `0x07` / `0x08`
  - set/get TX power `0xB6` / `0xB7`
- Test vectors for checksum, inventory, read, write, power, and malformed frames.

#### Acceptance Criteria

- Known SDK examples encode to expected bytes.
- Known response frames parse into expected fields.
- Bad checksum frames are rejected.
- Multiple frames in one UART buffer are parsed correctly.
- Split frames across reads are parsed correctly.

#### QA Notes

- Parser tests should run without live hardware.
- Raw frame logging should be available for bench diagnostics.

---

### PHASE-003 - ESP32 BLE RFID Service

#### Phase Identity

**Phase ID**: PHASE-003
**Name**: ESP32 BLE RFID Service
**Requirement Mapping**: RF21-RF30

#### Objective

The ESP32 advertises as an RFID BLE peripheral, accepts high-level app commands, and emits RFID events without exposing raw YRM100 frames as the normal app API.

#### Dependencies

- PHASE-002.

#### Deliverables

- BLE service with stable documented UUIDs.
- Command characteristic.
- Event notification characteristic.
- Status/capability characteristic.
- Firmware command dispatcher.
- Basic commands:
  - getInfo/status
  - start inventory
  - stop inventory
  - get/set power
  - get/set region
- Event types:
  - connected/status
  - scan started/stopped
  - tag seen
  - command result
  - error

#### Acceptance Criteria

- iPhone or BLE debugging tool can discover ESP32 service.
- Commands produce corresponding result/error events.
- Active scan stops on explicit stop command.
- Active scan stops or settles on BLE disconnect.
- Firmware reports capability/version data.

#### QA Notes

- BLE protocol encoding must be documented as implementation solidifies.
- Write commands may be stubbed until PHASE-006.

---

### PHASE-004 - iOS App Shell and BLE Connection

#### Phase Identity

**Phase ID**: PHASE-004
**Name**: iOS App Shell and BLE Connection
**Requirement Mapping**: RF01-RF03, RF09

#### Objective

The native iOS app can scan for, connect to, disconnect from, and display status for the ESP32 RFID peripheral.

#### Dependencies

- PHASE-003, or a BLE stub peripheral for app-only development.

#### Deliverables

- Xcode project under `ios/RFIDTools/`.
- SwiftUI app shell.
- CoreBluetooth manager.
- Reader connection screen.
- Connection status model.
- Diagnostics screen for BLE events.
- Basic command send and event receive path.

#### Acceptance Criteria

- App runs on a real iPhone.
- App discovers ESP32 peripheral.
- App connects and subscribes to event notifications.
- App shows disconnected, scanning, connecting, connected, and error states.
- Diagnostics show BLE lifecycle events.

#### QA Notes

- iOS Simulator is not sufficient for BLE validation.
- Free Apple ID signing limitations are accepted for development.

---

### PHASE-005 - Inventory, Read, and Save Labeled Tags

#### Phase Identity

**Phase ID**: PHASE-005
**Name**: Inventory, Read, and Save Labeled Tags
**Requirement Mapping**: RF04-RF18, RF31-RF32, RF39-RF42

#### Objective

The system can scan tags, display live reads, save reads with user labels, and use saved reads as future write/clone sources.

#### Dependencies

- PHASE-003.
- PHASE-004.

#### Deliverables

- iOS scan UI.
- Start/stop inventory commands.
- Live tag list with EPC, RSSI when available, seen count, and timestamp.
- Duplicate handling.
- Saved tag model and persistence.
- Save tag flow with label entry.
- Saved tags list.
- Tag detail screen.
- Optional read-memory command if stable enough for phase scope.

#### Acceptance Criteria

- One nearby tag can be read and displayed.
- Repeated reads update an existing row or count instead of creating confusing duplicates.
- User can save a tag with a label like `PLA Blue TAG`.
- User can view, rename, and delete saved reads.
- Saved read includes EPC and timestamp at minimum.

#### QA Notes

- Use a controlled tag environment for first tests.
- Keep write functionality disabled in this phase.

---

### PHASE-006 - Write and Clone Workflows

#### Phase Identity

**Phase ID**: PHASE-006
**Name**: Write and Clone Workflows
**Requirement Mapping**: RF11, RF17-RF20, RF33-RF36, RF41-RF42

#### Objective

The system can write supported memory values to compatible tags and clone supported data from a saved read to another compatible writable tag.

#### Dependencies

- PHASE-005.
- Sacrificial writable test tags.
- Confirmed tag memory behavior for H9 tags.

#### Deliverables

- Firmware write command support.
- Firmware read-back verification path.
- iOS manual write screen.
- iOS clone-from-saved-read workflow.
- Confirmation screen showing:
  - source
  - target memory bank
  - word offset
  - data length
  - hex data
- Write result and verification result display.
- Safety lockout for protected operations.

#### Acceptance Criteria

- User cannot write without explicit confirmation.
- Manual write can write a supported value to a test tag.
- Clone workflow can write supported saved data to a compatible test tag.
- App does not claim success unless firmware reports success.
- Verification read-back is performed when supported.
- Lock, kill, password/protect commands remain unavailable.

#### QA Notes

- Test only with sacrificial tags.
- Begin at conservative RF power.
- Keep only one target tag in range for first write tests.

---

### PHASE-007 - Diagnostics, Testing, and QA Hardening

#### Phase Identity

**Phase ID**: PHASE-007
**Name**: Diagnostics, Testing, and QA Hardening
**Requirement Mapping**: RF09, RF19, RF28, RF43-RF48

#### Objective

The project is stable enough for repeated local use, with documented QA flows, reliable diagnostics, and enough tests to protect the protocol and write workflows.

#### Dependencies

- PHASE-006.

#### Deliverables

- Firmware counters:
  - BLE connects/disconnects
  - UART frames
  - checksum failures
  - tags seen
  - write attempts/successes/failures
- iOS diagnostics screen refinements.
- Protocol test-vector suite.
- Firmware parser tests.
- iOS BLE decoder tests.
- Manual QA checklist updates.
- Known issues list.

#### Acceptance Criteria

- Protocol tests pass.
- Firmware parser handles malformed frames safely.
- iOS app remains coherent across BLE disconnect/reconnect.
- Write failures are displayed as failures.
- QA checklist can be followed end-to-end.

#### QA Notes

- Run full QA after any protocol or write-flow change.
- Capture failing UART frames as test vectors when possible.

---

## Bugfix Phases

Bugfix work should be tracked as small phase addenda:

| ID | Pattern | Example |
|---|---|---|
| BUG-001 | Firmware protocol fix | checksum, parser, command mapping |
| BUG-002 | BLE interoperability fix | notifications, MTU, reconnect |
| BUG-003 | iOS workflow fix | saved read, write confirmation, diagnostics |
| BUG-004 | Hardware reliability fix | power, EN, UART, RF behavior |

Each bugfix should include:

- reproduction steps
- affected phase
- fix summary
- regression test or QA step

---

## Notes

- The implementation plan is intentionally phased so hardware can be validated before iOS workflows depend on it.
- The iPhone app should never be the only way to test YRM100 communication.
- The vendor SDK remains outside the repo and reference-only.
- ESP32 firmware framework is still a deferred choice.
- Module wiring is tracked; physical layout beyond module-to-module wiring is not tracked.
