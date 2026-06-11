/*
  YRM100 UART bring-up sketch for ESP32-S3 SuperMini.

  Purpose:
  - Validate wiring and UART direction before BLE/app work starts.
  - Send safe diagnostic commands to the YRM100.
  - Dump and validate raw YRM100 frames on USB serial.

  Wiring for first bring-up:
  - YRM100 GND  -> ESP32 GND
  - YRM100 EN   -> ESP32 3V3
  - YRM100 RXD  -> ESP32 TX
  - YRM100 TXD  -> ESP32 RX
  - YRM100 VCC  -> ESP32 5V USB rail, or external regulated 5V with common GND
*/

#include <Arduino.h>

// ESP32-S3 SuperMini board-labeled TX/RX pins. Prefer the Arduino board
// package's RX/TX constants when available. Many ESP32-S3 SuperMini boards
// route edge-labeled TX/RX to GPIO43/GPIO44.
#ifndef YRM100_RX_PIN
#ifdef RX
#define YRM100_RX_PIN RX
#else
#define YRM100_RX_PIN 44
#endif
#endif

#ifndef YRM100_TX_PIN
#ifdef TX
#define YRM100_TX_PIN TX
#else
#define YRM100_TX_PIN 43
#endif
#endif

#ifndef YRM100_DEFAULT_BAUD
#define YRM100_DEFAULT_BAUD 115200
#endif

#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 48
#endif

#ifndef STATUS_LED_BRIGHTNESS
#define STATUS_LED_BRIGHTNESS 3
#endif

static constexpr int kYrmRxPin = YRM100_RX_PIN;  // ESP32 receives from YRM100 TXD.
static constexpr int kYrmTxPin = YRM100_TX_PIN;  // ESP32 transmits to YRM100 RXD.
static constexpr int kStatusLedPin = STATUS_LED_PIN;
static constexpr uint8_t kStatusLedBrightness = STATUS_LED_BRIGHTNESS;
static constexpr uint16_t kStatusLedTxMs = 40;
static constexpr uint16_t kStatusLedRxMs = 80;
static constexpr uint32_t kUsbSerialBaud = 115200;
static constexpr uint32_t kDefaultYrmBaud = YRM100_DEFAULT_BAUD;
static constexpr uint32_t kVisualTestBaud = 1200;
static constexpr uint32_t kStartupListenMs = 5000;
static constexpr uint32_t kRawCaptureMs = 5000;
static constexpr size_t kVisualTestBytes = 512;
static constexpr uint32_t kSupportedYrmBauds[] = {115200, 57600, 38400, 19200, 9600};

HardwareSerial YrmSerial(1);
static uint32_t currentYrmBaud = kDefaultYrmBaud;

static const uint8_t CMD_GET_HARDWARE_VERSION[] = {
  0xBB, 0x00, 0x03, 0x00, 0x01, 0x00, 0x04, 0x7E
};

static const uint8_t CMD_GET_SOFTWARE_VERSION[] = {
  0xBB, 0x00, 0x03, 0x00, 0x01, 0x01, 0x05, 0x7E
};

static const uint8_t CMD_GET_MANUFACTURER[] = {
  0xBB, 0x00, 0x03, 0x00, 0x01, 0x02, 0x06, 0x7E
};

static const uint8_t CMD_GET_REGION[] = {
  0xBB, 0x00, 0x08, 0x00, 0x00, 0x08, 0x7E
};

static const uint8_t CMD_GET_TX_POWER[] = {
  0xBB, 0x00, 0xB7, 0x00, 0x00, 0xB7, 0x7E
};

// Vendor quick-reference labels this as 18.5 dBm.
static const uint8_t CMD_SET_TX_POWER_LOW[] = {
  0xBB, 0x00, 0xB6, 0x00, 0x02, 0x04, 0xE2, 0x9E, 0x7E
};

static const uint8_t CMD_SINGLE_INVENTORY[] = {
  0xBB, 0x00, 0x22, 0x00, 0x00, 0x22, 0x7E
};

static const uint8_t CMD_MULTIPLE_INVENTORY[] = {
  0xBB, 0x00, 0x27, 0x00, 0x03, 0x22, 0xFF, 0xFF, 0x4A, 0x7E
};

static const uint8_t CMD_STOP_MULTIPLE_INVENTORY[] = {
  0xBB, 0x00, 0x28, 0x00, 0x00, 0x28, 0x7E
};

static uint8_t frame[256];
static size_t frameLen = 0;
static size_t expectedFrameLen = 0;
static uint32_t lastRxAtMs = 0;
static uint32_t statusLedOffAtMs = 0;
static bool rawCaptureEnabled = false;
static uint32_t rawCaptureUntilMs = 0;

