# 04 - User Stories

## As a Builder / Developer

- I want to connect the YRM100 module to an ESP32 so that I can control the reader without using vendor Windows software.
- I want to validate UART communication with the reader so that I know the module is wired and powered correctly.
- I want the firmware to expose readable logs so that I can diagnose reader and BLE problems.
- I want the iPhone app to show BLE and RFID errors clearly so that hardware problems are not hidden behind generic failures.
- I want to inspect raw-ish development events during testing so that protocol behavior can be confirmed.

## As an Operator

- I want to open the iPhone app and connect to the RFID reader so that I can start scanning tags.
- I want to start and stop scanning from the app so that I control when RF inventory is active.
- I want to see scanned tags in a list so that I know which tags were detected.
- I want duplicate reads to be grouped or updated so that the scan result is easy to understand.
- I want to see signal strength when available so that I can understand distance or orientation effects.
- I want to save a tag read with a label, such as `PLA Blue TAG`, so that I can recognize it later.
- I want to open saved tag reads so that I can reuse known tag data.
- I want to write my own supported value to a writable tag so that I can program tags for experiments.
- I want to clone a saved tag read to another compatible tag so that I can duplicate a known tag value when the tag technology permits it.

## As an Asset Manager

- I want to associate a scanned EPC with an asset so that RFID tags can identify real items.
- I want to verify that a tag can be read consistently before attaching it permanently.
- I want to program or commission tags in a controlled workflow so that incorrect tag writes are avoided.
- I want labeled saved reads so that tags can be described in plain language instead of only by EPC values.

## Technical Stories

- As a developer, I want a documented BLE protocol between iPhone and ESP32 so that the app and firmware can evolve independently.
- As a developer, I want a documented YRM100 command mapping so that vendor SDK behavior is not treated as magic.
- As a developer, I want tests around protocol parsing so that firmware changes do not break known reader responses.
- As a developer, I want the SDK directory treated as untrusted so that vendor code does not become an accidental runtime dependency.
