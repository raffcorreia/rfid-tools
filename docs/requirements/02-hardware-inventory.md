# 02 - Hardware Inventory

## RFID Reader Module

| Item | Value |
|---|---|
| Module | YRM100 integrated UHF RFID reader/writer module |
| Antenna | Integrated antenna |
| Interface | TTL UART |
| UART level | 3.3V TTL |
| Default baud rate | 115200 bps, vendor-recommended |
| Alternate baud rate mentioned | 38400 bps |
| Working voltage | DC 3.5-5V |
| Working current | 180mA at 3.5V and 26 dBm, 25C |
| Lower-power current example | 110mA at 3.5V and 18 dBm, 25C |
| Peak pulse current | Below 260mA |
| Standby current | Below 80mA when EN is high |
| Sleep current | Below 100uA when EN is low |
| Start time | Below 100ms |
| Cooling | Air cooling, no external heat sink according to vendor material |

## RFID Capabilities

| Item | Value |
|---|---|
| Air protocol | EPCglobal UHF Class 1 Gen 2 / ISO 18000-6C |
| Working spectrum | 840-960 MHz |
| US frequency option | 902-928 MHz |
| EU frequency option | 865-868 MHz |
| Target region | United States |
| Output power range | 15-26 dBm |
| Output power accuracy | +/- 1 dB |
| Output power flatness | +/- 0.2 dB |
| Receiving sensitivity | Below -70 dBm |
| RSSI | Supported according to vendor material |
| Inventory speed | More than 50 tags per second |
| Tag buffer | 200 labels at 96-bit EPC |
| Claimed read distance | 0-6m with vendor example UHF RFID card |

## Connector Pinout

Vendor material shows a 5-pin WAFER connector.

| Pin | Name | Notes |
|---|---|---|
| 1 | GND | Ground |
| 2 | EN | High-level enable, greater than 1.5V |
| 3 | RXD | 3.3V TTL receive pin |
| 4 | TXD | 3.3V TTL transmit pin |
| 5 | VCC | DC 3-5V |

The TX/RX direction must be confirmed from the module perspective before wiring.

## Tags

| Item | Value |
|---|---|
| Brand shown | INVETON |
| Product text | IN9654 |
| Type | UHF RFID paper tag / label |
| Use case | Warehouse / asset management |
| Chip | H9 |
| Size | 96 x 22 mm |
| Frequency | 860-960 MHz |

## ESP32 Controller

Multiple ESP32 boards are available. The exact board will be selected later.

Selection criteria:

- Stable power path for the RFID module.
- Available hardware UART pins.
- BLE support.
- USB serial debugging.
- Enough GPIO for EN/reset/status controls.
- Practical enclosure and mounting options.

## Vendor SDK

| Item | Value |
|---|---|
| Local path | `../YRM100 SDK 20240525/` |
| Vendor link | `https://od.lk/f/NzNfMTE5MTY5NjQyXw` |
| Vendor claim | Compatible with Arduino, ESP32, and Raspberry Pi |
| SDK default target | Windows |
| Demo software | `NEW DEMO 2.2` |
| Driver folder | `PC demo+driver` |
| Blue USB connector driver | `CP210x_Universal_Windows_Driver.zip` |
| Black USB connector driver | `CH341SER Drivers.EXE` |

## SDK Safety Policy

The SDK directory is treated as untrusted vendor material.

- Do not execute vendor binaries.
- Do not execute vendor scripts.
- Do not install vendor drivers from this workspace without explicit approval.
- Do not run sample code directly.
- Inspect documents and source code as reference material only.
- Reimplement required behavior in project-owned code.
