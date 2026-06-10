# 04 - Hardware Design

> Layer: Core Design | Depends on: 03-architecture

## Hardware Components

| Component | Role |
|---|---|
| iPhone | BLE central and user interface |
| ESP32 | BLE peripheral, UART host, reader controller |
| YRM100 | UHF RFID reader/writer |
| INVETON IN9654 / H9 tags | UHF RFID targets |

## Module Wiring Deliverable

The project will document module-to-module wiring only:

- YRM100 connector pin.
- YRM100 signal name.
- ESP32 GPIO or power rail.
- Direction from ESP32 perspective.
- Wire color when known.
- Notes and validation status.

Physical layout beyond module-to-module wiring is intentionally not tracked.

## YRM100 Connector

| Pin | Signal | Notes |
|---|---|---|
| 1 | GND | Common ground |
| 2 | EN | Enable, high level greater than 1.5V |
| 3 | RXD | 3.3V TTL receive |
| 4 | TXD | 3.3V TTL transmit |
| 5 | VCC | DC 3-5V |

TX/RX direction must be confirmed from the module perspective during bring-up.

## Power Design

- YRM100 VCC must be supplied by a rail capable of the documented peak pulse current below 260mA.
- ESP32 GPIO must not power the YRM100.
- ESP32 and YRM100 must share ground.
- Conservative RF power should be used during bring-up.
- Brownouts, serial corruption, or intermittent reads should first trigger power-path review.

## EN Strategy

Two acceptable prototype options exist:

| Option | Description | Tradeoff |
|---|---|---|
| Tie EN high | Simplest bring-up path | Less firmware control over sleep/wake |
| ESP32 GPIO controls EN | Firmware can sleep/wake reader | Requires documented timing and pin choice |

Final choice is deferred until ESP32 board selection.

## Bring-Up Checklist

1. Confirm YRM100 VCC rail and common ground.
2. Confirm EN state.
3. Confirm ESP32 UART pins and cross TX/RX if required.
4. Send get-module-info or single-inventory command over UART.
5. Confirm valid `0xBB ... 0x7E` response frame.
6. Set/read US region when safe.
7. Test low-power inventory with one tag nearby.
8. Record final module wiring table.

## Hardware Risks

| Risk | Mitigation |
|---|---|
| TX/RX label ambiguity | Validate with simple command and document direction. |
| Insufficient power | Use a stable rail, common ground, and conservative RF power. |
| EN behavior unclear | Start simple, then add GPIO control after timing is known. |
| Multiple tags during writes | Require select workflow and single-target validation. |
