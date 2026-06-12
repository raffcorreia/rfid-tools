/*
  RFID Tools ESP32 BLE service sketch.

  PHASE-003 first pass:
  - advertise a stable custom RFID service
  - accept high-level JSON commands over BLE
  - emit JSON events and expose status
  - route inventory/config commands through the shared YRM100 UART driver
*/

#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <esp_system.h>

#include "../yrm100_driver/Yrm100Driver.h"

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

static constexpr const char *kDeviceName = "RFID Tools ESP32";
static constexpr const char *kFirmwareVersion = "0.1.6";
static constexpr const char *kServiceUuid = "63802432-69FC-406D-A538-FE33CEF32AEF";
static constexpr const char *kCommandUuid = "DE0D7201-CC2B-46D9-8F92-564A209C37EF";
static constexpr const char *kEventsUuid = "456F5CDA-632A-4541-A2A5-6FAEC234075E";
static constexpr const char *kStatusUuid = "0AF05FBF-ADAA-4D94-8DCF-44A62F82332B";
static constexpr int kYrmRxPin = YRM100_RX_PIN;
static constexpr int kYrmTxPin = YRM100_TX_PIN;
static constexpr uint32_t kYrmBaud = YRM100_DEFAULT_BAUD;
static constexpr uint32_t kCommandTimeoutMs = 1500;
static constexpr uint32_t kInventoryCooldownMs = 150;
static constexpr uint32_t kLateDuplicateWindowMs = 750;
static constexpr int kMinPowerDbm = 0;
static constexpr int kMaxPowerDbm = 26;
static constexpr uint8_t kRegionUs = 0x02;
static constexpr size_t kMaxBleCommandLength = 256;
static constexpr size_t kBleCommandQueueDepth = 4;
static constexpr size_t kMaxBleEventLength = 384;
static constexpr size_t kBleEventQueueDepth = 10;
static constexpr uint32_t kBleNotifySpacingMs = 30;
static constexpr size_t kMaxYrmBytesPerLoop = 128;

static BLEServer *bleServer = nullptr;
static BLECharacteristic *eventsCharacteristic = nullptr;
static BLECharacteristic *statusCharacteristic = nullptr;
static HardwareSerial YrmSerial(1);
static rfid::yrm100::FrameParser yrmParser;

static bool bleConnected = false;
static bool scanActive = false;
static bool readerReady = false;
static int currentPowerCentiDbm = -1;
static int currentRegion = -1;
static uint32_t lastInventoryEndMs = 0;
static uint32_t lastNotifiedInventoryMs = 0;
static std::vector<uint8_t> lastInventoryEpc;
static std::vector<uint8_t> lastNotifiedInventoryEpc;

struct QueuedBleCommand {
  bool occupied = false;
  size_t length = 0;
  char text[kMaxBleCommandLength + 1] = {};
};

struct QueuedBleEvent {
  bool occupied = false;
  size_t length = 0;
  char text[kMaxBleEventLength + 1] = {};
};

static portMUX_TYPE bleQueueMux = portMUX_INITIALIZER_UNLOCKED;
static QueuedBleCommand bleCommandQueue[kBleCommandQueueDepth];
static size_t bleCommandHead = 0;
static size_t bleCommandTail = 0;
static size_t bleCommandCount = 0;
static QueuedBleEvent bleEventQueue[kBleEventQueueDepth];
static size_t bleEventHead = 0;
static size_t bleEventTail = 0;
static size_t bleEventCount = 0;
static uint32_t lastBleNotifyMs = 0;
static volatile bool bleCommandOverflow = false;
static volatile bool bleCommandTooLong = false;
static volatile bool bleEventOverflow = false;
static volatile bool bleEventTooLong = false;
static volatile bool bleConnectPending = false;
static volatile bool bleDisconnectPending = false;
static volatile bool bleRestartAdvertisingPending = false;
static volatile bool statusUpdatePending = false;

static const char *resetReasonName(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON:
      return "power_on";
    case ESP_RST_EXT:
      return "external";
    case ESP_RST_SW:
      return "software";
    case ESP_RST_PANIC:
      return "panic";
    case ESP_RST_INT_WDT:
      return "interrupt_watchdog";
    case ESP_RST_TASK_WDT:
      return "task_watchdog";
    case ESP_RST_WDT:
      return "other_watchdog";
    case ESP_RST_DEEPSLEEP:
      return "deep_sleep";
    case ESP_RST_BROWNOUT:
      return "brownout";
    case ESP_RST_SDIO:
      return "sdio";
    case ESP_RST_UNKNOWN:
    default:
      return "unknown";
  }
}

enum class AppCommand {
  GetInfo,
  Status,
  StartInventory,
  StopInventory,
  GetPower,
  SetPower,
  GetRegion,
  SetRegion,
  WriteEpc,
  Unknown,
};

enum class PendingAction {
  None,
  StartInventory,
  StopInventory,
  GetPower,
  SetPower,
  GetRegion,
  SetRegion,
  WriteSelectMode,
  WriteSelectTag,
  WriteEpc,
  WriteClearSelectMode,
};

