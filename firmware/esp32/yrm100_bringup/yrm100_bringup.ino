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

#ifndef LED_BUILTIN
#define LED_BUILTIN -1
#endif

static constexpr int kYrmRxPin = YRM100_RX_PIN;  // ESP32 receives from YRM100 TXD.
static constexpr int kYrmTxPin = YRM100_TX_PIN;  // ESP32 transmits to YRM100 RXD.
static constexpr int kTxIndicatorPin = LED_BUILTIN;
static constexpr uint32_t kUsbSerialBaud = 115200;
static constexpr uint32_t kDefaultYrmBaud = YRM100_DEFAULT_BAUD;

HardwareSerial YrmSerial(1);
static uint32_t currentYrmBaud = kDefaultYrmBaud;

static const uint8_t CMD_GET_MODULE_INFO[] = {
  0xBB, 0x00, 0x03, 0x00, 0x00, 0x03, 0x7E
};

static const uint8_t CMD_SINGLE_INVENTORY[] = {
  0xBB, 0x00, 0x22, 0x00, 0x00, 0x22, 0x7E
};

static const uint8_t CMD_STOP_MULTIPLE_INVENTORY[] = {
  0xBB, 0x00, 0x28, 0x00, 0x00, 0x28, 0x7E
};

static uint8_t frame[256];
static size_t frameLen = 0;
static size_t expectedFrameLen = 0;
static uint32_t lastRxAtMs = 0;

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

static void sendCommand(const char *label, const uint8_t *command, size_t len) {
  Serial.print("[TX] ");
  Serial.print(label);
  Serial.print("  ");
  printBytes(command, len);
  Serial.println();

  YrmSerial.write(command, len);
  YrmSerial.flush();

  if (kTxIndicatorPin >= 0) {
    digitalWrite(kTxIndicatorPin, HIGH);
    delay(30);
    digitalWrite(kTxIndicatorPin, LOW);
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
  }
}

static void handleYrmByte(uint8_t value) {
  lastRxAtMs = millis();

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
  Serial.println("  g = send get module info command");
  Serial.println("  i = send single inventory command");
  Serial.println("  s = send stop multiple inventory command");
  Serial.println("  b = toggle YRM100 UART baud between 115200 and 38400");
  Serial.println("  h = print this help");
  Serial.println();
}

void setup() {
  if (kTxIndicatorPin >= 0) {
    pinMode(kTxIndicatorPin, OUTPUT);
    digitalWrite(kTxIndicatorPin, LOW);
  }

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
  Serial.print("TX indicator pin: ");
  if (kTxIndicatorPin >= 0) {
    Serial.println(kTxIndicatorPin);
  } else {
    Serial.println("none");
  }
  beginYrmSerial(currentYrmBaud);

  printHelp();
  delay(500);
  sendCommand("get module info", CMD_GET_MODULE_INFO, sizeof(CMD_GET_MODULE_INFO));
}

void loop() {
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
      case 'g':
      case 'G':
        sendCommand("get module info", CMD_GET_MODULE_INFO, sizeof(CMD_GET_MODULE_INFO));
        break;
      case 'i':
      case 'I':
        sendCommand("single inventory", CMD_SINGLE_INVENTORY, sizeof(CMD_SINGLE_INVENTORY));
        break;
      case 's':
      case 'S':
        sendCommand("stop multiple inventory", CMD_STOP_MULTIPLE_INVENTORY, sizeof(CMD_STOP_MULTIPLE_INVENTORY));
        break;
      case 'b':
      case 'B':
        beginYrmSerial(currentYrmBaud == 115200 ? 38400 : 115200);
        sendCommand("get module info", CMD_GET_MODULE_INFO, sizeof(CMD_GET_MODULE_INFO));
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
