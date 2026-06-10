# Phase Status - RFID Tools

**Current Phase**: PHASE-001 - Module Wiring and Hardware Bring-Up
**Current Status**: In Progress
**Previous Phase**: PHASE-000 - Repository Scaffolding and Tooling
**Last Updated**: 2026-06-10

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

### PHASE-001 - Module Wiring and Hardware Bring-Up

| Item | Status | Evidence |
|---|---|---|
| Select ESP32 board | Done | ESP32-S3 SuperMini development board selected |
| Confirm ESP32-S3 SuperMini exact pinout | Done | External edge pins documented from user-provided board photos |
| Choose ESP32 UART TX/RX GPIOs | Done | Use board-labeled `TX` and `RX` pins for first bring-up |
| Decide EN handling | Done | Tie YRM100 `EN` to ESP32 `3V3` for first bring-up |
| Decide YRM100 VCC power source | Done | Use ESP32 `5V` USB rail first; external regulated 5V fallback if unstable |
| Document module wiring table | Done | See `docs/requirements/02-hardware-inventory.md` and `docs/solution-design/04-hardware-design.md` |
| Capture first valid YRM100 UART response | Open | Requires wiring |

## Phase History

| Phase | Status | Started | Completed | Notes |
|---|---|---|---|---|
| PHASE-000 | Complete | 2026-06-10 | 2026-06-10 | Repository scaffolding and ownership docs created. |
| PHASE-001 | In Progress | 2026-06-10 | - | ESP32-S3 SuperMini board selected and first prototype wiring documented; first UART response still open. |

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

## Open Follow-Ups

| ID | Phase | Follow-Up | Status |
|---|---|---|---|
| FU-000-01 | PHASE-000 | Commit PHASE-000 scaffolding after review. | Done |
| FU-001-01 | PHASE-001 | Select ESP32 board. | Done |
| FU-001-02 | PHASE-001 | Decide YRM100 power source for prototype. | Done |
| FU-001-03 | PHASE-001 | Confirm exact ESP32-S3 SuperMini pinout from the physical board. | Done |
| FU-001-04 | PHASE-001 | Wire the modules and capture the first valid YRM100 UART response. | Open |