struct PendingCommand {
  PendingAction action = PendingAction::None;
  int id = 0;
  uint8_t yrmCommand = 0;
  uint32_t sentAtMs = 0;
};

static PendingCommand pending;

struct WriteContext {
  bool active = false;
  int id = 0;
  std::vector<uint8_t> targetEpc;
  std::vector<uint8_t> newEpc;
};

static WriteContext writeContext;

static const char *pendingActionName(PendingAction action);
static AppCommand parseCommand(const String &message);
static bool isWriteAction(PendingAction action);
static bool sendYrmCommand(int id, PendingAction action, uint8_t yrmCommand, const std::vector<uint8_t> &frame);
static bool sendWriteStep(PendingAction action, uint8_t yrmCommand, const std::vector<uint8_t> &frame);

static bool enqueueBleCommand(const char *value, size_t length) {
  if (length == 0) {
    return true;
  }

  if (length > kMaxBleCommandLength) {
    bleCommandTooLong = true;
    return false;
  }

  portENTER_CRITICAL(&bleQueueMux);
  if (bleCommandCount >= kBleCommandQueueDepth) {
    bleCommandOverflow = true;
    portEXIT_CRITICAL(&bleQueueMux);
    return false;
  }

  QueuedBleCommand &slot = bleCommandQueue[bleCommandTail];
  memcpy(slot.text, value, length);
  slot.text[length] = '\0';
  slot.length = length;
  slot.occupied = true;
  bleCommandTail = (bleCommandTail + 1) % kBleCommandQueueDepth;
  bleCommandCount++;
  portEXIT_CRITICAL(&bleQueueMux);
  return true;
}

static bool dequeueBleCommand(String &message) {
  char text[kMaxBleCommandLength + 1] = {};
  bool hasCommand = false;

  portENTER_CRITICAL(&bleQueueMux);
  if (bleCommandCount > 0) {
    QueuedBleCommand &slot = bleCommandQueue[bleCommandHead];
    const size_t length = slot.length < sizeof(text) - 1 ? slot.length : sizeof(text) - 1;
    memcpy(text, slot.text, length);
    text[length] = '\0';
    slot.text[0] = '\0';
    slot.length = 0;
    slot.occupied = false;
    bleCommandHead = (bleCommandHead + 1) % kBleCommandQueueDepth;
    bleCommandCount--;
    hasCommand = true;
  }
  portEXIT_CRITICAL(&bleQueueMux);

  if (hasCommand) {
    message = text;
  }
  return hasCommand;
}

static void clearBleCommandQueue() {
  portENTER_CRITICAL(&bleQueueMux);
  for (QueuedBleCommand &slot : bleCommandQueue) {
    slot.text[0] = '\0';
    slot.length = 0;
    slot.occupied = false;
  }
  bleCommandHead = 0;
  bleCommandTail = 0;
  bleCommandCount = 0;
  portEXIT_CRITICAL(&bleQueueMux);
}

static bool enqueueBleEvent(const char *value, size_t length) {
  if (length == 0) {
    return true;
  }

  if (length > kMaxBleEventLength) {
    bleEventTooLong = true;
    return false;
  }

  portENTER_CRITICAL(&bleQueueMux);
  if (bleEventCount >= kBleEventQueueDepth) {
    bleEventOverflow = true;
    portEXIT_CRITICAL(&bleQueueMux);
    return false;
  }

  QueuedBleEvent &slot = bleEventQueue[bleEventTail];
  memcpy(slot.text, value, length);
  slot.text[length] = '\0';
  slot.length = length;
  slot.occupied = true;
  bleEventTail = (bleEventTail + 1) % kBleEventQueueDepth;
  bleEventCount++;
  portEXIT_CRITICAL(&bleQueueMux);
  return true;
}

static bool dequeueBleEvent(char *text, size_t textSize) {
  bool hasEvent = false;

  if (textSize == 0) {
    return false;
  }
  text[0] = '\0';

  portENTER_CRITICAL(&bleQueueMux);
  if (bleEventCount > 0) {
    QueuedBleEvent &slot = bleEventQueue[bleEventHead];
    const size_t length = slot.length < textSize - 1 ? slot.length : textSize - 1;
    memcpy(text, slot.text, length);
    text[length] = '\0';
    slot.text[0] = '\0';
    slot.length = 0;
    slot.occupied = false;
    bleEventHead = (bleEventHead + 1) % kBleEventQueueDepth;
    bleEventCount--;
    hasEvent = true;
  }
  portEXIT_CRITICAL(&bleQueueMux);

  return hasEvent;
}

static void clearBleEventQueue() {
  portENTER_CRITICAL(&bleQueueMux);
  for (QueuedBleEvent &slot : bleEventQueue) {
    slot.text[0] = '\0';
    slot.length = 0;
    slot.occupied = false;
  }
  bleEventHead = 0;
  bleEventTail = 0;
  bleEventCount = 0;
  portEXIT_CRITICAL(&bleQueueMux);
}

