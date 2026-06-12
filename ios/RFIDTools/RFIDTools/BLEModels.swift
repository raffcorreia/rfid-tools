import Foundation

enum ReaderConnectionState: Equatable {
    case bluetoothUnavailable
    case disconnected
    case reconnecting
    case scanning
    case connecting(String)
    case connected(String)
    case error(String)

    var label: String {
        switch self {
        case .bluetoothUnavailable:
            return "Bluetooth unavailable"
        case .disconnected:
            return "Disconnected"
        case .reconnecting:
            return "Reconnecting"
        case .scanning:
            return "Looking for reader"
        case .connecting(let name):
            return "Connecting to \(name)"
        case .connected(let name):
            return "Connected to \(name)"
        case .error(let message):
            return message
        }
    }
}

enum RFIDCommand: String, CaseIterable, Identifiable {
    case getInfo
    case status
    case getPower
    case setPower
    case getRegion
    case setRegion
    case startInventory
    case stopInventory

    var id: String { rawValue }
}

enum TagDisplayFormat: String, CaseIterable, Identifiable {
    case text = "Text"
    case hex = "Hex"

    var id: String { rawValue }
}

struct ReaderPeripheral: Identifiable, Equatable {
    let id: UUID
    let name: String
    let rssi: Int
}

struct DiagnosticEntry: Identifiable {
    let id = UUID()
    let timestamp = Date()
    let direction: Direction
    let message: String

    enum Direction {
        case app
        case ble
        case firmware
        case error
    }

    var prefix: String {
        switch direction {
        case .app:
            return "APP"
        case .ble:
            return "BLE"
        case .firmware:
            return "ESP"
        case .error:
            return "ERR"
        }
    }
}

struct TagRead: Identifiable, Equatable {
    let id: String
    var epc: String
    var pc: String?
    var rssi: Int?
    var crc: String?
    var seenCount: Int
    var updatedAt: Date

    func displayValue(format: TagDisplayFormat) -> String {
        switch format {
        case .hex:
            return epc
        case .text:
            return epc.hexDecodedString ?? "Not readable as text"
        }
    }
}

struct SavedTag: Identifiable, Codable, Equatable {
    let id: UUID
    var name: String
    var epc: String
    var createdAt: Date

    init(id: UUID = UUID(), name: String, epc: String, createdAt: Date = Date()) {
        self.id = id
        self.name = name
        self.epc = epc
        self.createdAt = createdAt
    }
}

extension String {
    var hexDecodedString: String? {
        let cleaned = filter { !$0.isWhitespace }
        guard cleaned.count.isMultiple(of: 2), !cleaned.isEmpty else {
            return nil
        }

        var bytes: [UInt8] = []
        var index = cleaned.startIndex
        while index < cleaned.endIndex {
            let next = cleaned.index(index, offsetBy: 2)
            guard let byte = UInt8(cleaned[index..<next], radix: 16) else {
                return nil
            }
            bytes.append(byte)
            index = next
        }

        let printable = bytes.filter { $0 >= 32 && $0 <= 126 }
        guard printable.count == bytes.count else {
            return nil
        }

        return String(bytes: bytes, encoding: .utf8)
    }

    var utf8HexEncoded: String {
        data(using: .utf8)?
            .map { String(format: "%02X", $0) }
            .joined(separator: " ") ?? ""
    }
}
