# Protocol Tools

This directory will contain protocol helpers that are useful during development.

Ownership:

- protocol test vectors
- optional scripts for validating frame checksums
- optional scripts for inspecting captured UART/BLE payloads
- generated fixtures used by tests

Vendor SDK binaries and executable demos must not be copied here.

## Current Tools

| Path | Purpose |
|---|---|
| `scripts/run-yrm100-driver-tests.sh` | Builds and runs host-side YRM100 driver tests |
| `tests/test_yrm100_driver.cpp` | C++ protocol-driver test runner |
| `test-vectors/yrm100-frames.md` | Captured and SDK-audited YRM100 protocol fixtures |
