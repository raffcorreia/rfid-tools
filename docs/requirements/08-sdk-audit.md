# 08 - SDK Audit

## Scope

This document records a read-only audit of `../YRM100 SDK 20240525/`.

The SDK was treated as untrusted vendor material. No binaries, installers, scripts, APKs, JARs, demos, or sample applications were executed.

## High-Level Finding

The SDK is usable as protocol reference material, but it should not be used as a runtime dependency for this project.

The project should reimplement the protocol in project-owned ESP32 firmware and iOS code. Vendor source can be used only to understand behavior and cross-check the protocol documentation.

## Useful Reference Files

| File | Usefulness |
|---|---|
| `YRM100 user protocol/Communication user Protocol V2.1_en.docx` | Primary protocol specification. |
| `YRM100 user protocol/Quick use of protocol commands referrence.docx` | Practical examples for inventory, select, read, and write. |
| `YRM100 command.txt` | Plain command-frame examples. |
| `YRM100 connect with Arduino/EPS32 STM32 connect YRM module.txt` | Wiring note for ESP32/STM32-style connection. |
| `PC demo+driver/PC demo software User's manual（ENG).docx` | PC demo usage notes, including EPC write offset guidance. |
| `YRM100 sdk reference/C#DEMO source code sample/.../Commands.cs` | Cross-check for frame building and checksum behavior. |
| `YRM100 sdk reference/C#DEMO source code sample/.../ReceiveParser.cs` | Cross-check for frame parsing behavior. |
| `YRM100 sdk reference/Bluetooth WiFi handheld terminal demo source code sample/Handheld terminal SDK/jar note.txt` | Notes about Android Bluetooth sample and power commands. |

## Unsafe / Not Used Directly

| Content | Reason |
|---|---|
| Windows `.exe`, `.dll`, `.sys`, `.vxd`, `.msi` style files | Executable/vendor binary material. |
| Windows USB drivers | Not needed for ESP32/iOS design and not safe to install casually. |
| Android `.apk` files | Executable/vendor app material. |
| Java `.jar` files | Binary dependency with unclear licensing and target platform. |
| `.rar` / `.zip` archives with demos or drivers | May contain executable material; inspect listings only unless a later controlled extraction is explicitly needed. |
| Vendor C# library binaries | Windows/.NET demo dependency, not relevant for ESP32 firmware. |

## Protocol Summary

### Frame Format

```text
Header Type Command PL_MSB PL_LSB Parameters... Checksum End
BB     xx   xx      xx     xx     ...           xx       7E
```

| Field | Meaning |
|---|---|
| Header | Always `0xBB`. |
| Type | `0x00` command, `0x01` response, `0x02` notice. |
| Command | Operation code. |
| PL | 2-byte parameter length. |
| Parameters | Command-specific payload. |
| Checksum | Low byte of sum from Type through Parameters. |
| End | Always `0x7E`. |

### Core Commands

| Command | Code | Notes |
|---|---:|---|
| Get module info | `0x03` | Hardware, software, manufacturer info. |
| Single inventory | `0x22` | One inventory operation. |
| Multiple inventory | `0x27` | Repeated inventory with count. |
| Stop multiple inventory | `0x28` | Stops active multiple inventory. |
| Set select parameters | `0x0C` | Selects target tag/memory mask. |
| Set select mode | `0x12` | Controls whether Select is sent before operations. |
| Read memory | `0x39` | Reads a memory bank by word address/length. |
| Write memory | `0x49` | Writes a memory bank by word address/length; max 32 words / 64 bytes per docs. |
| Set region | `0x07` | USA region code is `0x02`. |
| Get region | `0x08` | Reads current region. |
| Set channel | `0xAB` | Sets working channel index. |
| Get channel | `0xAA` | Reads working channel index. |
| Frequency hopping | `0xAD` | `0xFF` enable, `0x00` disable. |
| Get TX power | `0xB7` | Returns power encoded as dBm x 100. |
| Set TX power | `0xB6` | Sets power encoded as dBm x 100. |
| Sleep | `0x17` | Module sleep command. |
| Set idle sleep time | `0x1D` | Idle sleep time command. |

### Inventory Response

Successful inventory produces notice frames with:

- RSSI
- PC
- EPC
- tag CRC

RSSI is a signed/complement-coded dBm value. The protocol example states `0xC9` represents about `-55 dBm`.

### Read / Write Memory

Read and write operations use:

- 4-byte access password
- memory bank
- segment address in words
- data length in words
- data payload for writes

The protocol says a Select command should be sent before Read/Write to select a single tag.

Memory-bank values from the Select documentation:

| Value | Bank |
|---:|---|
| `0b00` | RFU / Reserved |
| `0b01` | EPC |
| `0b10` | TID |
| `0b11` | User |

The PC demo manual states that EPC writes start at word offset `00 02`, while other writable areas except TID start at word offset `0`.

## Bluetooth Finding

The SDK contains Android Bluetooth handheld-terminal examples, but the reviewed code uses classic Bluetooth RFCOMM/SPP behavior, including UUID `00001101-0000-1000-8000-00805F9B34FB`.

That is not a good fit for iPhone, where BLE/CoreBluetooth is the practical route for custom hardware communication. This supports keeping the ESP32 BLE bridge architecture unless the physical YRM100 module is later proven to expose iPhone-compatible BLE.

## Design Implications

- Use the protocol docs as the source of truth.
- Reimplement YRM100 UART frame building and parsing in ESP32 firmware.
- Keep the iPhone app talking to a higher-level ESP32 BLE API.
- Do not expose raw YRM100 frames as the normal iOS app API.
- Support read/save/write/clone workflows through explicit firmware commands.
- Treat lock, kill, ReadProtect, EAS, and vendor/tag-specific commands as out of scope until deliberately designed.

## Remaining Risks

- The purchased module's shipping region/default power is still not confirmed.
- H9 tag memory layout and actual writability still need hardware validation.
- SDK licensing/redistribution terms are not clear.
- The Arduino archive may contain useful examples, but it has not been extracted or reviewed yet.
