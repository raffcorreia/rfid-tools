# Phase Status - RFID Tools

**Current Phase**: PHASE-002 - YRM100 UART Protocol Driver
**Current Status**: In Progress
**Previous Phase**: PHASE-001 - Module Wiring and Hardware Bring-Up
**Last Updated**: 2026-06-11

---

## How Phase Execution Is Tracked

`plan.md` defines the phases, deliverables, dependencies, and acceptance criteria.

This file records execution state:

- current phase
- phase checklist
- evidence produced
- decisions made during the phase
- blockers and open follow-ups
- completion record

The plan should stay mostly stable. This status file should change as work progresses.

## Current Phase Checklist

### PHASE-002 - YRM100 UART Protocol Driver

| Item | Status | Evidence |
|---|---|---|
| Define driver module location and structure | Open | Needs implementation decision |
| Extract frame builder from bring-up sketch | Open | Bring-up sketch has working frame construction |
| Extract stream parser from bring-up sketch | Open | Bring-up sketch has working frame parsing |
| Add typed inventory/tag parsing | Open | Hardware logs confirm RSSI, PC, EPC, and CRC layout |
| Add command builders for core YRM100 operations | Open | Commands proven in bring-up sketch and SDK audit |
| Add protocol test vectors | Open | Need fixtures for valid, split, multi-frame, and bad-checksum cases |
| Add host-runnable parser/builder tests | Open | Tests should run without live RFID hardware |

## Phase History

| Phase | Status | Started | Completed | Notes |
|---|---|---|---|---|
| PHASE-000 | Complete | 2026-06-10 | 2026-06-10 | Repository scaffolding and ownership docs created. |
| PHASE-001 | Complete | 2026-06-10 | 2026-06-11 | ESP32-S3 SuperMini board selected, first prototype wiring documented, valid YRM100 UART responses captured, and tag inventory/read/write validation completed. |
| PHASE-002 | In Progress | 2026-06-11 | - | Started after successful YRM100 hardware bring-up and EPC clone verification. |

## Decisions

| Date | Phase | Decision | Reason |
|---|---|---|---|
| 2026-06-10 | PHASE-000 | Keep iOS and firmware source separate. | Swift and ESP32 firmware should stay idiomatic; BLE docs and test vectors are the shared contract. |
| 2026-06-10 | PHASE-000 | Defer firmware framework choice. | Board and pinout were not confirmed during PHASE-000. |
| 2026-06-10 | PHASE-000 | Keep vendor SDK outside repo. | SDK is reference-only and contains untrusted executable material. |
| 2026-06-10 | PHASE-001 | Use ESP32-S3 SuperMini development board as controller. | User selected this board for the project. |
| 2026-06-10 | PHASE-001 | Use board-labeled `TX` and `RX` pins for YRM100 UART bring-up. | User-provided photos show accessible external UART pins. |
| 2026-06-10 | PHASE-001 | Tie YRM100 `EN` to `3V3` during first bring-up. | Removes firmware-controlled enable as a variable during initial UART validation. |
| 2026-06-10 | PHASE-001 | Power YRM100 from ESP32 `5V` USB rail first, with external regulated 5V as fallback. | The YRM100 accepts DC 3.5-5V and requires peak pulse current below 260mA. |
| 2026-06-11 | PHASE-001 | Use an Arduino-compatible sketch for UART bring-up only. | This validates hardware before the final firmware framework and BLE service are chosen. |
| 2026-06-11 | PHASE-001 | Correct YRM100 `0x03` diagnostic command to include the required selector byte. | Vendor command examples show hardware/software/manufacturer reads as `0x03` with payload `0x00`, `0x01`, or `0x02`. |
| 2026-06-11 | PHASE-001 | Confirm YRM100 UART communication at `115200` baud. | The module returned valid checksum-verified frames for hardware, software, and manufacturer info. |
| 2026-06-11 | PHASE-001 | Complete module bring-up after successful inventory and EPC write verification. | Single inventory, continuous inventory, and EPC clone/write flow all worked with the YRM100 and H9 tags. |
| 2026-06-11 | PHASE-002 | Start protocol driver phase from the proven bring-up sketch. | The bring-up sketch contains validated command bytes, frame parsing, inventory decoding, power commands, and write/clone sequencing that should be formalized into a driver. |

## Open Follow-Ups

| ID | Phase | Follow-Up | Status |
|---|---|---|---|
| FU-000-01 | PHASE-000 | Commit PHASE-000 scaffolding after review. | Done |
| FU-001-01 | PHASE-001 | Select ESP32 board. | Done |
| FU-001-02 | PHASE-001 | Decide YRM100 power source for prototype. | Done |
| FU-001-03 | PHASE-001 | Confirm exact ESP32-S3 SuperMini pinout from the physical board. | Done |
| FU-001-04 | PHASE-001 | Wire the modules and capture the first valid YRM100 UART response. | Done |
| FU-001-05 | PHASE-001 | Upload `yrm100_bringup` sketch and record Serial Monitor output. | Done |
| FU-001-06 | PHASE-001 | Capture `r` output for region and TX power. | Done |
| FU-001-07 | PHASE-001 | Capture first successful single inventory response from an H9 tag. | Done |
| FU-001-08 | PHASE-001 | Investigate ESP32 reset when YRM100 single inventory enables RF. | Done |
| FU-002-01 | PHASE-002 | Decide firmware driver layout and test harness location. | Open |
| FU-002-02 | PHASE-002 | Convert working frame builder/parser code into reusable driver code. | Open |
| FU-002-03 | PHASE-002 | Capture protocol fixtures from confirmed hardware logs. | Open |
| FU-002-04 | PHASE-002 | Add host-runnable parser and frame-builder tests. | Open |