static const char *pendingActionName(PendingAction action) {
  switch (action) {
    case PendingAction::None:
      return "none";
    case PendingAction::StartInventory:
      return "startInventory";
    case PendingAction::StopInventory:
      return "stopInventory";
    case PendingAction::GetPower:
      return "getPower";
    case PendingAction::SetPower:
      return "setPower";
    case PendingAction::GetRegion:
      return "getRegion";
    case PendingAction::SetRegion:
      return "setRegion";
    case PendingAction::WriteSelectMode:
      return "writeSelectMode";
    case PendingAction::WriteSelectTag:
      return "writeSelectTag";
    case PendingAction::WriteEpc:
      return "writeEpc";
    case PendingAction::WriteClearSelectMode:
      return "writeClearSelectMode";
  }
  return "unknown";
}

static String bytesToHex(const std::vector<uint8_t> &bytes) {
  static constexpr char kHex[] = "0123456789ABCDEF";
  String out;
  out.reserve(bytes.size() * 2);
  for (uint8_t value : bytes) {
    out += kHex[(value >> 4) & 0x0F];
    out += kHex[value & 0x0F];
  }
  return out;
}

static int hexValue(char value) {
  if (value >= '0' && value <= '9') {
    return value - '0';
  }
  if (value >= 'a' && value <= 'f') {
    return value - 'a' + 10;
  }
  if (value >= 'A' && value <= 'F') {
    return value - 'A' + 10;
  }
  return -1;
}

static bool parseHexBytes(const String &text, std::vector<uint8_t> &bytes) {
  String cleaned;
  cleaned.reserve(text.length());
  for (size_t i = 0; i < text.length(); i++) {
    const char value = text[i];
    if (!isspace(value) && value != ':' && value != '-') {
      cleaned += value;
    }
  }

  if (cleaned.length() == 0 || (cleaned.length() % 2) != 0) {
    return false;
  }

  bytes.clear();
  bytes.reserve(cleaned.length() / 2);
  for (size_t i = 0; i < cleaned.length(); i += 2) {
    const int high = hexValue(cleaned[i]);
    const int low = hexValue(cleaned[i + 1]);
    if (high < 0 || low < 0) {
      bytes.clear();
      return false;
    }
    bytes.push_back(static_cast<uint8_t>((high << 4) | low));
  }

  return true;
}

static int parseId(const String &message) {
  const int keyIndex = message.indexOf("\"id\"");
  if (keyIndex < 0) {
    return 0;
  }

  const int colonIndex = message.indexOf(':', keyIndex);
  if (colonIndex < 0) {
    return 0;
  }

  int valueStart = colonIndex + 1;
  while (valueStart < static_cast<int>(message.length()) && isspace(message[valueStart])) {
    valueStart++;
  }

  bool negative = false;
  if (valueStart < static_cast<int>(message.length()) && message[valueStart] == '-') {
    negative = true;
    valueStart++;
  }

  int value = 0;
  bool foundDigit = false;
  while (valueStart < static_cast<int>(message.length()) && isdigit(message[valueStart])) {
    foundDigit = true;
    value = (value * 10) + (message[valueStart] - '0');
    valueStart++;
  }

  if (!foundDigit) {
    return 0;
  }
  return negative ? -value : value;
}

static bool parseNumberField(const String &message, const char *field, int &value) {
  String key = "\"";
  key += field;
  key += "\"";

  const int keyIndex = message.indexOf(key);
  if (keyIndex < 0) {
    return false;
  }

  const int colonIndex = message.indexOf(':', keyIndex + key.length());
  if (colonIndex < 0) {
    return false;
  }

  int valueStart = colonIndex + 1;
  while (valueStart < static_cast<int>(message.length()) && isspace(message[valueStart])) {
    valueStart++;
  }

  bool negative = false;
  if (valueStart < static_cast<int>(message.length()) && message[valueStart] == '-') {
    negative = true;
    valueStart++;
  }

  int parsed = 0;
  bool foundDigit = false;
  while (valueStart < static_cast<int>(message.length()) && isdigit(message[valueStart])) {
    foundDigit = true;
    parsed = (parsed * 10) + (message[valueStart] - '0');
    valueStart++;
  }

  if (!foundDigit) {
    return false;
  }
  value = negative ? -parsed : parsed;
  return true;
}

static bool parseStringField(const String &message, const char *field, String &value) {
  String key = "\"";
  key += field;
  key += "\"";

  const int keyIndex = message.indexOf(key);
  if (keyIndex < 0) {
    return false;
  }

  const int colonIndex = message.indexOf(':', keyIndex + key.length());
  if (colonIndex < 0) {
    return false;
  }

  int quoteStart = colonIndex + 1;
  while (quoteStart < static_cast<int>(message.length()) && isspace(message[quoteStart])) {
    quoteStart++;
  }
  if (quoteStart >= static_cast<int>(message.length()) || message[quoteStart] != '"') {
    return false;
  }

  const int valueStart = quoteStart + 1;
  const int valueEnd = message.indexOf('"', valueStart);
  if (valueEnd < 0) {
    return false;
  }

  value = message.substring(valueStart, valueEnd);
  return true;
}

