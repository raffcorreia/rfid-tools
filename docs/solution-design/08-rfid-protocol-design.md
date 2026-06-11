# 08 - RFID Protocol Design

> Layer: Core Design | Depends on: 05-firmware-design

## Source of Truth

The YRM100 protocol implementation should be based on:

- `requirements/08-sdk-audit.md`
- `Communication user Protocol V2.1_en.docx`
- `Quick use of protocol commands referrence.docx`
- `YRM100 command.txt`

Vendor demo binaries and libraries are not runtime dependencies.

## Frame Format

```text
BB type command len_msb len_lsb payload... checksum 7E
```

Checksum is the low byte of the sum from `type` through the final payload byte.

## Command Mapping

| Firmware Operation | YRM100 Command |
|---|---|
| Get hardware/software/manufacturer | `0x03` with selector `0x00`, `0x01`, or `0x02` |
| Single inventory | `0x22` |
| Multiple inventory | `0x27` |
| Stop multiple inventory | `0x28` |
| Select tag | `0x0C` |
| Set select mode | `0x12` |
| Read memory | `0x39` |
| Write memory | `0x49` |
| Set region | `0x07` |
| Get region | `0x08` |
| Set channel | `0xAB` |
| Get channel | `0xAA` |
| Frequency hopping | `0xAD` |
| Get TX power | `0xB7` |
| Set TX power | `0xB6` |

## Inventory Parsing

Inventory notice frames include:

- RSSI
- PC
- EPC
- CRC

The driver should normalize RSSI into signed dBm. EPC length should be derived from frame/payload structure rather than assuming all tags are 96-bit forever.

## Memory Banks

| Bank | Meaning |
|---|---|
| Reserved | Passwords and protected data; out of scope initially |
| EPC | EPC memory, EPC writes start at word offset `0x0002` |
| TID | Tag identifier; expected read-only for this project |
| User | User memory if tag supports it |

## Write Strategy

- Select target tag before write when target EPC is known.
- Limit writes to documented memory banks and ranges.
- Default access password is `00000000` unless user specifies otherwise later.
- Verify by read-back when possible.
- Do not expose lock, kill, ReadProtect, EAS, or vendor-specific commands in normal UI.

## Region and Power

- Use USA region code `0x02`.
- Use conservative default power for bring-up.
- Encode power as dBm x 100.
- Example: `20 dBm` = `2000` decimal = `0x07D0`.

## Parser Tests

Parser tests should include:

- valid single inventory notice
- no-tag response
- malformed checksum
- split frame across reads
- multiple frames in one buffer
- write success response
- write failure response