static void resetFrameParser();
static void handleYrmByte(uint8_t value);

static void printByteHex(uint8_t value) {
  if (value < 0x10) {
    Serial.print('0');
  }
  Serial.print(value, HEX);
}

static void printBytes(const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    if (i > 0) {
      Serial.print(' ');
    }
    printByteHex(data[i]);
  }
}

static uint8_t yrmChecksum(const uint8_t *data, size_t len) {
  uint16_t sum = 0;
  for (size_t i = 1; i < len; i++) {
    sum += data[i];
  }
  return static_cast<uint8_t>(sum & 0xFF);
}

static void printAsciiPayload(const uint8_t *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    const uint8_t value = data[i];
    Serial.print(value >= 0x20 && value <= 0x7E ? static_cast<char>(value) : '.');
  }
}

static void setStatusLed(uint8_t red, uint8_t green, uint8_t blue) {
  if (kStatusLedPin < 0 || kStatusLedBrightness == 0) {
    return;
  }
  neopixelWrite(kStatusLedPin, red, green, blue);
}

static void blinkTxLed() {
  setStatusLed(kStatusLedBrightness, 0, 0);
}

static void finishTxLed() {
  statusLedOffAtMs = millis() + kStatusLedTxMs;
}

static void blinkRxLed() {
  setStatusLed(0, kStatusLedBrightness, 0);
  statusLedOffAtMs = millis() + kStatusLedRxMs;
}

static void updateStatusLed() {
  if (statusLedOffAtMs > 0 && static_cast<int32_t>(millis() - statusLedOffAtMs) >= 0) {
    setStatusLed(0, 0, 0);
    statusLedOffAtMs = 0;
  }
}

static void sendCommand(const char *label, const uint8_t *command, size_t len) {
  Serial.print("[TX] ");
  Serial.print(label);
  Serial.print("  ");
  printBytes(command, len);
  Serial.println();

  blinkTxLed();
  YrmSerial.write(command, len);
  YrmSerial.flush();
  finishTxLed();
}

static void sendRepeatedByte(const char *label, uint8_t value, size_t count) {
  Serial.print("[TX] ");
  Serial.print(label);
  Serial.print("  byte=0x");
  printByteHex(value);
  Serial.print(" count=");
  Serial.println(count);

  blinkTxLed();
  for (size_t i = 0; i < count; i++) {
    YrmSerial.write(value);
  }
  YrmSerial.flush();
  finishTxLed();
}

static void drainYrmFor(uint32_t durationMs) {
  const uint32_t untilMs = millis() + durationMs;
  while (static_cast<int32_t>(millis() - untilMs) < 0) {
    updateStatusLed();
    while (YrmSerial.available() > 0) {
      handleYrmByte(static_cast<uint8_t>(YrmSerial.read()));
    }
    if (frameLen > 0 && millis() - lastRxAtMs > 500) {
      Serial.print("[RX partial timeout] ");
      printBytes(frame, frameLen);
      Serial.println();
      resetFrameParser();
    }
    delay(1);
  }
}

static void resetFrameParser() {
  frameLen = 0;
  expectedFrameLen = 0;
}

static void beginYrmSerial(uint32_t baud) {
  currentYrmBaud = baud;
  YrmSerial.end();
  delay(50);
  YrmSerial.begin(currentYrmBaud, SERIAL_8N1, kYrmRxPin, kYrmTxPin);
  resetFrameParser();

  Serial.print("[UART] YRM100 baud set to ");
  Serial.println(currentYrmBaud);
}

static void printFrameSummary(const uint8_t *data, size_t len) {
  Serial.print("[RX FRAME] ");
  printBytes(data, len);
  Serial.println();

  if (len < 7) {
    Serial.println("[RX FRAME] invalid: too short");
    return;
  }

  const uint8_t type = data[1];
  const uint8_t command = data[2];
  const uint16_t payloadLen = (static_cast<uint16_t>(data[3]) << 8) | data[4];
  const size_t checksumIndex = 5 + payloadLen;
  const size_t endIndex = checksumIndex + 1;

  if (endIndex >= len) {
    Serial.println("[RX FRAME] invalid: length mismatch");
    return;
  }

  const uint8_t actualChecksum = data[checksumIndex];
  const uint8_t expectedChecksum = yrmChecksum(data, checksumIndex);

  Serial.print("[RX FRAME] type=0x");
  printByteHex(type);
  Serial.print(" command=0x");
  printByteHex(command);
  Serial.print(" payload_len=");
  Serial.print(payloadLen);
  Serial.print(" checksum=");
  Serial.print(actualChecksum == expectedChecksum ? "OK" : "BAD");
  Serial.print(" end=");
  Serial.println(data[endIndex] == 0x7E ? "OK" : "BAD");

  if (payloadLen > 0) {
    Serial.print("[RX PAYLOAD] ");
    printBytes(data + 5, payloadLen);
    Serial.println();

    if (type == 0x01 && command == 0x03 && payloadLen > 1) {
      Serial.print("[MODULE INFO] selector=0x");
      printByteHex(data[5]);
      Serial.print(" text=\"");
      printAsciiPayload(data + 6, payloadLen - 1);
      Serial.println("\"");
    }
  }
}