static AppCommand parseCommand(const String &message) {
  String command;
  if (!parseStringField(message, "cmd", command)) {
    return AppCommand::Unknown;
  }

  if (command == "getInfo") {
    return AppCommand::GetInfo;
  }
  if (command == "status") {
    return AppCommand::Status;
  }
  if (command == "startInventory") {
    return AppCommand::StartInventory;
  }
  if (command == "stopInventory") {
    return AppCommand::StopInventory;
  }
  if (command == "getPower") {
    return AppCommand::GetPower;
  }
  if (command == "setPower") {
    return AppCommand::SetPower;
  }
  if (command == "getRegion") {
    return AppCommand::GetRegion;
  }
  if (command == "setRegion") {
    return AppCommand::SetRegion;
  }
  if (command == "writeEpc") {
    return AppCommand::WriteEpc;
  }
  return AppCommand::Unknown;
}

static bool parseRegionValue(const String &message, uint8_t &region) {
  String text;
  if (parseStringField(message, "region", text)) {
    text.toUpperCase();
    if (text == "US" || text == "USA" || text == "FCC") {
      region = kRegionUs;
      return true;
    }
    return false;
  }

  int numericRegion = 0;
  if (!parseNumberField(message, "region", numericRegion) || numericRegion < 0 || numericRegion > 255) {
    return false;
  }

  region = static_cast<uint8_t>(numericRegion);
  return true;
}

static bool parseEpcBytes(const String &message, std::vector<uint8_t> &epc) {
  String value;
  if (!parseStringField(message, "epc", value)) {
    return false;
  }
  return parseHexBytes(value, epc);
}

static bool parsePowerCentiDbm(const String &message, int16_t &centiDbm) {
  int dbm = 0;
  if (!parseNumberField(message, "dbm", dbm)) {
    return false;
  }

  if (dbm < kMinPowerDbm || dbm > kMaxPowerDbm) {
    return false;
  }

  centiDbm = static_cast<int16_t>(dbm * 100);
  return true;
}

static String statusJson(int id = -1) {
  String state = scanActive ? "scanning" : "idle";
  String reader = readerReady ? "ready" : "not_ready";
  String connected = bleConnected ? "true" : "false";
  String regionValue = currentRegion >= 0 ? String(currentRegion) : "null";
  String powerValue = currentPowerCentiDbm >= 0 ? String(currentPowerCentiDbm / 100.0, 2) : "null";
  String idValue = id >= 0 ? String(id) : "null";

  return String("{\"v\":1,\"id\":") + idValue +
         ",\"evt\":\"status\",\"state\":\"" + state +
         "\",\"reader\":\"" + reader +
         "\",\"bleConnected\":" + connected +
         ",\"region\":" + regionValue +
         ",\"powerDbm\":" + powerValue +
         ",\"fw\":\"" + kFirmwareVersion +
         "\",\"caps\":[\"inventory\",\"power\",\"region\",\"writeEpc\"]}";
}

static void updateStatusCharacteristic() {
  if (statusCharacteristic == nullptr) {
    return;
  }
  statusCharacteristic->setValue(statusJson().c_str());
}

static void notifyEvent(const String &json) {
  Serial.print("[BLE EVT] ");
  Serial.println(json);

  if (eventsCharacteristic == nullptr || !bleConnected) {
    return;
  }
  enqueueBleEvent(json.c_str(), json.length());
}

static void notifyError(int id, const char *code, const char *source, const char *message) {
  String json = String("{\"v\":1,\"id\":") + id +
                ",\"evt\":\"error\",\"code\":\"" + code +
                "\",\"source\":\"" + source +
                "\",\"message\":\"" + message + "\"}";
  notifyEvent(json);
}

static void clearPending() {
  pending = PendingCommand{};
}

static void clearWriteContext() {
  writeContext = WriteContext{};
}

static bool hasPending() {
  return pending.action != PendingAction::None;
}

static bool isWriteAction(PendingAction action) {
  return action == PendingAction::WriteSelectMode ||
         action == PendingAction::WriteSelectTag ||
         action == PendingAction::WriteEpc ||
         action == PendingAction::WriteClearSelectMode;
}

static bool isRecentDuplicateInventory(const std::vector<uint8_t> &epc) {
  return !lastNotifiedInventoryEpc.empty() &&
         epc == lastNotifiedInventoryEpc &&
         millis() - lastNotifiedInventoryMs < kLateDuplicateWindowMs;
}

static bool sendYrmCommand(int id, PendingAction action, uint8_t yrmCommand, const std::vector<uint8_t> &frame) {
  if (hasPending()) {
    notifyError(id, "busy", "firmware", "Another reader command is in progress");
    return false;
  }

  if (frame.empty()) {
    notifyError(id, "invalid_command", "firmware", "Failed to build YRM100 command frame");
    return false;
  }

  Serial.print("[YRM TX] ");
  Serial.print(pendingActionName(action));
  Serial.print(" ");
  Serial.println(bytesToHex(frame));

  YrmSerial.write(frame.data(), frame.size());
  YrmSerial.flush();

  pending.action = action;
  pending.id = id;
  pending.yrmCommand = yrmCommand;
  pending.sentAtMs = millis();
  return true;
}

