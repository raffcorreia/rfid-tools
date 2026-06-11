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

## Alternate YRM100 J2 Pads

The physical YRM100 board also exposes an unpopulated 5-pad `J2` footprint near the left edge of the board. User-provided photos show the populated `J1` cable colors as:

| J1 Wire Color | Signal |
|---|---|
| Blue | `GND` |
| Green | `EN` |
| Yellow | `RXD` |
| Black | `TXD` |
| Red | `VCC` / `5V` |

Multimeter continuity testing currently confirms only the power rails between `J2` and `J1`:

| J2 Pad, Top to Bottom | Confirmed / Suspected Signal | Status |
|---|---|---|
| 1, square top pad | `VCC` / `5V` | Confirmed continuity to red J1 wire |
| 2 | Unknown | No direct continuity to J1 signal confirmed |
| 3 | Unknown | No direct continuity to J1 signal confirmed |
| 4 | Unknown | No direct continuity to J1 signal confirmed |
| 5, bottom pad | `GND` | Confirmed continuity to blue J1 wire |

Do not use the middle `J2` pads for UART or `EN` until their function is confirmed. The populated `J1` cable remains the preferred first-bring-up connector.

## Module Wiring Plan

The project should document the module-to-module wiring only.

The wiring reference should include:

- ESP32 board model used.
- ESP32 GPIO selected for YRM100 UART TX/RX.
- ESP32 GPIO selected for YRM100 EN, if controlled by firmware.
- YRM100 pin-to-ESP32 signal mapping.
- Power source and voltage rail used for YRM100 VCC.
- Common ground between ESP32 and YRM100.
- Confirmation that UART logic is 3.3V TTL.
- Notes about wire colors, connector orientation, and module pin 1 orientation.
- Bring-up checklist for power, EN, UART, and first inventory command.

Initial vendor wiring note from SDK:

| ESP32/STM32 Side | YRM-RFID Side |
|---|---|
| `3.3V` | `5V` / red wire plus `EN` / green wire |
| `GND` | `GND` / blue wire |
| `B10` | `RXD` / yellow wire |
| `B11` | `TXD` / black wire |

This note is useful but not final. The actual ESP32 GPIO names and TX/RX direction must be confirmed with the selected ESP32 board and module cable.

Power caution:

- The YRM100 should not be powered from an ESP32 GPIO.
- The YRM100 supply must support at least the documented peak pulse current below 260mA.
- Prototype jumper wiring should be treated as temporary until stable behavior is confirmed.

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

The selected controller is an ESP32-S3 SuperMini development board.

Photo-derived external pinout from the selected physical board:

Component side, USB-C connector at the top:

| Left edge, top to bottom | Right edge, top to bottom |
|---|---|
| `TX` | `5V` |
| `RX` | `GND` |
| `1` | `3V3` |
| `2` | `13` |
| `3` | `12` |
| `4` | `11` |
| `5` | `10` |
| `6` | `9` |
| `7` | `8` |

Back side, USB-C connector at the top:

| Left edge, top to bottom | Right edge, top to bottom |
|---|---|
| `5V` | `TX` |
| `GND` | `RX` |
| `3V3` | `1` |
| `13` | `2` |
| `12` | `3` |
| `11` | `4` |
| `10` | `5` |
| `9` | `6` |
| `8` | `7` |

The first prototype should use the external edge pins only. The back side also exposes additional inner pads, but those are not needed for initial wiring and should not be part of the first breadboard setup.

First prototype YRM100 wiring:

| YRM100 Pin | YRM100 Signal | ESP32-S3 SuperMini Connection | Direction / Purpose | Validation |
|---|---|---|---|---|
| 1 | `GND` | `GND` | Common ground | Photo-derived board pin; required |
| 2 | `EN` | `3V3` | Holds reader enabled | Uses vendor EN high threshold greater than 1.5V |
| 3 | `RXD` | `TX` | ESP32 transmits to YRM100 | Validate with first command response |
| 4 | `TXD` | `RX` | ESP32 receives from YRM100 | Validate with first command response |
| 5 | `VCC` | `5V` | Reader power from USB-powered board rail | Must remain stable under peak pulse current below 260mA |

If the reader is unstable, use an external regulated 5V supply for YRM100 VCC with common ground to the ESP32.

Validation criteria:

- Stable power path for the RFID module.
- Available hardware UART pins.
- BLE support.
- USB serial debugging.
- Enough GPIO for EN/reset/status controls.
- Practical enclosure and mounting options.
- Confirmed board pinout matching the physical board markings.

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

## Confirmed SDK Reference Contents

| Item | Finding |
|---|---|
| User protocol | `YRM100 user protocol/Communication user Protocol V2.1_en.docx` |
| Quick protocol guide | `YRM100 user protocol/Quick use of protocol commands referrence.docx` |
| Plain command examples | `YRM100 command.txt` |
| ESP32/STM32 wiring note | `YRM100 connect with Arduino/EPS32 STM32 connect YRM module.txt` |
| Windows sample source | C# demo source and parser/library source are included. |
| Android sample source | Android serial and Bluetooth/WiFi handheld terminal samples are included. |
| Windows executables/drivers | Multiple `.exe`, `.dll`, `.sys`, `.apk`, `.jar`, `.rar`, and installer files are present and must not be executed. |

## Confirmed Protocol Facts

| Item | Finding |
|---|---|
| UART framing | Frames start with `0xBB` and end with `0x7E`. |
| Frame fields | Header, type, command, 2-byte parameter length, parameters, checksum, end. |
| Command type | `0x00` command, `0x01` response, `0x02` notice. |
| Checksum | Low byte of the sum from Type through Parameter. Header and end are excluded. |
| Single inventory | Command `0x22`, frame `BB 00 22 00 00 22 7E`. |
| Multiple inventory | Command `0x27`, with reserved byte `0x22` and 2-byte count. |
| Stop multiple inventory | Command `0x28`, frame `BB 00 28 00 00 28 7E`. |
| Read memory | Command `0x39`; requires access password, memory bank, word address, and word length. |
| Write memory | Command `0x49`; supports writing up to 32 words / 64 bytes per command according to protocol docs. |
| Select tag | Command `0x0C` sets select parameters; command `0x12` controls select mode. |
| Region | Command `0x07` set region, `0x08` get region. USA region code is `0x02`. |
| Channel | Command `0xAB` set channel, `0xAA` get channel. American band channel formula is `(frequency MHz - 902.25) / 0.5`. |
| Frequency hopping | Command `0xAD`; `0xFF` enables hopping, `0x00` disables it. |
| TX power | Command `0xB6` set power, `0xB7` get power. Values are encoded as dBm x 100, e.g. `0x07D0` = 20 dBm and `0x0A28` = 26 dBm. |
| RSSI | Inventory notice includes RSSI; value is signed/complement-coded dBm, e.g. `0xC9` means about `-55 dBm`. |
| UART settings | Protocol document states 8 data bits, 1 stop bit, no parity. Vendor material recommends 115200 bps. |
| EPC write offset | PC demo manual says EPC writing starts at word offset `00 02`; other areas except TID start at `0`. |

## SDK Safety Policy

The SDK directory is treated as untrusted vendor material.

- Do not execute vendor binaries.
- Do not execute vendor scripts.
- Do not install vendor drivers from this workspace without explicit approval.
- Do not run sample code directly.
- Inspect documents and source code as reference material only.
- Reimplement required behavior in project-owned code.