static void handleYrmByte(uint8_t value) {
  lastRxAtMs = millis();
  blinkRxLed();

  if (rawCaptureEnabled) {
    printByteHex(value);
    Serial.print(' ');
    return;
  }

  if (frameLen == 0) {
    if (value != 0xBB) {
      Serial.print("[RX stray] ");
      printByteHex(value);
      Serial.println();
      return;
    }
    frame[frameLen++] = value;
    return;
  }

  if (frameLen >= sizeof(frame)) {
    Serial.println("[RX] parser reset: frame buffer overflow");
    resetFrameParser();
    return;
  }

  frame[frameLen++] = value;

  if (frameLen == 5) {
    const uint16_t payloadLen = (static_cast<uint16_t>(frame[3]) << 8) | frame[4];
    expectedFrameLen = 7 + payloadLen;
    if (expectedFrameLen > sizeof(frame)) {
      Serial.print("[RX] parser reset: frame too large, payload_len=");
      Serial.println(payloadLen);
      resetFrameParser();
    }
    return;
  }

  if (expectedFrameLen > 0 && frameLen == expectedFrameLen) {
    printFrameSummary(frame, frameLen);
    resetFrameParser();
  }
}

static void printHelp() {
  Serial.println();
  Serial.println("YRM100 UART bring-up commands:");
  Serial.println("  l = listen only; send nothing");
  Serial.println("  x = raw byte capture for 5 seconds; send nothing");
  Serial.println("  g = send hardware/software/manufacturer version commands");
  Serial.println("  r = send get region and get TX power commands");
  Serial.println("  t = set low TX power, then get TX power");
  Serial.println("  i = send single inventory command");
  Serial.println("  m = start multiple inventory command");
  Serial.println("  s = stop multiple inventory command");
  Serial.println("  b = cycle YRM100 UART baud through SDK/demo supported rates");
  Serial.println("  p = probe all SDK/demo supported baud rates");
  Serial.println("  v = visual TX test: set 1200 baud and send long 0x55 pattern");
  Serial.println("  h = print this help");
  Serial.println();
}

void setup() {
  Serial.begin(kUsbSerialBaud);
  const uint32_t waitStart = millis();
  while (!Serial && millis() - waitStart < 3000) {
    delay(10);
  }

  Serial.println();
  Serial.println("RFID Tools - YRM100 UART bring-up");
  Serial.print("ESP32 RX pin: GPIO");
  Serial.println(kYrmRxPin);
  Serial.print("ESP32 TX pin: GPIO");
  Serial.println(kYrmTxPin);
  Serial.print("Status LED pin: GPIO");
  Serial.println(kStatusLedPin);
  Serial.print("Status LED brightness: ");
  Serial.println(kStatusLedBrightness);
  setStatusLed(0, 0, 0);
  beginYrmSerial(currentYrmBaud);

  printHelp();
  Serial.print("[LISTEN] passive startup window, ms=");
  Serial.println(kStartupListenMs);
  const uint32_t listenUntilMs = millis() + kStartupListenMs;
  while (static_cast<int32_t>(millis() - listenUntilMs) < 0) {
    updateStatusLed();
    while (YrmSerial.available() > 0) {
      handleYrmByte(static_cast<uint8_t>(YrmSerial.read()));
    }
    if (frameLen > 0 && millis() - lastRxAtMs > 500) {
      Serial.print("[RX partial timeout] ");
      printBytes(frame, frameLen);
      Serial.println();
      resetFrameParser();
    }
    delay(1);
  }
  sendCommand("get hardware version", CMD_GET_HARDWARE_VERSION, sizeof(CMD_GET_HARDWARE_VERSION));
  drainYrmFor(250);
  sendCommand("get region", CMD_GET_REGION, sizeof(CMD_GET_REGION));
  drainYrmFor(250);
  sendCommand("get TX power", CMD_GET_TX_POWER, sizeof(CMD_GET_TX_POWER));
}

