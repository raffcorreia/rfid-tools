# RFID Tools

RFID Tools is an iPhone-centered RFID reader/writer project for reading, saving, writing, and cloning UHF RFID tags through an ESP32-controlled YRM100 integrated reader module.

The planned system connects a native iOS app to an ESP32 over Bluetooth Low Energy. The ESP32 controls the YRM100 module over 3.3V TTL UART, and the YRM100 reads or writes EPC Gen2 / ISO 18000-6C UHF RFID tags.

```text
iPhone app <BLE> ESP32 <UART> YRM100 reader <UHF RF> RFID tags
```

This architecture is provisional. If the YRM100 documentation proves that the module has usable standalone Bluetooth support, the ESP32 bridge may be reduced or removed.

## Current Direction

- Native iOS app written in Swift / SwiftUI.
- CoreBluetooth used directly for BLE communication.
- ESP32 acts as a BLE peripheral and RFID controller.
- YRM100 reader is controlled over TTL UART.
- Scanned tag reads can be saved with user labels, such as `PLA Blue TAG`.
- Supported tag data can be written manually or cloned from a saved read onto a compatible writable tag.
- Vendor SDK is treated as untrusted reference material only.
- Protocol behavior should be reimplemented from documentation and careful source review, not by executing vendor binaries or scripts.

## Hardware Notes

- Reader module: YRM100 integrated UHF RFID reader/writer module.
- Reader interface: 3.3V TTL UART.
- Reader supply: DC 3.5-5V, with peak pulse current below 260mA according to vendor material.
- RFID protocol: EPCglobal UHF Class 1 Gen 2 / ISO 18000-6C.
- Region target: US frequency band, 902-928 MHz.
- Tags: INVETON IN9654 UHF paper tags, H9 chip, 96 x 22 mm, 860-960 MHz.
- Controller: ESP32-S3 SuperMini development board.

## Documentation

Requirements are tracked in:

- [docs/requirements/01-overview.md](docs/requirements/01-overview.md)
- [docs/requirements/02-hardware-inventory.md](docs/requirements/02-hardware-inventory.md)
- [docs/requirements/03-functional-requirements.md](docs/requirements/03-functional-requirements.md)
- [docs/requirements/04-user-stories.md](docs/requirements/04-user-stories.md)
- [docs/requirements/05-non-functional-requirements.md](docs/requirements/05-non-functional-requirements.md)
- [docs/requirements/06-business-rules.md](docs/requirements/06-business-rules.md)
- [docs/requirements/07-open-questions.md](docs/requirements/07-open-questions.md)
- [docs/requirements/08-sdk-audit.md](docs/requirements/08-sdk-audit.md)

Solution design is tracked in:

- [docs/solution-design/00-index.md](docs/solution-design/00-index.md)

Implementation tracking is available in:

- [docs/implementation/plan.md](docs/implementation/plan.md)
- [docs/implementation/phase-status.md](docs/implementation/phase-status.md)

## Project Structure

```text
rfid-tools/
  docs/                  Documentation, plans, and QA records
  firmware/esp32/        ESP32 firmware for BLE and YRM100 UART control
  ios/RFIDTools/         Native iOS app
  tools/protocol/        Protocol test vectors and helper scripts
```