static bool sendWriteStep(PendingAction action, uint8_t yrmCommand, const std::vector<uint8_t> &frame) {
  return sendYrmCommand(writeContext.id, action, yrmCommand, frame);
}

static bool beginWriteEpc(int id, const std::vector<uint8_t> &newEpc) {
  if (hasPending()) {
    notifyError(id, "busy", "firmware", "Another reader command is in progress");
    return false;
  }

  if (scanActive) {
    notifyError(id, "scan_active", "firmware", "Stop reading before writing a tag");
    return false;
  }

  if (lastInventoryEpc.empty()) {
    notifyError(id, "no_target_tag", "firmware", "Read the target tag before writing");
    return false;
  }

  if (newEpc.empty() || (newEpc.size() % 2) != 0 || newEpc.size() > 64) {
    notifyError(id, "invalid_argument", "ble", "writeEpc requires an even-length EPC hex value up to 64 bytes");
    return false;
  }

  if (newEpc.size() != lastInventoryEpc.size()) {
    notifyError(id, "length_mismatch", "firmware", "New EPC must match the target tag EPC byte length");
    return false;
  }

  writeContext.active = true;
  writeContext.id = id;
  writeContext.targetEpc = lastInventoryEpc;
  writeContext.newEpc = newEpc;

  return sendWriteStep(
      PendingAction::WriteSelectMode,
      static_cast<uint8_t>(rfid::yrm100::Command::SetSelectMode),
      rfid::yrm100::buildSetSelectMode(rfid::yrm100::SelectMode::BeforeTagOperations));
}

static bool sendStopInventoryCommand(int id) {
  if (hasPending() && pending.action != PendingAction::StartInventory) {
    notifyError(id, "busy", "firmware", "Another reader command is in progress");
    return false;
  }

  const auto frame = rfid::yrm100::buildStopMultipleInventory();
  Serial.print("[YRM TX] stopInventory ");
  Serial.println(bytesToHex(frame));
  YrmSerial.write(frame.data(), frame.size());
  YrmSerial.flush();

  pending.action = PendingAction::StopInventory;
  pending.id = id;
  pending.yrmCommand = static_cast<uint8_t>(rfid::yrm100::Command::StopMultipleInventory);
  pending.sentAtMs = millis();
  return true;
}

static void stopInventoryForDisconnect() {
  if (!scanActive && pending.action != PendingAction::StartInventory) {
    clearPending();
    return;
  }

  const auto frame = rfid::yrm100::buildStopMultipleInventory();
  Serial.println("[YRM TX] stop inventory on disconnect");
  YrmSerial.write(frame.data(), frame.size());
  YrmSerial.flush();
  scanActive = false;
  lastInventoryEndMs = millis();
  clearPending();
  clearWriteContext();
}

