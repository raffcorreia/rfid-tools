# YRM100 UART Bring-Up Sketch

This Arduino-compatible sketch is for `PHASE-001` hardware validation only. It does not include BLE or app behavior.

## Purpose

- Verify ESP32-to-YRM100 wiring.
- Send a known YRM100 UART command.
- Print raw response frames on USB serial.
- Validate `0xBB ... 0x7E` framing and checksum.

## Wiring

Use the populated YRM100 `J1` cable, not the unconfirmed `J2` middle pads.

| YRM100 | ESP32-S3 SuperMini |
|---|---|
| `GND` / blue | `GND` |
| `EN` / green | `3V3` |
| `RXD` / yellow | board-labeled `TX` |
| `TXD` / black | board-labeled `RX` |
| `VCC` / red | `5V` USB rail, or external regulated 5V with common ground |

Default sketch pin assumptions:

| Signal | ESP32 GPIO |
|---|---|
| ESP32 RX from YRM100 TXD | Arduino board `RX` constant, fallback `GPIO44` |
| ESP32 TX to YRM100 RXD | Arduino board `TX` constant, fallback `GPIO43` |

These are based on common ESP32-S3 SuperMini `RX` / `TX` labels. `GPIO43` and `GPIO44` are valid ESP32-S3 pins and are commonly used for UART0 on ESP32-S3 boards. If there is no response, verify the selected board package pin map and TX/RX wiring.

The sketch prints the resolved GPIO numbers on boot. Those values are the firmware configuration, not proof that the physical silkscreen pads use those GPIOs.

The sketch also uses the onboard addressable status LED:

| Color | Meaning |
|---|---|
| Dim red | ESP32 is sending bytes to YRM100 |
| Dim green | ESP32 received bytes from YRM100 |

Receive indication takes priority over transmit indication. The default status LED pin is `GPIO48` with brightness `3`. If your board uses a different RGB LED pin, override `STATUS_LED_PIN` before upload.

UART wiring is crossed:

| Controller Side | Reader Side |
|---|---|
| ESP32 `TX` | YRM100 `RXD` / yellow |
| ESP32 `RX` | YRM100 `TXD` / black |

Do not wire `RX` to `RXD` and `TX` to `TXD` unless you are deliberately testing whether the labels are from the opposite perspective. If testing, swap only the yellow and black UART wires; never move power or ground while powered.

## Run

1. Open `yrm100_bringup.ino` in Arduino IDE.
2. Select an ESP32-S3 board profile compatible with the ESP32-S3 SuperMini.
3. Enable USB CDC on boot if the board profile exposes that option.
4. Upload the sketch.
5. Open Serial Monitor at `115200` baud.

Recommended ESP32-S3 Arduino IDE settings:

| Setting | Value |
|---|---|
| USB CDC On Boot | `Enabled` |
| Upload Speed | `115200` while troubleshooting |
| Serial Monitor baud | `115200` |

If Serial Monitor is blank after upload:

- Close and reopen Serial Monitor after upload.
- Reselect the current ESP32 port under `Tools > Port`.
- Press `RESET` once after Serial Monitor is open.
- Confirm USB CDC on boot is enabled, then re-upload.
- Type `h` in Serial Monitor and press send.
- Use the USB-C connector on the ESP32 board, not an external UART adapter, unless the board profile is configured for UART serial.

On boot, the sketch sends:

```text
BB 00 03 00 01 00 04 7E
```

This is the get-hardware-version command from the vendor command examples and should return a YRM100 frame if UART wiring is correct.

Serial monitor commands:

| Key | Action |
|---|---|
| `g` | Send hardware version, software version, and manufacturer commands |
| `r` | Send get region and get TX power commands |
| `i` | Send single inventory |
| `s` | Send stop multiple inventory |
| `b` | Toggle YRM100 UART baud between `115200` and `38400`, then send hardware version |
| `v` | Visual TX test: switch UART to `1200` baud and send a long `0x55` pattern |
| `h` | Print help |

The `v` command is only for checking whether the configured ESP32 TX pin is physically toggling. It is not a valid YRM100 command, and the reader is not expected to answer it.

For a temporary LED test, use a resistor:

```text
ESP32 TX pin -> 1k resistor -> LED anode
LED cathode -> GND
```

At `1200` baud with repeated `0x55`, the TX line alternates slowly enough to be easier to see than normal `115200` UART traffic. Remove the LED after the test if there is any doubt that it is loading the UART line.

## Success Criteria

A useful first success looks like:

```text
[RX FRAME] BB ... 7E
[RX FRAME] type=0x.. command=0x.. payload_len=.. checksum=OK end=OK
```

If no bytes arrive:

- Confirm `EN` is tied high.
- Confirm common ground.
- Confirm YRM100 VCC is stable.
- Swap UART TX/RX wires once before changing firmware assumptions.
- Type `b` to test the vendor-mentioned alternate `38400` baud.
- Verify the board-labeled `TX` / `RX` pins map to `GPIO43` / `GPIO44` for the selected board profile.

If the boot message prints but there are no `[RX ...]` lines after sending `g` or `r`, the ESP32 is not receiving any bytes from the YRM100. That points to wiring, power, enable, UART pin selection, or baud rate rather than USB serial.
