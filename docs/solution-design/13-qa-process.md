# 13 - QA Process

> Layer: Operations | Depends on: 10-testing

## Principle

QA follows the physical system from wiring to app workflow. Each phase must leave enough evidence in logs, screenshots, or notes to reproduce failures.

## Phase Flow

```text
1. Module wiring verified
2. ESP32 UART communication verified
3. YRM100 inventory verified
4. BLE connection verified
5. iOS live scan verified
6. Save labeled read verified
7. Read memory verified
8. Write test tag verified
9. Clone saved read verified
10. Safety/error cases verified
```

## Acceptance Criteria

### Module Wiring

- Power source documented.
- Common ground documented.
- TX/RX direction confirmed.
- EN behavior documented.
- First valid YRM100 response captured.

### Inventory

- Single tag can be read.
- RSSI appears when available.
- Stop scan works.
- Duplicate display remains understandable.

### Saved Reads

- User can save a tag with a label.
- User can rename/delete saved read.
- Saved read preserves original captured values.

### Write / Clone

- Write requires confirmation.
- Write source is visible.
- Target bank, offset, and data are visible.
- Write result is shown.
- Read-back verification is performed when possible.

### Failure Cases

- BLE disconnect stops active workflow.
- No-tag response is handled clearly.
- Write failure is not reported as success.
- Malformed UART frames are logged and ignored.

## Sign-Off Record

Each QA pass should record:

| Field | Value |
|---|---|
| Date | |
| ESP32 board | |
| Firmware version | |
| iOS build | |
| YRM100 region/power | |
| Tag type | |
| Result | |
| Notes | |