static void handleYrmFrame(const rfid::yrm100::Frame &frame) {
  readerReady = true;

  rfid::yrm100::InventoryTag tag;
  if (rfid::yrm100::decodeInventoryTag(frame, tag)) {
    if (!scanActive && pending.action != PendingAction::StartInventory) {
      if (isRecentDuplicateInventory(tag.epc)) {
        Serial.print("[YRM RX] ignored duplicate inventory tag ");
        Serial.println(bytesToHex(tag.epc));
        return;
      }
      Serial.print("[YRM RX] unsolicited inventory tag ");
      Serial.println(bytesToHex(tag.epc));
    }

    lastInventoryEpc = tag.epc;
    lastNotifiedInventoryEpc = tag.epc;
    lastNotifiedInventoryMs = millis();
    notifyEvent(String("{\"v\":1,\"id\":null,\"evt\":\"tagSeen\",\"epc\":\"") +
                bytesToHex(tag.epc) +
                "\",\"pc\":\"" + String(tag.pc, HEX) +
                "\",\"rssi\":" + String(tag.rssiDbm) +
                ",\"crc\":\"" + String(tag.crc, HEX) + "\"}");
    if (pending.action == PendingAction::StartInventory) {
      notifyEvent(String("{\"v\":1,\"id\":") + pending.id + ",\"evt\":\"scanStopped\",\"reason\":\"single_read\"}");
      scanActive = false;
      lastInventoryEndMs = millis();
      clearPending();
      updateStatusCharacteristic();
    }
    return;
  }

  rfid::yrm100::ErrorResponse error;
  if (rfid::yrm100::decodeErrorResponse(frame, error)) {
    notifyError(pending.id, "reader_error", "yrm100", (String("YRM100 error 0x") + String(error.code, HEX)).c_str());
    if (pending.action == PendingAction::StartInventory || pending.action == PendingAction::StopInventory) {
      scanActive = false;
      lastInventoryEndMs = millis();
    }
    if (isWriteAction(pending.action)) {
      clearWriteContext();
    }
    clearPending();
    updateStatusCharacteristic();
    return;
  }

  if (!hasPending()) {
    Serial.print("[YRM RX] unsolicited frame command=0x");
    Serial.println(frame.command, HEX);
    return;
  }

  if (frame.command != pending.yrmCommand) {
    Serial.print("[YRM RX] command mismatch pending=0x");
    Serial.print(pending.yrmCommand, HEX);
    Serial.print(" got=0x");
    Serial.println(frame.command, HEX);
    return;
  }

  rfid::yrm100::CommandStatus status;
  uint8_t region = 0;
  uint16_t centiDbm = 0;

  switch (pending.action) {
    case PendingAction::StartInventory:
      if (rfid::yrm100::decodeCommandStatus(frame, pending.yrmCommand, status) && status.status == 0x00) {
        scanActive = true;
        notifyEvent(String("{\"v\":1,\"id\":") + pending.id + ",\"evt\":\"scanStarted\"}");
        clearPending();
      }
      break;
    case PendingAction::StopInventory:
      if (rfid::yrm100::decodeCommandStatus(frame, pending.yrmCommand, status) && status.status == 0x00) {
        scanActive = false;
        lastInventoryEndMs = millis();
        notifyEvent(String("{\"v\":1,\"id\":") + pending.id + ",\"evt\":\"scanStopped\",\"reason\":\"requested\"}");
        clearPending();
      }
      break;
    case PendingAction::GetPower:
      if (rfid::yrm100::decodeTxPowerCentiDbm(frame, centiDbm)) {
        currentPowerCentiDbm = centiDbm;
        notifyEvent(String("{\"v\":1,\"id\":") + pending.id +
                    ",\"evt\":\"result\",\"cmd\":\"getPower\",\"powerDbm\":" + String(centiDbm / 100.0, 2) + "}");
        clearPending();
      }
      break;
    case PendingAction::SetPower:
      if (rfid::yrm100::decodeCommandStatus(frame, pending.yrmCommand, status) && status.status == 0x00) {
        notifyEvent(String("{\"v\":1,\"id\":") + pending.id + ",\"evt\":\"result\",\"cmd\":\"setPower\",\"ok\":true}");
        clearPending();
      }
      break;
    case PendingAction::GetRegion:
      if (rfid::yrm100::decodeRegion(frame, region)) {
        currentRegion = region;
        notifyEvent(String("{\"v\":1,\"id\":") + pending.id +
                    ",\"evt\":\"result\",\"cmd\":\"getRegion\",\"region\":" + String(region) + "}");
        clearPending();
      }
      break;
    case PendingAction::SetRegion:
      if (rfid::yrm100::decodeCommandStatus(frame, pending.yrmCommand, status) && status.status == 0x00) {
        notifyEvent(String("{\"v\":1,\"id\":") + pending.id + ",\"evt\":\"result\",\"cmd\":\"setRegion\",\"ok\":true}");
        clearPending();
      }
      break;
    case PendingAction::WriteSelectMode:
      if (rfid::yrm100::decodeCommandStatus(frame, pending.yrmCommand, status) && status.status == 0x00) {
        const int id = pending.id;
        clearPending();
        if (!sendWriteStep(PendingAction::WriteSelectTag,
                           static_cast<uint8_t>(rfid::yrm100::Command::SetSelect),
                           rfid::yrm100::buildSetSelectByEpc(writeContext.targetEpc))) {
          clearWriteContext();
          notifyError(id, "write_failed", "firmware", "Could not select the target tag");
        }
      }
      break;
    case PendingAction::WriteSelectTag:
      if (rfid::yrm100::decodeCommandStatus(frame, pending.yrmCommand, status) && status.status == 0x00) {
        const int id = pending.id;
        clearPending();
        if (!sendWriteStep(PendingAction::WriteEpc,
                           static_cast<uint8_t>(rfid::yrm100::Command::WriteMemory),
                           rfid::yrm100::buildWriteEpc(writeContext.newEpc))) {
          clearWriteContext();
          notifyError(id, "write_failed", "firmware", "Could not build the write command");
        }
      }
      break;
    case PendingAction::WriteEpc:
      if (rfid::yrm100::decodeCommandStatus(frame, pending.yrmCommand, status) && status.status == 0x00) {
        lastInventoryEpc = writeContext.newEpc;
        const int id = pending.id;
        clearPending();
        if (!sendWriteStep(PendingAction::WriteClearSelectMode,
                           static_cast<uint8_t>(rfid::yrm100::Command::SetSelectMode),
                           rfid::yrm100::buildSetSelectMode(rfid::yrm100::SelectMode::Disabled))) {
          clearWriteContext();
          notifyEvent(String("{\"v\":1,\"id\":") + id +
                      ",\"evt\":\"result\",\"cmd\":\"writeEpc\",\"ok\":true,\"epc\":\"" +
                      bytesToHex(lastInventoryEpc) + "\"}");
        }
      }
      break;
    case PendingAction::WriteClearSelectMode:
      if (rfid::yrm100::decodeCommandStatus(frame, pending.yrmCommand, status) && status.status == 0x00) {
        notifyEvent(String("{\"v\":1,\"id\":") + pending.id +
                    ",\"evt\":\"result\",\"cmd\":\"writeEpc\",\"ok\":true,\"epc\":\"" +
                    bytesToHex(writeContext.newEpc) + "\"}");
        clearWriteContext();
        clearPending();
      }
      break;
    case PendingAction::None:
      break;
  }

  updateStatusCharacteristic();
}

