# YRM100 Driver

This directory contains the project-owned YRM100 UART protocol driver for `PHASE-002`.

The code is plain C++17 and has no Arduino dependency. That keeps frame building and parsing testable on a development machine before it is wired into the final ESP32 firmware framework.

## Contents

| File | Purpose |
|---|---|
| `Yrm100Driver.h` | Public frame, command, parser, and decoder API |
| `Yrm100Driver.cpp` | Protocol implementation |

## Supported Behavior

- Build `0xBB ... 0x7E` YRM100 frames.
- Validate checksums.
- Parse split byte streams and multiple frames in one stream.
- Reject bad checksum and bad end-marker frames.
- Build core command frames:
  - module info `0x03`
  - inventory `0x22`
  - multiple inventory `0x27`
  - stop inventory `0x28`
  - read memory `0x39`
  - write memory `0x49`
  - select `0x0C`
  - select mode `0x12`
  - region `0x07` / `0x08`
  - TX power `0xB6` / `0xB7`
- Decode:
  - inventory tags into RSSI, PC, EPC, and tag CRC
  - module info responses
  - region responses
  - TX power responses
  - command status responses
  - reader error responses

## Tests

Run from the repository root:

```sh
tools/protocol/scripts/run-yrm100-driver-tests.sh
```

The tests use captured YRM100 frames and SDK-audited command examples. They do not require the RFID hardware or vendor SDK.
