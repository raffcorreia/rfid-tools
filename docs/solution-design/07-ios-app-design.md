# 07 - iOS App Design

> Layer: Experience | Depends on: 03-architecture, 06-ble-protocol-design

## App Structure

| Area | Responsibility |
|---|---|
| BLE Manager | CoreBluetooth scan/connect/disconnect/characteristics |
| RFID Session Store | Current scan state and live tag list |
| Saved Tag Store | Local saved reads and labels |
| Write Coordinator | Manual write and clone workflows |
| Diagnostics Store | In-app command/event/error log |

## Main Screens

| Screen | Purpose |
|---|---|
| Reader | Connect to ESP32, show reader state, start/stop scan |
| Live Scan | Display current tag reads, RSSI, counts, save action |
| Saved Tags | Show labeled saved reads such as `PLA Blue TAG` |
| Tag Detail | Show EPC, memory data, RSSI history, timestamp |
| Write / Clone | Choose manual data or saved read source, confirm write |
| Settings | Reader power slider, region/status, firmware info |
| Diagnostics | BLE and RFID command/event log |

## Saved Tag Data

Local saved read fields:

| Field | Description |
|---|---|
| id | App-local identifier |
| label | User label |
| epc | EPC when captured |
| pc | Protocol control value when captured |
| tid | TID if read later |
| userMemory | User memory if read later |
| rssiDbm | RSSI at capture or latest scan |
| firstSeenAt | Timestamp |
| updatedAt | Timestamp |

## User Workflows

### Save a Tag

1. User scans tags.
2. User selects a detected tag.
3. User enters label.
4. App saves captured data locally.

### Clone a Tag

1. User opens a saved read.
2. User chooses clone/write.
3. App shows memory bank, offset, and data to write.
4. User confirms.
5. App sends write command.
6. App displays write and verification result.

## UX Rules

- Write actions must be visually distinct from read-only actions.
- The app must show connected reader state before allowing RF operations.
- The app must show which saved read or manual value is the write source.
- The app must not claim a clone is complete without firmware success and optional verification.
- Diagnostics should be available but not dominate normal scanning.
- Tag detail and diagnostics views should include a hex/text toggle for payload display, with hex as the default for EPC and protocol fields.

## Reader Power

- The default reader power for the real app should be `15 dBm`.
- The settings screen should expose a slider for selecting power before sending it to the ESP32.

## iOS Constraints

- BLE testing requires a real iPhone.
- Free Apple ID signing creates weekly redeploy friction.
- App Store/TestFlight distribution is deferred.
