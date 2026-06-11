import Foundation

enum ReaderConnectionState: Equatable {
    case bluetoothUnavailable
    case disconnected
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
        case .scanning:
            return "Scanning"
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
    case hello
    case status
    case getPower
    case setPower
    case getRegion
    case setRegion
    case startInventory
    case stopInventory

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
}