static void pollYrmSerial() {
  size_t bytesRead = 0;
  while (YrmSerial.available() > 0 && bytesRead < kMaxYrmBytesPerLoop) {
    bytesRead++;
    const auto event = yrmParser.feed(static_cast<uint8_t>(YrmSerial.read()));
    switch (event.status) {
      case rfid::yrm100::ParseStatus::FrameReady:
        handleYrmFrame(event.frame);
        break;
      case rfid::yrm100::ParseStatus::FrameTooLarge:
      case rfid::yrm100::ParseStatus::BadChecksum:
      case rfid::yrm100::ParseStatus::BadEnd:
        notifyError(pending.id, "parser_error", "yrm100", "Invalid YRM100 frame");
        if (isWriteAction(pending.action)) {
          clearWriteContext();
        }
        clearPending();
        updateStatusCharacteristic();
        break;
      case rfid::yrm100::ParseStatus::Waiting:
      case rfid::yrm100::ParseStatus::IgnoredByte:
        break;
    }
  }
}

static void checkCommandTimeout() {
  if (!hasPending()) {
    return;
  }

  if (millis() - pending.sentAtMs < kCommandTimeoutMs) {
    return;
  }

  const PendingAction timedOutAction = pending.action;
  notifyError(pending.id, "reader_timeout", "yrm100", "Reader did not respond");
  if (timedOutAction == PendingAction::StartInventory && scanActive) {
    const auto stopFrame = rfid::yrm100::buildStopMultipleInventory();
    Serial.println("[YRM TX] stop inventory after start timeout");
    YrmSerial.write(stopFrame.data(), stopFrame.size());
    YrmSerial.flush();
    scanActive = false;
  }
  if (timedOutAction == PendingAction::StartInventory || timedOutAction == PendingAction::StopInventory) {
    lastInventoryEndMs = millis();
  }
  if (isWriteAction(pending.action)) {
    clearWriteContext();
  }
  clearPending();
  updateStatusCharacteristic();
}

static void handleCommand(const String &message) {
  const int id = parseId(message);

  Serial.print("[BLE CMD] ");
  Serial.println(message);

  const AppCommand command = parseCommand(message);
  switch (command) {
    case AppCommand::GetInfo:
      notifyEvent(String("{\"v\":1,\"id\":") + id +
                  ",\"evt\":\"info\",\"name\":\"" + kDeviceName +
                  "\",\"fw\":\"" + kFirmwareVersion +
                  "\",\"caps\":[\"inventory\",\"power\",\"region\",\"writeEpc\"]}");
      break;
    case AppCommand::Status:
      notifyEvent(statusJson(id));
      break;
    case AppCommand::StopInventory:
      if (!scanActive && pending.action != PendingAction::StartInventory) {
        notifyEvent(String("{\"v\":1,\"id\":") + id + ",\"evt\":\"scanStopped\",\"reason\":\"not_running\"}");
        break;
      }
      sendStopInventoryCommand(id);
      break;
    case AppCommand::StartInventory:
      if (millis() - lastInventoryEndMs < kInventoryCooldownMs) {
        notifyError(id, "reader_busy", "yrm100", "Reader is settling after the last inventory");
        break;
      }
      scanActive = false;
      sendYrmCommand(id, PendingAction::StartInventory, static_cast<uint8_t>(rfid::yrm100::Command::SingleInventory),
                     rfid::yrm100::buildSingleInventory());
      break;
    case AppCommand::GetPower:
      sendYrmCommand(id, PendingAction::GetPower, static_cast<uint8_t>(rfid::yrm100::Command::GetTxPower),
                     rfid::yrm100::buildGetTxPower());
      break;
    case AppCommand::SetPower:
      {
        int16_t centiDbm = 0;
        if (!parsePowerCentiDbm(message, centiDbm)) {
          notifyError(id, "invalid_argument", "ble", "setPower requires integer dbm from 0 to 26");
          break;
        }
        currentPowerCentiDbm = centiDbm;
        sendYrmCommand(id, PendingAction::SetPower, static_cast<uint8_t>(rfid::yrm100::Command::SetTxPower),
                       rfid::yrm100::buildSetTxPowerCentiDbm(centiDbm));
      }
      break;
    case AppCommand::GetRegion:
      sendYrmCommand(id, PendingAction::GetRegion, static_cast<uint8_t>(rfid::yrm100::Command::GetRegion),
                     rfid::yrm100::buildGetRegion());
      break;
    case AppCommand::SetRegion:
      {
        uint8_t region = 0;
        if (!parseRegionValue(message, region)) {
          notifyError(id, "invalid_argument", "ble", "setRegion requires region \"US\" or numeric code");
          break;
        }
        currentRegion = region;
        sendYrmCommand(id, PendingAction::SetRegion, static_cast<uint8_t>(rfid::yrm100::Command::SetRegion),
                       rfid::yrm100::buildSetRegion(region));
      }
      break;
    case AppCommand::WriteEpc:
      {
        std::vector<uint8_t> epc;
        if (!parseEpcBytes(message, epc)) {
          notifyError(id, "invalid_argument", "ble", "writeEpc requires an EPC hex string");
          break;
        }
        beginWriteEpc(id, epc);
      }
      break;
    case AppCommand::Unknown:
      notifyError(id, "unknown_command", "ble", "Command is not supported");
      break;
  }
}

