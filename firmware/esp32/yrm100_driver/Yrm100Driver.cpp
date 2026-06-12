#include "Yrm100Driver.h"

#include <algorithm>

namespace rfid::yrm100 {
namespace {

constexpr uint8_t kHeader = 0xBB;
constexpr uint8_t kEnd = 0x7E;
constexpr uint16_t kEpcStartWord = 0x0002;
constexpr uint32_t kEpcSelectPointerBits = 0x20;
constexpr uint8_t kSelectTargetS0 = 0x00;
constexpr uint8_t kSelectActionAssert = 0x00;
constexpr uint8_t kSelectMemBankEpc = 0x01;
constexpr uint8_t kSelectTruncatedDisabled = 0x00;
constexpr uint8_t kDefaultAccessPassword[4] = {0x00, 0x00, 0x00, 0x00};

void appendU16(std::vector<uint8_t> &out, uint16_t value) {
  out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
  out.push_back(static_cast<uint8_t>(value & 0xFF));
}

void appendU32(std::vector<uint8_t> &out, uint32_t value) {
  out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
  out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
  out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
  out.push_back(static_cast<uint8_t>(value & 0xFF));
}

}  // namespace

uint8_t checksum(const uint8_t *data, size_t len) {
  uint16_t sum = 0;
  for (size_t i = 1; i < len; i++) {
    sum += data[i];
  }
  return static_cast<uint8_t>(sum & 0xFF);
}

uint8_t checksum(const std::vector<uint8_t> &data, size_t len) {
  return checksum(data.data(), std::min(len, data.size()));
}

std::vector<uint8_t> buildFrame(FrameType type, uint8_t command, const std::vector<uint8_t> &payload) {
  std::vector<uint8_t> frame;
  frame.reserve(7 + payload.size());
  frame.push_back(kHeader);
  frame.push_back(static_cast<uint8_t>(type));
  frame.push_back(command);
  appendU16(frame, static_cast<uint16_t>(payload.size()));
  frame.insert(frame.end(), payload.begin(), payload.end());
  frame.push_back(checksum(frame, frame.size()));
  frame.push_back(kEnd);
  return frame;
}

std::vector<uint8_t> buildCommand(uint8_t command, const std::vector<uint8_t> &payload) {
  return buildFrame(FrameType::Command, command, payload);
}

std::vector<uint8_t> buildGetModuleInfo(ModuleInfoSelector selector) {
  return buildCommand(static_cast<uint8_t>(Command::GetModuleInfo), {static_cast<uint8_t>(selector)});
}

std::vector<uint8_t> buildSingleInventory() {
  return buildCommand(static_cast<uint8_t>(Command::SingleInventory));
}

std::vector<uint8_t> buildMultipleInventory(uint16_t rounds) {
  std::vector<uint8_t> payload = {static_cast<uint8_t>(Command::SingleInventory)};
  appendU16(payload, rounds);
  return buildCommand(static_cast<uint8_t>(Command::MultipleInventory), payload);
}

std::vector<uint8_t> buildStopMultipleInventory() {
  return buildCommand(static_cast<uint8_t>(Command::StopMultipleInventory));
}

std::vector<uint8_t> buildGetRegion() {
  return buildCommand(static_cast<uint8_t>(Command::GetRegion));
}

std::vector<uint8_t> buildSetRegion(uint8_t region) {
  return buildCommand(static_cast<uint8_t>(Command::SetRegion), {region});
}

std::vector<uint8_t> buildGetTxPower() {
  return buildCommand(static_cast<uint8_t>(Command::GetTxPower));
}

std::vector<uint8_t> buildSetTxPowerCentiDbm(int16_t centiDbm) {
  const auto value = static_cast<uint16_t>(centiDbm);
  std::vector<uint8_t> payload;
  appendU16(payload, value);
  return buildCommand(static_cast<uint8_t>(Command::SetTxPower), payload);
}

std::vector<uint8_t> buildSetSelectMode(SelectMode mode) {
  return buildCommand(static_cast<uint8_t>(Command::SetSelectMode), {static_cast<uint8_t>(mode)});
}

std::vector<uint8_t> buildSetSelectByEpc(const std::vector<uint8_t> &epc) {
  if (epc.empty() || epc.size() > 31) {
    return {};
  }

  std::vector<uint8_t> payload;
  payload.reserve(7 + epc.size());
  const uint8_t selectByte = static_cast<uint8_t>((kSelectTargetS0 << 5) | (kSelectActionAssert << 2) | kSelectMemBankEpc);
  payload.push_back(selectByte);
  appendU32(payload, kEpcSelectPointerBits);
  payload.push_back(static_cast<uint8_t>(epc.size() * 8));
  payload.push_back(kSelectTruncatedDisabled);
  payload.insert(payload.end(), epc.begin(), epc.end());
  return buildCommand(static_cast<uint8_t>(Command::SetSelect), payload);
}

std::vector<uint8_t> buildReadMemory(const uint8_t accessPassword[4], MemoryBank bank, uint16_t startWord, uint16_t wordCount) {
  const uint8_t *password = accessPassword == nullptr ? kDefaultAccessPassword : accessPassword;
  std::vector<uint8_t> payload;
  payload.reserve(9);
  payload.insert(payload.end(), password, password + 4);
  payload.push_back(static_cast<uint8_t>(bank));
  appendU16(payload, startWord);
  appendU16(payload, wordCount);
  return buildCommand(static_cast<uint8_t>(Command::ReadMemory), payload);
}

std::vector<uint8_t> buildWriteMemory(
    const uint8_t accessPassword[4],
    MemoryBank bank,
    uint16_t startWord,
    const std::vector<uint8_t> &data) {
  if (data.empty() || (data.size() % 2) != 0 || data.size() > 64) {
    return {};
  }

  const uint8_t *password = accessPassword == nullptr ? kDefaultAccessPassword : accessPassword;
  std::vector<uint8_t> payload;
  payload.reserve(9 + data.size());
  payload.insert(payload.end(), password, password + 4);
  payload.push_back(static_cast<uint8_t>(bank));
  appendU16(payload, startWord);
  appendU16(payload, static_cast<uint16_t>(data.size() / 2));
  payload.insert(payload.end(), data.begin(), data.end());
  return buildCommand(static_cast<uint8_t>(Command::WriteMemory), payload);
}

std::vector<uint8_t> buildWriteEpc(const std::vector<uint8_t> &epc, const uint8_t accessPassword[4]) {
  return buildWriteMemory(accessPassword, MemoryBank::Epc, kEpcStartWord, epc);
}

bool decodeInventoryTag(const Frame &frame, InventoryTag &tag) {
  if (frame.type != FrameType::Notice || frame.command != static_cast<uint8_t>(Command::SingleInventory)) {
    return false;
  }
  if (frame.payload.size() < 5) {
    return false;
  }

  const uint16_t pc = (static_cast<uint16_t>(frame.payload[1]) << 8) | frame.payload[2];
  const size_t maxEpcLen = frame.payload.size() - 5;
  const size_t pcEpcLen = static_cast<size_t>((pc >> 11) & 0x1F) * 2;
  const size_t epcLen = pcEpcLen > 0 && pcEpcLen <= maxEpcLen ? pcEpcLen : maxEpcLen;
  if (epcLen == 0 || 3 + epcLen + 1 >= frame.payload.size()) {
    return false;
  }

  tag.rssiRaw = frame.payload[0];
  tag.rssiDbm = static_cast<int8_t>(tag.rssiRaw);
  tag.pc = pc;
  tag.epc.assign(frame.payload.begin() + 3, frame.payload.begin() + 3 + static_cast<std::ptrdiff_t>(epcLen));
  const size_t crcIndex = 3 + epcLen;
  tag.crc = (static_cast<uint16_t>(frame.payload[crcIndex]) << 8) | frame.payload[crcIndex + 1];
  return true;
}

bool decodeErrorResponse(const Frame &frame, ErrorResponse &error) {
  if (frame.type != FrameType::Response || frame.command != 0xFF || frame.payload.size() != 1) {
    return false;
  }
  error.code = frame.payload[0];
  return true;
}

bool decodeModuleInfo(const Frame &frame, ModuleInfo &info) {
  if (frame.type != FrameType::Response || frame.command != static_cast<uint8_t>(Command::GetModuleInfo)) {
    return false;
  }
  if (frame.payload.size() < 2) {
    return false;
  }

  info.selector = frame.payload[0];
  info.textBytes.assign(frame.payload.begin() + 1, frame.payload.end());
  return true;
}

bool decodeRegion(const Frame &frame, uint8_t &region) {
  if (frame.type != FrameType::Response || frame.command != static_cast<uint8_t>(Command::GetRegion) || frame.payload.size() != 1) {
    return false;
  }

  region = frame.payload[0];
  return true;
}

bool decodeTxPowerCentiDbm(const Frame &frame, uint16_t &centiDbm) {
  if (frame.type != FrameType::Response || frame.command != static_cast<uint8_t>(Command::GetTxPower) || frame.payload.size() != 2) {
    return false;
  }

  centiDbm = (static_cast<uint16_t>(frame.payload[0]) << 8) | frame.payload[1];
  return true;
}

bool decodeCommandStatus(const Frame &frame, uint8_t command, CommandStatus &status) {
  if (frame.type != FrameType::Response || frame.command != command || frame.payload.size() != 1) {
    return false;
  }

  status.status = frame.payload[0];
  return true;
}

FrameParser::FrameParser(size_t maxFrameLen) : maxFrameLen_(maxFrameLen) {
  buffer_.reserve(maxFrameLen_);
}

void FrameParser::reset() {
  buffer_.clear();
  expectedLen_ = 0;
}

ParseEvent FrameParser::feed(uint8_t byte) {
  ParseEvent event;
  event.byte = byte;

  if (buffer_.empty()) {
    if (byte != kHeader) {
      event.status = ParseStatus::IgnoredByte;
      return event;
    }
    buffer_.push_back(byte);
    event.status = ParseStatus::Waiting;
    return event;
  }

  if (buffer_.size() >= maxFrameLen_) {
    reset();
    event.status = ParseStatus::FrameTooLarge;
    return event;
  }

  buffer_.push_back(byte);

  if (buffer_.size() == 5) {
    const uint16_t payloadLen = (static_cast<uint16_t>(buffer_[3]) << 8) | buffer_[4];
    expectedLen_ = 7 + payloadLen;
    if (expectedLen_ > maxFrameLen_) {
      reset();
      event.status = ParseStatus::FrameTooLarge;
      return event;
    }
  }

  if (expectedLen_ == 0 || buffer_.size() < expectedLen_) {
    event.status = ParseStatus::Waiting;
    return event;
  }

  if (buffer_.back() != kEnd) {
    reset();
    event.status = ParseStatus::BadEnd;
    return event;
  }

  const size_t checksumIndex = expectedLen_ - 2;
  const uint8_t expectedChecksum = checksum(buffer_, checksumIndex);
  if (buffer_[checksumIndex] != expectedChecksum) {
    reset();
    event.status = ParseStatus::BadChecksum;
    return event;
  }

  event.status = ParseStatus::FrameReady;
  event.frame.type = static_cast<FrameType>(buffer_[1]);
  event.frame.command = buffer_[2];
  event.frame.payload.assign(buffer_.begin() + 5, buffer_.begin() + static_cast<std::ptrdiff_t>(checksumIndex));
  event.frame.raw = buffer_;
  reset();
  return event;
}

}  // namespace rfid::yrm100
