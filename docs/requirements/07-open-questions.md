# 07 - Open Questions

## YRM100 Protocol

| ID | Question | Why It Matters | Status |
|---|---|---|---|
| OQ01 | What is the exact UART frame format? | Required to implement ESP32 driver. | Open |
| OQ02 | What checksum or CRC is used? | Required for command generation and response validation. | Open |
| OQ03 | What are the command IDs for inventory, stop inventory, read, write, set power, and set region? | Required for firmware implementation. | Open |
| OQ04 | What are all response and error codes? | Required for reliable app/firmware errors. | Open |
| OQ05 | Does the reader support single inventory and continuous inventory separately? | Affects scan workflow and stop behavior. | Open |
| OQ06 | How is the reader-side tag buffer cleared? | Prevents stale reads. | Open |
| OQ07 | How is RSSI encoded in tag responses? | Required for signal display. | Open |

## Region and RF Power

| ID | Question | Why It Matters | Status |
|---|---|---|---|
| OQ08 | Is the purchased module preconfigured for US 902-928 MHz? | Required for compliant operation. | Open |
| OQ09 | Can region/frequency be changed by command? | Affects firmware settings and safety rules. | Open |
| OQ10 | How are frequency hopping channels configured? | Required for correct regional behavior. | Open |
| OQ11 | How is output power set from 15-26 dBm? | Required for reader settings. | Open |
| OQ12 | What is the safest default RF power for bench testing? | Reduces accidental long-range reads. | Open |

## Tag Memory

| ID | Question | Why It Matters | Status |
|---|---|---|---|
| OQ13 | What EPC length is used by the INVETON IN9654 / H9 tags by default? | Affects parsing and display. | Open |
| OQ14 | Do the tags expose user memory? | Affects future data model and write workflows. | Open |
| OQ15 | What is the TID format for these tags? | Useful for tag identification and validation. | Open |
| OQ16 | Are the tags rewritable, and what memory banks are safe to write? | Required before write support. | Open |
| OQ17 | Can the H9 tag EPC be rewritten with the YRM100? | Required for clone-to-EPC workflows. | Open |
| OQ18 | Does the H9 tag have user memory, and if so how much? | Required for arbitrary data writes beyond EPC. | Open |
| OQ19 | Which memory banks are readable, writable, lockable, or password-protected by default? | Required for safe read/write UI. | Open |

## Hardware and Wiring

| ID | Question | Why It Matters | Status |
|---|---|---|---|
| OQ20 | Are RXD/TXD labels from the module perspective? | Prevents incorrect wiring. | Open |
| OQ21 | What is the exact WAFER connector pitch/type? | Required for cabling/enclosure. | Open |
| OQ22 | Must EN be held high continuously during operation? | Required for power and sleep design. | Open |
| OQ23 | What is the wake timing after EN is asserted? | Affects firmware startup sequencing. | Open |
| OQ24 | Which ESP32 board will be used? | Affects pin mapping, power, and enclosure. | Open |

## Bluetooth Architecture

| ID | Question | Why It Matters | Status |
|---|---|---|---|
| OQ25 | Does the YRM100 module itself have usable Bluetooth/BLE support? | Could change architecture and remove ESP32 bridge. | Open |
| OQ26 | If Bluetooth exists, is it iPhone-compatible BLE or another profile? | iOS supports BLE well, but classic SPP is not generally available. | Open |
| OQ27 | What BLE service and characteristic layout should the ESP32 expose? | Required for app/firmware contract. | Open |
| OQ28 | What message encoding should be used over BLE? | Affects reliability, debugging, and future compatibility. | Open |

## iOS Development

| ID | Question | Why It Matters | Status |
|---|---|---|---|
| OQ29 | How much weekly free-signing friction is acceptable during development? | Affects testing cadence. | Open |
| OQ30 | Will Apple Developer Program enrollment be needed later? | Required for TestFlight/App Store and easier device distribution. | Open |
| OQ31 | What iOS versions should be supported? | Affects CoreBluetooth and SwiftUI choices. | Open |
| OQ32 | What local persistence should saved tag reads use? | Affects data model and clone workflow. | Open |

## SDK Review

| ID | Question | Why It Matters | Status |
|---|---|---|---|
| OQ33 | Which files in the SDK contain protocol documentation? | Identifies safe reference material. | Open |
| OQ34 | Does the SDK include Arduino, ESP32, or Raspberry Pi source code? | May reveal protocol behavior for reimplementation. | Open |
| OQ35 | What is the SDK license or redistribution status? | Affects whether snippets or docs can be included. | Open |