void loop() {
  updateStatusLed();

  if (rawCaptureEnabled && static_cast<int32_t>(millis() - rawCaptureUntilMs) >= 0) {
    rawCaptureEnabled = false;
    Serial.println();
    Serial.println("[RAW] capture ended");
  }

  while (YrmSerial.available() > 0) {
    handleYrmByte(static_cast<uint8_t>(YrmSerial.read()));
  }

  if (frameLen > 0 && millis() - lastRxAtMs > 500) {
    Serial.print("[RX partial timeout] ");
    printBytes(frame, frameLen);
    Serial.println();
    resetFrameParser();
  }

  while (Serial.available() > 0) {
    const char input = static_cast<char>(Serial.read());
    switch (input) {
      case 'l':
      case 'L':
        Serial.println("[LISTEN] passive mode; no command sent");
        break;
      case 'x':
      case 'X':
        resetFrameParser();
        rawCaptureEnabled = true;
        rawCaptureUntilMs = millis() + kRawCaptureMs;
        Serial.print("[RAW] capture started, ms=");
        Serial.println(kRawCaptureMs);
        break;
      case 'g':
      case 'G':
        sendCommand("get hardware version", CMD_GET_HARDWARE_VERSION, sizeof(CMD_GET_HARDWARE_VERSION));
        delay(100);
        sendCommand("get software version", CMD_GET_SOFTWARE_VERSION, sizeof(CMD_GET_SOFTWARE_VERSION));
        delay(100);
        sendCommand("get manufacturer", CMD_GET_MANUFACTURER, sizeof(CMD_GET_MANUFACTURER));
        break;
      case 'r':
      case 'R':
        sendCommand("get region", CMD_GET_REGION, sizeof(CMD_GET_REGION));
        delay(100);
        sendCommand("get TX power", CMD_GET_TX_POWER, sizeof(CMD_GET_TX_POWER));
        break;
      case 't':
      case 'T':
        sendCommand("set low TX power", CMD_SET_TX_POWER_LOW, sizeof(CMD_SET_TX_POWER_LOW));
        delay(100);
        sendCommand("get TX power", CMD_GET_TX_POWER, sizeof(CMD_GET_TX_POWER));
        break;
      case 'i':
      case 'I':
        sendCommand("single inventory", CMD_SINGLE_INVENTORY, sizeof(CMD_SINGLE_INVENTORY));
        break;
      case 'm':
      case 'M':
        sendCommand("start multiple inventory", CMD_MULTIPLE_INVENTORY, sizeof(CMD_MULTIPLE_INVENTORY));
        break;
      case 's':
      case 'S':
        sendCommand("stop multiple inventory", CMD_STOP_MULTIPLE_INVENTORY, sizeof(CMD_STOP_MULTIPLE_INVENTORY));
        break;
      case 'b':
      case 'B':
        for (size_t i = 0; i < sizeof(kSupportedYrmBauds) / sizeof(kSupportedYrmBauds[0]); i++) {
          if (currentYrmBaud == kSupportedYrmBauds[i]) {
            const size_t nextIndex = (i + 1) % (sizeof(kSupportedYrmBauds) / sizeof(kSupportedYrmBauds[0]));
            beginYrmSerial(kSupportedYrmBauds[nextIndex]);
            break;
          }
          if (i == (sizeof(kSupportedYrmBauds) / sizeof(kSupportedYrmBauds[0])) - 1) {
            beginYrmSerial(kSupportedYrmBauds[0]);
          }
        }
        sendCommand("get hardware version", CMD_GET_HARDWARE_VERSION, sizeof(CMD_GET_HARDWARE_VERSION));
        break;
      case 'p':
      case 'P':
        Serial.println("[PROBE] probing SDK/demo baud rates with get hardware version");
        for (size_t i = 0; i < sizeof(kSupportedYrmBauds) / sizeof(kSupportedYrmBauds[0]); i++) {
          beginYrmSerial(kSupportedYrmBauds[i]);
          sendCommand("get hardware version", CMD_GET_HARDWARE_VERSION, sizeof(CMD_GET_HARDWARE_VERSION));
          drainYrmFor(1000);
        }
        Serial.println("[PROBE] ended");
        break;
      case 'v':
      case 'V':
        beginYrmSerial(kVisualTestBaud);
        sendRepeatedByte("visual TX test pattern", 0x55, kVisualTestBytes);
        break;
      case 'h':
      case 'H':
      case '?':
        printHelp();
        break;
      case '\r':
      case '\n':
        break;
      default:
        Serial.print("[USB] unknown command: ");
        Serial.println(input);
        printHelp();
        break;
    }
  }
}
