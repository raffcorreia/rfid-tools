#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "../../../firmware/esp32/yrm100_driver/Yrm100Driver.h"

using rfid::yrm100::Command;
using rfid::yrm100::ErrorResponse;
using rfid::yrm100::Frame;
using rfid::yrm100::FrameParser;
using rfid::yrm100::FrameType;
using rfid::yrm100::InventoryTag;
using rfid::yrm100::MemoryBank;
using rfid::yrm100::ModuleInfo;
using rfid::yrm100::ModuleInfoSelector;
using rfid::yrm100::ParseStatus;
using rfid::yrm100::SelectMode;

namespace {

int failures = 0;

std::string hexBytes(const std::vector<uint8_t> &bytes) {
  std::ostringstream out;
  out << std::hex << std::uppercase;
  for (size_t i = 0; i < bytes.size(); i++) {
    if (i > 0) {
      out << ' ';
    }
    if (bytes[i] < 0x10) {
      out << '0';
    }
    out << static_cast<int>(bytes[i]);
  }
  return out.str();
}

void expectTrue(bool condition, const std::string &name) {
  if (!condition) {
    std::cerr << "FAIL: " << name << "\n";
    failures++;
  }
}

void expectBytes(const std::string &name, const std::vector<uint8_t> &actual, const std::vector<uint8_t> &expected) {
  if (actual != expected) {
    std::cerr << "FAIL: " << name << "\n"
              << "  actual:   " << hexBytes(actual) << "\n"
              << "  expected: " << hexBytes(expected) << "\n";
    failures++;
  }
}

Frame parseOne(const std::vector<uint8_t> &bytes) {
  FrameParser parser;
  Frame frame;
  bool ready = false;
  for (uint8_t byte : bytes) {
    const auto event = parser.feed(byte);
    if (event.status == ParseStatus::FrameReady) {
      frame = event.frame;
      ready = true;
    }
  }
  expectTrue(ready, "parseOne produced a frame");
  return frame;
}

void testCommandBuilders() {
  expectBytes("hardware version command",
              rfid::yrm100::buildGetModuleInfo(ModuleInfoSelector::Hardware),
              {0xBB, 0x00, 0x03, 0x00, 0x01, 0x00, 0x04, 0x7E});

  expectBytes("single inventory command",
              rfid::yrm100::buildSingleInventory(),
              {0xBB, 0x00, 0x22, 0x00, 0x00, 0x22, 0x7E});

  expectBytes("multiple inventory command",
              rfid::yrm100::buildMultipleInventory(),
              {0xBB, 0x00, 0x27, 0x00, 0x03, 0x22, 0xFF, 0xFF, 0x4A, 0x7E});

  expectBytes("stop multiple inventory command",
              rfid::yrm100::buildStopMultipleInventory(),
              {0xBB, 0x00, 0x28, 0x00, 0x00, 0x28, 0x7E});

  expectBytes("get region command",
              rfid::yrm100::buildGetRegion(),
              {0xBB, 0x00, 0x08, 0x00, 0x00, 0x08, 0x7E});

  expectBytes("get tx power command",
              rfid::yrm100::buildGetTxPower(),
              {0xBB, 0x00, 0xB7, 0x00, 0x00, 0xB7, 0x7E});

  expectBytes("set tx power 15dBm command",
              rfid::yrm100::buildSetTxPowerCentiDbm(1500),
              {0xBB, 0x00, 0xB6, 0x00, 0x02, 0x05, 0xDC, 0x99, 0x7E});

  expectBytes("set select mode before tag operations",
              rfid::yrm100::buildSetSelectMode(SelectMode::BeforeTagOperations),
              {0xBB, 0x00, 0x12, 0x00, 0x01, 0x02, 0x15, 0x7E});

  expectBytes("set select mode disabled",
              rfid::yrm100::buildSetSelectMode(SelectMode::Disabled),
              {0xBB, 0x00, 0x12, 0x00, 0x01, 0x01, 0x14, 0x7E});

  expectBytes("write epc command",
              rfid::yrm100::buildWriteEpc({0xE2, 0x80, 0x11, 0x70, 0x40, 0x00, 0x02, 0x1D, 0x35, 0xAE, 0x40, 0x08}),
              {0xBB, 0x00, 0x49, 0x00, 0x15, 0x00, 0x00, 0x00, 0x00, 0x01,
               0x00, 0x02, 0x00, 0x06, 0xE2, 0x80, 0x11, 0x70, 0x40, 0x00,
               0x02, 0x1D, 0x35, 0xAE, 0x40, 0x08, 0xD4, 0x7E});

  const uint8_t password[4] = {0x12, 0x34, 0x56, 0x78};
  expectBytes("read user memory command",
              rfid::yrm100::buildReadMemory(password, MemoryBank::User, 0x0001, 0x0002),
              {0xBB, 0x00, 0x39, 0x00, 0x09, 0x12, 0x34, 0x56, 0x78, 0x03,
               0x00, 0x01, 0x00, 0x02, 0x5C, 0x7E});

  expectTrue(rfid::yrm100::buildWriteEpc({0xE2}).empty(), "odd-length EPC write rejected");
  expectTrue(rfid::yrm100::buildSetSelectByEpc({}).empty(), "empty EPC select rejected");
}

void testParserAndInventoryDecode() {
  const std::vector<uint8_t> raw = {
      0xBB, 0x02, 0x22, 0x00, 0x11, 0xD6, 0x34, 0x00, 0xE2, 0x80, 0x11, 0x70,
      0x40, 0x00, 0x02, 0x1D, 0x35, 0xAE, 0x40, 0x08, 0xD4, 0x44, 0xC4, 0x7E};
  const Frame frame = parseOne(raw);
  expectTrue(frame.type == FrameType::Notice, "inventory frame type");
  expectTrue(frame.command == static_cast<uint8_t>(Command::SingleInventory), "inventory command");
  expectTrue(frame.payload.size() == 17, "inventory payload length");

  InventoryTag tag;
  expectTrue(rfid::yrm100::decodeInventoryTag(frame, tag), "decode inventory tag");
  expectTrue(tag.rssiRaw == 0xD6, "inventory rssi raw");
  expectTrue(tag.rssiDbm == -42, "inventory rssi signed");
  expectTrue(tag.pc == 0x3400, "inventory pc");
  expectBytes("inventory epc", tag.epc, {0xE2, 0x80, 0x11, 0x70, 0x40, 0x00, 0x02, 0x1D, 0x35, 0xAE, 0x40, 0x08});
  expectTrue(tag.crc == 0xD444, "inventory tag crc");
}

void testErrorDecode() {
  const Frame frame = parseOne({0xBB, 0x01, 0xFF, 0x00, 0x01, 0x15, 0x16, 0x7E});
  ErrorResponse error;
  expectTrue(rfid::yrm100::decodeErrorResponse(frame, error), "decode no-tag error response");
  expectTrue(error.code == 0x15, "error code");
}

void testResponseDecoders() {
  const Frame moduleInfoFrame = parseOne({
      0xBB, 0x01, 0x03, 0x00, 0x10, 0x00, 0x4D, 0x31, 0x30, 0x30, 0x20, 0x32,
      0x36, 0x64, 0x42, 0x6D, 0x20, 0x56, 0x31, 0x2E, 0x30, 0x92, 0x7E});
  ModuleInfo moduleInfo;
  expectTrue(rfid::yrm100::decodeModuleInfo(moduleInfoFrame, moduleInfo), "decode module info");
  expectTrue(moduleInfo.selector == 0x00, "module info selector");
  expectBytes("module info text bytes",
              moduleInfo.textBytes,
              {0x4D, 0x31, 0x30, 0x30, 0x20, 0x32, 0x36, 0x64, 0x42, 0x6D, 0x20, 0x56, 0x31, 0x2E, 0x30});

  uint8_t region = 0;
  expectTrue(rfid::yrm100::decodeRegion(parseOne({0xBB, 0x01, 0x08, 0x00, 0x01, 0x01, 0x0B, 0x7E}), region), "decode region");
  expectTrue(region == 0x01, "region value");

  uint16_t txPower = 0;
  expectTrue(rfid::yrm100::decodeTxPowerCentiDbm(parseOne({0xBB, 0x01, 0xB7, 0x00, 0x02, 0x0A, 0x28, 0xEC, 0x7E}), txPower),
             "decode tx power");
  expectTrue(txPower == 2600, "tx power value");

  rfid::yrm100::CommandStatus status;
  expectTrue(rfid::yrm100::decodeCommandStatus(parseOne({0xBB, 0x01, 0x28, 0x00, 0x01, 0x00, 0x2A, 0x7E}),
                                               static_cast<uint8_t>(Command::StopMultipleInventory),
                                               status),
             "decode command status");
  expectTrue(status.status == 0x00, "command status success value");
}

void testSplitFramesAndStrayBytes() {
  FrameParser parser;
  const std::vector<uint8_t> bytes = {
      0x00, 0x55,
      0xBB, 0x01, 0x08, 0x00,
      0x01, 0x01, 0x0B, 0x7E,
      0xBB, 0x01, 0xB7, 0x00, 0x02, 0x0A, 0x28, 0xEC, 0x7E};

  size_t frames = 0;
  bool ignored = false;
  for (uint8_t byte : bytes) {
    const auto event = parser.feed(byte);
    if (event.status == ParseStatus::IgnoredByte) {
      ignored = true;
    }
    if (event.status == ParseStatus::FrameReady) {
      frames++;
      if (frames == 1) {
        expectTrue(event.frame.command == static_cast<uint8_t>(Command::GetRegion), "first split frame command");
      }
      if (frames == 2) {
        expectTrue(event.frame.command == static_cast<uint8_t>(Command::GetTxPower), "second frame command");
      }
    }
  }

  expectTrue(ignored, "stray bytes ignored");
  expectTrue(frames == 2, "two frames parsed from one byte stream");
}

void testMalformedFrames() {
  FrameParser parser;
  ParseStatus status = ParseStatus::Waiting;
  for (uint8_t byte : {0xBB, 0x01, 0x08, 0x00, 0x01, 0x01, 0x00, 0x7E}) {
    status = parser.feed(byte).status;
  }
  expectTrue(status == ParseStatus::BadChecksum, "bad checksum rejected");

  parser.reset();
  for (uint8_t byte : {0xBB, 0x01, 0x08, 0x00, 0x01, 0x01, 0x0B, 0x00}) {
    status = parser.feed(byte).status;
  }
  expectTrue(status == ParseStatus::BadEnd, "bad end rejected");
}

}  // namespace

int main() {
  testCommandBuilders();
  testParserAndInventoryDecode();
  testErrorDecode();
  testResponseDecoders();
  testSplitFramesAndStrayBytes();
  testMalformedFrames();

  if (failures > 0) {
    std::cerr << failures << " test failure(s)\n";
    return EXIT_FAILURE;
  }

  std::cout << "YRM100 driver tests passed\n";
  return EXIT_SUCCESS;
}
