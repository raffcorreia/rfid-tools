#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace rfid::yrm100 {

enum class FrameType : uint8_t {
  Command = 0x00,
  Response = 0x01,
  Notice = 0x02,
};

enum class Command : uint8_t {
  GetModuleInfo = 0x03,
  GetRegion = 0x08,
  SetRegion = 0x07,
  SetSelect = 0x0C,
  SetSelectMode = 0x12,
  SingleInventory = 0x22,
  MultipleInventory = 0x27,
  StopMultipleInventory = 0x28,
  ReadMemory = 0x39,
  WriteMemory = 0x49,
  SetTxPower = 0xB6,
  GetTxPower = 0xB7,
};

enum class ModuleInfoSelector : uint8_t {
  Hardware = 0x00,
  Software = 0x01,
  Manufacturer = 0x02,
};

enum class MemoryBank : uint8_t {
  Reserved = 0x00,
  Epc = 0x01,
  Tid = 0x02,
  User = 0x03,
};

enum class SelectMode : uint8_t {
  BeforeInventoryRound = 0x00,
  Disabled = 0x01,
  BeforeTagOperations = 0x02,
};

enum class ParseStatus {
  Waiting,
  IgnoredByte,
  FrameReady,
  FrameTooLarge,
  BadChecksum,
  BadEnd,
};

struct Frame {
  FrameType type = FrameType::Response;
  uint8_t command = 0;
  std::vector<uint8_t> payload;
  std::vector<uint8_t> raw;
};

struct ParseEvent {
  ParseStatus status = ParseStatus::Waiting;
  Frame frame;
  uint8_t byte = 0;
};

struct InventoryTag {
  uint8_t rssiRaw = 0;
  int8_t rssiDbm = 0;
  uint16_t pc = 0;
  std::vector<uint8_t> epc;
  uint16_t crc = 0;
};

struct ErrorResponse {
  uint8_t code = 0;
};

struct ModuleInfo {
  uint8_t selector = 0;
  std::vector<uint8_t> textBytes;
};

struct CommandStatus {
  uint8_t status = 0;
};

uint8_t checksum(const uint8_t *data, size_t len);
uint8_t checksum(const std::vector<uint8_t> &data, size_t len);

std::vector<uint8_t> buildFrame(FrameType type, uint8_t command, const std::vector<uint8_t> &payload = {});
std::vector<uint8_t> buildCommand(uint8_t command, const std::vector<uint8_t> &payload = {});

std::vector<uint8_t> buildGetModuleInfo(ModuleInfoSelector selector);
std::vector<uint8_t> buildSingleInventory();
std::vector<uint8_t> buildMultipleInventory(uint16_t rounds = 0xFFFF);
std::vector<uint8_t> buildStopMultipleInventory();
std::vector<uint8_t> buildGetRegion();
std::vector<uint8_t> buildSetRegion(uint8_t region);
std::vector<uint8_t> buildGetTxPower();
std::vector<uint8_t> buildSetTxPowerCentiDbm(int16_t centiDbm);
std::vector<uint8_t> buildSetSelectMode(SelectMode mode);
std::vector<uint8_t> buildSetSelectByEpc(const std::vector<uint8_t> &epc);
std::vector<uint8_t> buildReadMemory(const uint8_t accessPassword[4], MemoryBank bank, uint16_t startWord, uint16_t wordCount);
std::vector<uint8_t> buildWriteMemory(
    const uint8_t accessPassword[4],
    MemoryBank bank,
    uint16_t startWord,
    const std::vector<uint8_t> &data);
std::vector<uint8_t> buildWriteEpc(const std::vector<uint8_t> &epc, const uint8_t accessPassword[4] = nullptr);

bool decodeInventoryTag(const Frame &frame, InventoryTag &tag);
bool decodeErrorResponse(const Frame &frame, ErrorResponse &error);
bool decodeModuleInfo(const Frame &frame, ModuleInfo &info);
bool decodeRegion(const Frame &frame, uint8_t &region);
bool decodeTxPowerCentiDbm(const Frame &frame, uint16_t &centiDbm);
bool decodeCommandStatus(const Frame &frame, uint8_t command, CommandStatus &status);

class FrameParser {
 public:
  explicit FrameParser(size_t maxFrameLen = 256);

  ParseEvent feed(uint8_t byte);
  void reset();

 private:
  size_t maxFrameLen_;
  std::vector<uint8_t> buffer_;
  size_t expectedLen_ = 0;
};

}  // namespace rfid::yrm100
