# 09 - Safety and Security Design

> Layer: Experience | Depends on: 03-architecture, 08-rfid-protocol-design

## Safety Principles

- Reading is low risk; writing is high risk.
- Writes require explicit confirmation.
- Clone means copying supported writable memory, not bypassing security.
- Protected operations are out of scope until deliberately designed.
- Vendor SDK code is not executed.

## Tag Write Safety

| Risk | Control |
|---|---|
| Accidental write | Confirmation screen with bank, offset, and data |
| Wrong target tag | Select target EPC and require single-target validation |
| Invalid memory range | Firmware validates bank, offset, and length |
| Silent failed clone | Require acknowledgement and read-back when possible |
| Protected tag operations | Hide lock/kill/password/protect commands |

## RF Safety

- Default to conservative RF power during bring-up.
- Stop scan on BLE disconnect.
- Stop scan on explicit app command.
- Avoid continuous carrier commands in normal workflows.
- Keep region fixed to US unless explicitly changed in development tools.

## BLE Security

Early prototypes may use simple BLE access. Before real asset use, revisit:

- pairing/bonding
- app-level command authorization
- write-command lockout
- device identity
- firmware capability reporting

## Data Privacy

Saved tag reads are local to the iPhone in phase one.

No personal data is required. Exports or cloud sync, if added later, must be designed separately.

## SDK Safety

- No vendor executables.
- No vendor installers.
- No vendor APK/JAR runtime dependency.
- Source and docs are reference only.
- Protocol is reimplemented in project-owned code.