static void processBleLifecycleEvents() {
  if (bleConnectPending) {
    bleConnectPending = false;
    Serial.println("[BLE] central connected");
    updateStatusCharacteristic();
  }

  if (bleDisconnectPending) {
    bleDisconnectPending = false;
    clearBleCommandQueue();
    clearBleEventQueue();
    stopInventoryForDisconnect();
    updateStatusCharacteristic();
    Serial.println("[BLE] central disconnected");
  }

  if (bleRestartAdvertisingPending) {
    bleRestartAdvertisingPending = false;
    BLEDevice::startAdvertising();
    Serial.println("[BLE] advertising restarted");
  }
}

static void processQueuedBleCommands() {
  if (bleCommandTooLong) {
    bleCommandTooLong = false;
    notifyError(0, "invalid_argument", "ble", "Command payload is too large");
  }

  if (bleCommandOverflow) {
    bleCommandOverflow = false;
    notifyError(0, "busy", "firmware", "Command queue is full");
  }

  if (!bleConnected) {
    clearBleCommandQueue();
    return;
  }

  String message;
  while (dequeueBleCommand(message)) {
    handleCommand(message);
  }
}

static void processQueuedBleEvents() {
  if (bleEventTooLong) {
    bleEventTooLong = false;
    Serial.println("[BLE EVT] dropped oversized event");
  }

  if (bleEventOverflow) {
    bleEventOverflow = false;
    Serial.println("[BLE EVT] dropped event because queue is full");
  }

  if (!bleConnected || eventsCharacteristic == nullptr) {
    clearBleEventQueue();
    return;
  }

  const uint32_t now = millis();
  if (now - lastBleNotifyMs < kBleNotifySpacingMs) {
    return;
  }

  char text[kMaxBleEventLength + 1] = {};
  if (!dequeueBleEvent(text, sizeof(text))) {
    return;
  }

  eventsCharacteristic->setValue(text);
  eventsCharacteristic->notify();
  lastBleNotifyMs = now;
}

class ServerCallbacks final : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    (void)server;
    bleConnected = true;
    bleConnectPending = true;
    statusUpdatePending = true;
  }

  void onDisconnect(BLEServer *server) override {
    (void)server;
    bleConnected = false;
    bleDisconnectPending = true;
    bleRestartAdvertisingPending = true;
    statusUpdatePending = true;
  }
};

class CommandCallbacks final : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    const String value = characteristic->getValue().c_str();
    if (value.length() == 0) {
      return;
    }
    enqueueBleCommand(value.c_str(), value.length());
  }
};

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("RFID Tools ESP32 BLE service");
  Serial.print("Firmware version: ");
  Serial.println(kFirmwareVersion);
  Serial.print("Reset reason: ");
  Serial.println(resetReasonName(esp_reset_reason()));
  Serial.print("Service UUID: ");
  Serial.println(kServiceUuid);
  Serial.print("YRM100 UART RX pin: ");
  Serial.print(kYrmRxPin);
  Serial.print(" TX pin: ");
  Serial.print(kYrmTxPin);
  Serial.print(" baud: ");
  Serial.println(kYrmBaud);

  YrmSerial.begin(kYrmBaud, SERIAL_8N1, kYrmRxPin, kYrmTxPin);
  readerReady = true;

  BLEDevice::init(kDeviceName);
  bleServer = BLEDevice::createServer();
  bleServer->setCallbacks(new ServerCallbacks());

  BLEService *service = bleServer->createService(kServiceUuid);

  BLECharacteristic *commandCharacteristic = service->createCharacteristic(
      kCommandUuid,
      BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  commandCharacteristic->setCallbacks(new CommandCallbacks());

  eventsCharacteristic = service->createCharacteristic(
      kEventsUuid,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  eventsCharacteristic->addDescriptor(new BLE2902());

  statusCharacteristic = service->createCharacteristic(
      kStatusUuid,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  statusCharacteristic->addDescriptor(new BLE2902());

  updateStatusCharacteristic();
  eventsCharacteristic->setValue("{\"v\":1,\"id\":null,\"evt\":\"boot\",\"state\":\"idle\"}");

  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(kServiceUuid);
  advertising->setScanResponse(true);
  advertising->setMinPreferred(0x06);
  advertising->setMinPreferred(0x12);
  advertising->start();

  Serial.println("[BLE] advertising started");
}

void loop() {
  static uint32_t lastStatusUpdateMs = 0;
  const uint32_t now = millis();

  processBleLifecycleEvents();
  processQueuedBleCommands();
  processQueuedBleEvents();
  pollYrmSerial();
  checkCommandTimeout();

  if (statusUpdatePending || now - lastStatusUpdateMs >= 1000) {
    statusUpdatePending = false;
    lastStatusUpdateMs = now;
    updateStatusCharacteristic();
  }

  delay(1);
}
