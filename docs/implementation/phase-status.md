# Phase Status - RFID Tools

**Current Phase**: PHASE-000 - Repository Scaffolding and Tooling
**Current Status**: Complete
**Next Phase**: PHASE-001 - Module Wiring and Hardware Bring-Up
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

### PHASE-000 - Repository Scaffolding and Tooling

| Item | Status | Evidence |
|---|---|---|
| Create `ios/RFIDTools/` | Done | `ios/RFIDTools/README.md` |
| Create `firmware/esp32/` | Done | `firmware/esp32/README.md` |
| Create `tools/protocol/test-vectors/` | Done | `tools/protocol/test-vectors/README.md` |
| Create `tools/protocol/scripts/` | Done | `tools/protocol/scripts/README.md` |
| Add ownership README files | Done | README files in code/tool directories |
| Document firmware framework decision | Done | Deferred until ESP32 board selection |
| Add root repo map | Done | `README.md` Project Structure |
| Confirm SDK files are not copied | Done | No vendor SDK files in repo |
| Confirm ignored macOS noise | Done | `.DS_Store` is ignored |

## Phase History

| Phase | Status | Started | Completed | Notes |
|---|---|---|---|---|
| PHASE-000 | Complete | 2026-06-10 | 2026-06-10 | Repository scaffolding and ownership docs created. |

## Decisions

| Date | Phase | Decision | Reason |
|---|---|---|---|
| 2026-06-10 | PHASE-000 | Keep iOS and firmware source separate. | Swift and ESP32 firmware should stay idiomatic; BLE docs and test vectors are the shared contract. |
| 2026-06-10 | PHASE-000 | Defer firmware framework choice. | Exact ESP32 board is not selected yet. |
| 2026-06-10 | PHASE-000 | Keep vendor SDK outside repo. | SDK is reference-only and contains untrusted executable material. |

## Open Follow-Ups

| ID | Phase | Follow-Up | Status |
|---|---|---|---|
| FU-000-01 | PHASE-000 | Commit PHASE-000 scaffolding after review. | Done |
| FU-001-01 | PHASE-001 | Select ESP32 board. | Open |
| FU-001-02 | PHASE-001 | Decide YRM100 power source for prototype. | Open |
