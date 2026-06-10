# 07 - Open Questions

## YRM100 Protocol

| ID | Question | Why It Matters | Status |
|---|---|---|---|
| OQ01 | What is the exact UART frame format? | Required to implement ESP32 driver. | Answered: see `02-hardware-inventory.md` and `08-sdk-audit.md`. |
| OQ02 | What checksum or CRC is used? | Required for command generation and response validation. | Answered: low byte sum from Type through Parameter; tag CRC is separate data in inventory notices. |
| OQ03 | What are the command IDs for inventory, stop inventory, read, write, set power, and set region? | Required for firmware implementation. | Answered for core commands. |
| OQ04 | What are all response and error codes? | Required for reliable app/firmware errors. | Partially answered: SDK protocol has an error-code summary; firmware still needs final mapping. |
| OQ05 | Does the reader support single inventory and continuous inventory separately? | Affects scan workflow and stop behavior. | Answered: single inventory `0x22`, multiple inventory `0x27`, stop multiple inventory `0x28`. |
| OQ06 | How is the reader-side tag buffer cleared? | Prevents stale reads. | Open. |
| OQ07 | How is RSSI encoded in tag responses? | Required for signal display. | Answered: signed/complement-coded dBm in inventory notice RSSI byte. |

## Region and RF Power

| ID | Question | Why It Matters | Status |
|---|---|---|---|
| OQ08 | Is the purchased module preconfigured for US 902-928 MHz? | Required for compliant operation. | Still open: command support exists, shipping default not confirmed. |
| OQ09 | Can region/frequency be changed by command? | Affects firmware settings and safety rules. | Answered: set/get region, set/get channel, and frequency hopping commands exist. |
| OQ10 | How are frequency hopping channels configured? | Required for correct regional behavior. | Partially answered: FHSS enable/disable and channel formulas are documented; final US channel plan still needs validation. |
| OQ11 | How is output power set from 15-26 dBm? | Required for reader settings. | Answered: set TX power command uses dBm x 100. |
| OQ12 | What is the safest default RF power for bench testing? | Reduces accidental long-range reads. | Open |

## Tag Memory

| ID | Question | Why It Matters | Status |
|---|---|---|---|
| OQ13 | What EPC length is used by the INVETON IN9654 / H9 tags by default? | Affects parsing and display. | Open |
| OQ14 | Do the tags expose user memory? | Affects future data model and write workflows. | Open |
| OQ15 | What is the TID format for these tags? | Useful for tag identification and validation. | Open |
| OQ16 | Are the tags rewritable, and what memory banks are safe to write? | Required before write support. | Open |
| OQ17 | Can the H9 tag EPC be rewritten with the YRM100? | Required for clone-to-EPC workflows. | Partially answered: YRM100 supports EPC writes; H9 tag behavior still needs hardware validation. |
| OQ18 | Does the H9 tag have user memory, and if so how much? | Required for arbitrary data writes beyond EPC. | Open |
| OQ19 | Which memory banks are readable, writable, lockable, or password-protected by default? | Required for safe read/write UI. | Open |

## Hardware and Wiring

| ID | Question | Why It Matters | Status |
|---|---|---|---|
| OQ20 | Are RXD/TXD labels from the module perspective? | Prevents incorrect wiring. | Open |
| OQ21 | What is the exact WAFER connector pitch/type? | Required for cabling/enclosure. | Open |
| OQ22 | Must EN be held high continuously during operation? | Required for power and sleep design. | Open |
| OQ23 | What is the wake timing after EN is asserted? | Affects firmware startup sequencing. | Open |
| OQ24 | Which ESP32 board will be used? | Affects pin mapping, power, and enclosure. | Answered: ESP32-S3 SuperMini development board. |
| OQ25 | What exact ESP32-to-YRM100 module wiring will be used for the first prototype? | Required for reproducible hardware setup. | Open |
| OQ26 | What power source will supply YRM100 VCC during prototype testing? | Required to avoid unstable reads or brownouts. | Open |
| OQ27 | Should EN be tied high or controlled by an ESP32 GPIO? | Affects wiring and firmware bring-up. | Open |
| OQ28 | What is the exact pinout for the selected ESP32-S3 SuperMini board variant? | SuperMini boards vary by seller and silkscreen; pin choices must match the physical board. | Open |

## Bluetooth Architecture

| ID | Question | Why It Matters | Status |
|---|---|---|---|
| OQ29 | Does the YRM100 module itself have usable Bluetooth/BLE support? | Could change architecture and remove ESP32 bridge. | Partially answered: SDK includes Android Bluetooth terminal samples, but not evidence of YRM100 onboard BLE. |
| OQ30 | If Bluetooth exists, is it iPhone-compatible BLE or another profile? | iOS supports BLE well, but classic SPP is not generally available. | Likely not iPhone-compatible from SDK sample: Android sample uses classic RFCOMM/SPP UUID `00001101-0000-1000-8000-00805F9B34FB`; still open for actual purchased hardware. |
| OQ31 | What BLE service and characteristic layout should the ESP32 expose? | Required for app/firmware contract. | Open |
| OQ32 | What message encoding should be used over BLE? | Affects reliability, debugging, and future compatibility. | Open |

## iOS Development

| ID | Question | Why It Matters | Status |
|---|---|---|---|
| OQ33 | How much weekly free-signing friction is acceptable during development? | Affects testing cadence. | Open |
| OQ34 | Will Apple Developer Program enrollment be needed later? | Required for TestFlight/App Store and easier device distribution. | Open |
| OQ35 | What iOS versions should be supported? | Affects CoreBluetooth and SwiftUI choices. | Open |
| OQ36 | What local persistence should saved tag reads use? | Affects data model and clone workflow. | Open |

## SDK Review

| ID | Question | Why It Matters | Status |
|---|---|---|---|
| OQ37 | Which files in the SDK contain protocol documentation? | Identifies safe reference material. | Answered: see `08-sdk-audit.md`. |
| OQ38 | Does the SDK include Arduino, ESP32, or Raspberry Pi source code? | May reveal protocol behavior for reimplementation. | Partially answered: SDK includes Arduino-related archives/wiring notes and Android/C# sources; ESP32-specific source not yet confirmed. |
| OQ39 | What is the SDK license or redistribution status? | Affects whether snippets or docs can be included. | Open |
