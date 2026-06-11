# YRM100 Frame Test Vectors

These vectors are project-owned fixtures captured from the SDK audit and confirmed hardware bring-up logs.

## Command Frames

| Name | Bytes | Notes |
|---|---|---|
| Get hardware version | `BB 00 03 00 01 00 04 7E` | Module info selector `0x00`. |
| Single inventory | `BB 00 22 00 00 22 7E` | One inventory operation. |
| Multiple inventory | `BB 00 27 00 03 22 FF FF 4A 7E` | Inventory command `0x22`, count `0xFFFF`. |
| Stop multiple inventory | `BB 00 28 00 00 28 7E` | Stop active continuous inventory. |
| Get region | `BB 00 08 00 00 08 7E` | Reads current region. |
| Get TX power | `BB 00 B7 00 00 B7 7E` | Reads power as dBm x 100. |
| Set TX power 15 dBm | `BB 00 B6 00 02 05 DC 99 7E` | `0x05DC` = `1500` = `15.00 dBm`. |
| Set select mode before tag operations | `BB 00 12 00 01 02 15 7E` | Required before EPC write/clone flow. |
| Set select mode disabled/default | `BB 00 12 00 01 01 14 7E` | Restores default after write attempt. |
| Write EPC | `BB 00 49 00 15 00 00 00 00 01 00 02 00 06 E2 80 11 70 40 00 02 1D 35 AE 40 08 D4 7E` | Writes 12-byte EPC at EPC bank word offset `0x0002`. |

## Response / Notice Frames

| Name | Bytes | Expected Decode |
|---|---|---|
| Hardware version | `BB 01 03 00 10 00 4D 31 30 30 20 32 36 64 42 6D 20 56 31 2E 30 92 7E` | selector `0x00`, text `M100 26dBm V1.0` |
| Region response | `BB 01 08 00 01 01 0B 7E` | region `0x01` |
| TX power response | `BB 01 B7 00 02 0A 28 EC 7E` | `0x0A28` = `2600` = `26.00 dBm` |
| No tag / read failed | `BB 01 FF 00 01 15 16 7E` | error code `0x15` |
| Write failure observed before select-mode fix | `BB 01 FF 00 01 10 11 7E` | error code `0x10` |
| Inventory tag | `BB 02 22 00 11 D6 34 00 E2 80 11 70 40 00 02 1D 35 AE 40 08 D4 44 C4 7E` | RSSI `0xD6`, PC `0x3400`, EPC `E2 80 11 70 40 00 02 1D 35 AE 40 08`, CRC `0xD444` |

## Malformed Frames

| Name | Bytes | Expected Result |
|---|---|---|
| Bad checksum | `BB 01 08 00 01 01 00 7E` | Parser rejects with bad checksum. |
| Bad end byte | `BB 01 08 00 01 01 0B 00` | Parser rejects with bad end marker. |
