# 01 - Solution Overview

> Layer: Foundation | Depends on: requirements/01-overview

## Problem

The project needs a practical way to read, save, write, and clone compatible UHF RFID tags from an iPhone, without relying on vendor Windows demo software or executing untrusted SDK code.

## Solution

Build a native iOS app that talks to an ESP32 over Bluetooth Low Energy. The ESP32 controls the YRM100 RFID reader over 3.3V TTL UART using a project-owned implementation of the documented YRM100 protocol.

```text
iPhone Swift app <BLE> ESP32 firmware <UART> YRM100 reader <UHF> RFID tags
```

## Stakeholders

| Stakeholder | Interest |
|---|---|
| Builder / Developer | Hardware bring-up, firmware, diagnostics, protocol validation |
| Operator | Scanning tags, saving labeled reads, writing/cloning tags |
| Future Asset Manager | Associating saved tag reads with real assets |

## Scope

### In Scope

- Native iPhone app using SwiftUI and CoreBluetooth.
- ESP32 firmware exposing a BLE peripheral service.
- YRM100 UART protocol implementation in firmware.
- Module wiring reference for ESP32 and YRM100.
- Read/inventory workflows.
- Saved tag reads with user labels.
- Manual write workflow for supported memory values.
- Clone workflow from saved read to compatible writable tag.
- Local iPhone persistence.
- Hardware and protocol diagnostics.

### Out of Scope

- App Store release in the first phase.
- TestFlight unless Apple Developer Program enrollment is added.
- Cloud sync or production backend.
- Multi-user accounts.
- Executing vendor SDK binaries or using vendor libraries as dependencies.
- Circumventing locked, passworded, killed, or protected tag features.
- Physical layout documentation beyond module-to-module wiring.

## Fundamental Constraints

- iPhone custom hardware communication should use BLE/CoreBluetooth.
- ESP32 must remain independently testable without the iPhone app.
- YRM100 RF operation targets the US region, 902-928 MHz.
- YRM100 module power must support documented current peaks.
- Tag writes must be explicit, visible, and verified where possible.

## Architecture Flexibility

The ESP32 bridge remains the baseline. If future documentation or hardware testing proves that the purchased YRM100 exposes iPhone-compatible BLE directly, the architecture may be revisited. Current SDK evidence points to classic Bluetooth SPP in Android samples, which is not suitable as the primary iPhone path.
