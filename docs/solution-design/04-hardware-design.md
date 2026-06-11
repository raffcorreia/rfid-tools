# 04 - Hardware Design

> Layer: Core Design | Depends on: 03-architecture

## Hardware Components

| Component | Role |
|---|---|
| iPhone | BLE central and user interface |
| ESP32-S3 SuperMini | BLE peripheral, UART host, reader controller |
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

## YRM100 Alternate J2 Pads

The board appears to expose the same five UART/power signals on an unpopulated `J2` footprint. Based on the populated `J1` cable colors and board photos, the probable `J2` order when viewed from the component side with the square pad at the top is:

| J2 Pad, Top to Bottom | Probable Signal |
|---|---|
| 1, square top pad | `VCC` / `5V` |
| 2 | `TXD` |
| 3 | `RXD` |
| 4 | `EN` |
| 5, bottom pad | `GND` |

This mapping must be verified with continuity testing before using `J2`. The first prototype should still use the populated `J1` cable.

## ESP32-S3 SuperMini External Pins

The selected ESP32-S3 SuperMini board has been documented from user-provided photos. With the USB-C connector at the top, the external edge pins expose:

| Component Side Left | Component Side Right |
|---|---|
| `TX`, `RX`, `1`, `2`, `3`, `4`, `5`, `6`, `7` | `5V`, `GND`, `3V3`, `13`, `12`, `11`, `10`, `9`, `8` |

The first hardware bring-up uses only these edge pins. Inner pads visible on the back side are deferred until there is a specific need.

## First Prototype Wiring

| YRM100 Pin | YRM100 Signal | ESP32-S3 SuperMini Connection | Direction from ESP32 | Notes |
|---|---|---|---|---|
| 1 | `GND` | `GND` | Ground | Common ground is required. |
| 2 | `EN` | `3V3` | Enable high | Simplest bring-up path. |
| 3 | `RXD` | `TX` | TX to reader | Crossed UART signal. |
| 4 | `TXD` | `RX` | RX from reader | Crossed UART signal. |
| 5 | `VCC` | `5V` | Power | Use USB-powered board rail first; switch to external regulated 5V if unstable. |

## Power Design

- YRM100 VCC must be supplied by a rail capable of the documented peak pulse current below 260mA.
- ESP32 GPIO must not power the YRM100.
- ESP32 and YRM100 must share ground.
- Conservative RF power should be used during bring-up.
- Brownouts, serial corruption, or intermittent reads should first trigger power-path review.

## EN Strategy

First bring-up ties YRM100 `EN` to ESP32 `3V3`.

This keeps the first UART test independent from firmware-controlled reader power sequencing. A later revision can move `EN` to an ESP32 GPIO if sleep/wake control is needed.

| Option | Description | Tradeoff |
|---|---|---|
| Tie EN high | Selected for first bring-up | Less firmware control over sleep/wake |
| ESP32 GPIO controls EN | Deferred | Requires documented timing and pin choice |

## Bring-Up Checklist

1. Confirm YRM100 VCC rail and common ground.
2. Confirm EN is tied to `3V3`.
3. Confirm ESP32 `TX` goes to YRM100 `RXD` and ESP32 `RX` goes to YRM100 `TXD`.
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
