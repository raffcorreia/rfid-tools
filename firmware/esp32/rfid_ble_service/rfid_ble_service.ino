/*
  RFID Tools ESP32 BLE service sketch.

  PHASE-003 first pass:
  - advertise a stable custom RFID service
  - accept high-level JSON commands over BLE
  - emit JSON events and expose status

  The YRM100 UART driver is intentionally not wired in this first sketch so the
  existing yrm100_bringup sketch can continue to be tested independently.
*/

#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

static constexpr const char *kDeviceName = "RFID Tools ESP32";
static constexpr const char *kFirmwareVersion = "0.1.0";
static constexpr const char *kServiceUuid = "63802432-69FC-406D-A538-FE33CEF32AEF";
static constexpr const char *kCommandUuid = "DE0D7201-CC2B-46D9-8F92-564A209C37EF";
static constexpr const char *kEventsUuid = "456F5CDA-632A-4541-A2A5-6FAEC234075E";
static constexpr const char *kStatusUuid = "0AF05FBF-ADAA-4D94-8DCF-44A62F82332B";

static BLEServer *bleServer = nullptr;
static BLECharacteristic *eventsCharacteristic = nullptr;
static BLECharacteristic *statusCharacteristic = nullptr;

static bool bleConnected = false;
static bool scanActive = false;

enum class AppCommand {
  Hello,
  Status,
  StartInventory,
  StopInventory,
  GetPower,
  SetPower,
  GetRegion,
  SetRegion,
  Unknown,
};

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

  if (command == "hello") {
    return AppCommand::Hello;
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
  return AppCommand::Unknown;
}

static bool commandRequiresReader(AppCommand command) {
  switch (command) {
    case AppCommand::StartInventory:
    case AppCommand::GetPower:
    case AppCommand::SetPower:
    case AppCommand::GetRegion:
    case AppCommand::SetRegion:
      return true;
    case AppCommand::Hello:
    case AppCommand::Status:
    case AppCommand::StopInventory:
    case AppCommand::Unknown:
      return false;
  }
  return false;
}

static String statusJson() {
  String state = scanActive ? "scanning" : "idle";
  String reader = "not_ready";
  String connected = bleConnected ? "true" : "false";

  return String("{\"v\":1,\"id\":null,\"evt\":\"status\",\"state\":\"") + state +
         "\",\"reader\":\"" + reader +
         "\",\"bleConnected\":" + connected +
         ",\"fw\":\"" + kFirmwareVersion +
         "\",\"caps\":[\"ble\",\"status\"]}";
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

  if (eventsCharacteristic == nullptr) {
    return;
  }
  eventsCharacteristic->setValue(json.c_str());
  if (bleConnected) {
    eventsCharacteristic->notify();
  }
}

static void notifyError(int id, const char *code, const char *source, const char *message) {
  String json = String("{\"v\":1,\"id\":") + id +
                ",\"evt\":\"error\",\"code\":\"" + code +
                "\",\"source\":\"" + source +
                "\",\"message\":\"" + message + "\"}";
  notifyEvent(json);
}

static void handleCommand(const String &message) {
  const int id = parseId(message);

  Serial.print("[BLE CMD] ");
  Serial.println(message);

  const AppCommand command = parseCommand(message);
  switch (command) {
    case AppCommand::Hello:
      notifyEvent(String("{\"v\":1,\"id\":") + id +
                  ",\"evt\":\"hello\",\"name\":\"" + kDeviceName +
                  "\",\"fw\":\"" + kFirmwareVersion +
                  "\",\"caps\":[\"ble\",\"status\"]}");
      break;
    case AppCommand::Status:
      notifyEvent(String("{\"v\":1,\"id\":") + id +
                  ",\"evt\":\"status\",\"state\":\"" + (scanActive ? "scanning" : "idle") +
                  "\",\"reader\":\"not_ready\",\"fw\":\"" + kFirmwareVersion +
                  "\",\"caps\":[\"ble\",\"status\"]}");
      break;
    case AppCommand::StopInventory:
      scanActive = false;
      updateStatusCharacteristic();
      notifyEvent(String("{\"v\":1,\"id\":") + id + ",\"evt\":\"scanStopped\",\"reason\":\"requested\"}");
      break;
    case AppCommand::StartInventory:
    case AppCommand::GetPower:
    case AppCommand::SetPower:
    case AppCommand::GetRegion:
    case AppCommand::SetRegion:
      if (commandRequiresReader(command)) {
        notifyError(id, "not_implemented", "firmware", "YRM100 service wiring is not implemented yet");
      }
      break;
    case AppCommand::Unknown:
      notifyError(id, "unknown_command", "ble", "Command is not supported");
      break;
  }
}

class ServerCallbacks final : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    (void)server;
    bleConnected = true;
    updateStatusCharacteristic();
    Serial.println("[BLE] central connected");
  }

  void onDisconnect(BLEServer *server) override {
    bleConnected = false;
    scanActive = false;
    updateStatusCharacteristic();
    Serial.println("[BLE] central disconnected");
    server->getAdvertising()->start();
  }
};

class CommandCallbacks final : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    const String value = characteristic->getValue().c_str();
    if (value.length() == 0) {
      return;
    }
    handleCommand(value);
  }
};

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("RFID Tools ESP32 BLE service");
  Serial.print("Service UUID: ");
  Serial.println(kServiceUuid);

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

  if (now - lastStatusUpdateMs >= 1000) {
    lastStatusUpdateMs = now;
    updateStatusCharacteristic();
  }
}
