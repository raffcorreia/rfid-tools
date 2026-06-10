# 03 - Architecture

> Layer: Foundation | Depends on: 02-technology-stack

## Architectural Style

Small embedded bridge architecture with a native mobile client.

The iPhone app owns user workflows and local saved data. The ESP32 owns BLE transport, YRM100 command sequencing, RF safety defaults, and reader parsing. The YRM100 remains a UART-controlled RFID module.

## Component Diagram

```text
┌─────────────────────────────────────────────────────────────┐
│                         iPhone                              │
│  SwiftUI App                                                │
│  ┌──────────────┐ ┌─────────────┐ ┌──────────────────────┐ │
│  │ Scan UI      │ │ Saved Tags  │ │ Write / Clone UI     │ │
│  └──────┬───────┘ └──────┬──────┘ └──────────┬───────────┘ │
│         └────────────────┴──────────────┬────┘             │
│                                  CoreBluetooth              │
└───────────────────────────────────────┬─────────────────────┘
                                        │ BLE GATT
                                        ▼
┌─────────────────────────────────────────────────────────────┐
│                         ESP32                               │
│  ┌────────────┐ ┌─────────────┐ ┌────────────────────────┐ │
│  │ BLE Server │ │ Command API │ │ YRM100 Protocol Driver │ │
│  └─────┬──────┘ └──────┬──────┘ └───────────┬────────────┘ │
│        │               │                    │ UART         │
│        └───────────────┴────────────────────┘              │
│  USB serial diagnostics                                      │
└───────────────────────────────────────┬─────────────────────┘
                                        │ 3.3V TTL UART
                                        ▼
┌─────────────────────────────────────────────────────────────┐
│                         YRM100                              │
│  Integrated UHF RFID reader / writer + antenna              │
└───────────────────────────────────────┬─────────────────────┘
                                        │ UHF RF
                                        ▼
┌─────────────────────────────────────────────────────────────┐
│                    EPC Gen2 UHF Tags                        │
└─────────────────────────────────────────────────────────────┘
```

## Main Data Flows

### Inventory

```text
User taps Scan
  -> iOS sends START_INVENTORY over BLE
  -> ESP32 sends YRM100 multiple inventory command
  -> YRM100 emits notice frames with RSSI, PC, EPC, CRC
  -> ESP32 parses frames and deduplicates/throttles as needed
  -> ESP32 sends TAG_SEEN events over BLE
  -> iOS displays and optionally saves labeled read
```

### Save Labeled Read

```text
User selects a detected tag
  -> iOS stores label, EPC, memory data if present, RSSI, timestamp
  -> saved read becomes available as clone/write source
```

### Write / Clone

```text
User selects manual value or saved read
  -> iOS shows target memory bank, offset, length, and value
  -> user confirms write
  -> iOS sends WRITE_TAG command over BLE
  -> ESP32 selects target tag when needed
  -> ESP32 sends YRM100 write memory command
  -> ESP32 reads back when possible
  -> iOS shows verified/success/failure state
```

## Key Boundaries

| Boundary | Contract |
|---|---|
| iOS <-> ESP32 | High-level BLE messages, not raw YRM100 frames for normal use. |
| ESP32 <-> YRM100 | Raw documented YRM100 UART frames. |
| App storage | Local saved reads and labels. |
| Vendor SDK | Documentation/reference only. |

## Failure Handling

- BLE disconnect stops active scan from the app perspective.
- ESP32 should stop multiple inventory when the app disconnects or sends stop.
- YRM100 timeouts become explicit firmware error events.
- Write failures include reader error code when available.
- Clone is not successful unless write acknowledgement and optional read-back verification pass.
