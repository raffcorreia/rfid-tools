import CoreBluetooth
import Foundation

@MainActor
final class BLEManager: NSObject, ObservableObject {
    @Published private(set) var connectionState: ReaderConnectionState = .disconnected
    @Published private(set) var peripherals: [ReaderPeripheral] = []
    @Published private(set) var diagnostics: [DiagnosticEntry] = []
    @Published private(set) var tags: [TagRead] = []
    @Published private(set) var statusSummary = "No reader status"

    private let serviceUUID = CBUUID(string: "63802432-69FC-406D-A538-FE33CEF32AEF")
    private let commandUUID = CBUUID(string: "DE0D7201-CC2B-46D9-8F92-564A209C37EF")
    private let eventsUUID = CBUUID(string: "456F5CDA-632A-4541-A2A5-6FAEC234075E")
    private let statusUUID = CBUUID(string: "0AF05FBF-ADAA-4D94-8DCF-44A62F82332B")

    private var central: CBCentralManager?
    private var discoveredPeripherals: [UUID: CBPeripheral] = [:]
    private var connectedPeripheral: CBPeripheral?
    private var commandCharacteristic: CBCharacteristic?
    private var eventsCharacteristic: CBCharacteristic?
    private var statusCharacteristic: CBCharacteristic?
    private var nextCommandID = 1

    override init() {
        super.init()
        central = CBCentralManager(delegate: self, queue: nil)
    }

    func startScan() {
        guard central?.state == .poweredOn else {
            connectionState = .bluetoothUnavailable
            log(.error, "Bluetooth is not powered on")
            return
        }

        peripherals.removeAll()
        discoveredPeripherals.removeAll()
        connectionState = .scanning
        log(.app, "Scanning for RFID Tools ESP32")
        central?.scanForPeripherals(withServices: [serviceUUID], options: [CBCentralManagerScanOptionAllowDuplicatesKey: false])
    }

    func stopScan() {
        central?.stopScan()
        if case .scanning = connectionState {
            connectionState = .disconnected
        }
        log(.app, "Scan stopped")
    }

    func connect(to peripheral: ReaderPeripheral) {
        guard let cbPeripheral = discoveredPeripherals[peripheral.id] else {
            log(.error, "Peripheral is no longer available")
            return
        }

        central?.stopScan()
        connectionState = .connecting(peripheral.name)
        log(.app, "Connecting to \(peripheral.name)")
        connectedPeripheral = cbPeripheral
        cbPeripheral.delegate = self
        central?.connect(cbPeripheral)
    }

    func disconnect() {
        guard let connectedPeripheral else {
            connectionState = .disconnected
            return
        }

        log(.app, "Disconnect requested")
        central?.cancelPeripheralConnection(connectedPeripheral)
    }

    func send(_ command: RFIDCommand) {
        var payload: [String: Any] = [
            "v": 1,
            "id": nextCommandID,
            "cmd": command.rawValue
        ]

        switch command {
        case .setPower:
            payload["dbm"] = 15
        case .setRegion:
            payload["region"] = "US"
        case .getInfo, .status, .getPower, .getRegion, .startInventory, .stopInventory:
            break
        }

        nextCommandID += 1
        send(payload)
    }

    private func send(_ payload: [String: Any]) {
        guard let connectedPeripheral, let commandCharacteristic else {
            log(.error, "Command characteristic is not ready")
            return
        }

        do {
            let data = try JSONSerialization.data(withJSONObject: payload, options: [.sortedKeys])
            connectedPeripheral.writeValue(data, for: commandCharacteristic, type: .withResponse)
            log(.app, String(data: data, encoding: .utf8) ?? "<invalid command>")
        } catch {
            log(.error, "Failed to encode command: \(error.localizedDescription)")
        }
    }

    private func handleEventData(_ data: Data) {
        guard let text = String(data: data, encoding: .utf8) else {
            log(.error, "Received non-UTF8 event payload")
            return
        }

        log(.firmware, text)
        guard let object = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
              let event = object["evt"] as? String else {
            return
        }

        if event == "status" {
            statusSummary = text
        } else if event == "tagSeen", let epc = object["epc"] as? String {
            upsertTag(from: object, epc: epc)
        }
    }

    private func upsertTag(from object: [String: Any], epc: String) {
        let pc = object["pc"] as? String
        let rssi = object["rssi"] as? Int
        let crc = object["crc"] as? String

        if let index = tags.firstIndex(where: { $0.epc == epc }) {
            tags[index].pc = pc ?? tags[index].pc
            tags[index].rssi = rssi ?? tags[index].rssi
            tags[index].crc = crc ?? tags[index].crc
            tags[index].seenCount += 1
            tags[index].updatedAt = Date()
        } else {
            tags.insert(
                TagRead(id: epc, epc: epc, pc: pc, rssi: rssi, crc: crc, seenCount: 1, updatedAt: Date()),
                at: 0
            )
        }
    }

    private func resetConnectionState() {
        connectedPeripheral = nil
        commandCharacteristic = nil
        eventsCharacteristic = nil
        statusCharacteristic = nil
    }

    private func log(_ direction: DiagnosticEntry.Direction, _ message: String) {
        diagnostics.insert(DiagnosticEntry(direction: direction, message: message), at: 0)
        if diagnostics.count > 250 {
            diagnostics.removeLast(diagnostics.count - 250)
        }
    }
}

extension BLEManager: CBCentralManagerDelegate {
    nonisolated func centralManagerDidUpdateState(_ central: CBCentralManager) {
        Task { @MainActor in
            switch central.state {
            case .poweredOn:
                if case .bluetoothUnavailable = connectionState {
                    connectionState = .disconnected
                }
                log(.ble, "Bluetooth powered on")
            case .poweredOff:
                connectionState = .bluetoothUnavailable
                log(.ble, "Bluetooth powered off")
            case .unauthorized:
                connectionState = .error("Bluetooth permission denied")
                log(.error, "Bluetooth permission denied")
            case .unsupported:
                connectionState = .error("Bluetooth unsupported")
                log(.error, "Bluetooth unsupported")
            case .resetting:
                log(.ble, "Bluetooth resetting")
            case .unknown:
                log(.ble, "Bluetooth state unknown")
            @unknown default:
                log(.ble, "Unhandled Bluetooth state")
            }
        }
    }

    nonisolated func centralManager(
        _ central: CBCentralManager,
        didDiscover peripheral: CBPeripheral,
        advertisementData: [String: Any],
        rssi RSSI: NSNumber
    ) {
        Task { @MainActor in
            let name = peripheral.name
                ?? advertisementData[CBAdvertisementDataLocalNameKey] as? String
                ?? "RFID reader"
            discoveredPeripherals[peripheral.identifier] = peripheral

            let item = ReaderPeripheral(id: peripheral.identifier, name: name, rssi: RSSI.intValue)
            if let index = peripherals.firstIndex(where: { $0.id == item.id }) {
                peripherals[index] = item
            } else {
                peripherals.append(item)
            }
            log(.ble, "Discovered \(name) RSSI \(RSSI)")
        }
    }

    nonisolated func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        Task { @MainActor in
            connectionState = .connected(peripheral.name ?? "RFID reader")
            log(.ble, "Connected")
            peripheral.discoverServices([serviceUUID])
        }
    }

    nonisolated func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        Task { @MainActor in
            resetConnectionState()
            connectionState = .error(error?.localizedDescription ?? "Failed to connect")
            log(.error, "Connection failed: \(error?.localizedDescription ?? "unknown error")")
        }
    }

    nonisolated func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        Task { @MainActor in
            resetConnectionState()
            connectionState = .disconnected
            log(.ble, "Disconnected\(error.map { ": \($0.localizedDescription)" } ?? "")")
        }
    }
}

extension BLEManager: CBPeripheralDelegate {
    nonisolated func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        Task { @MainActor in
            if let error {
                log(.error, "Service discovery failed: \(error.localizedDescription)")
                return
            }

            guard let service = peripheral.services?.first(where: { $0.uuid == serviceUUID }) else {
                log(.error, "RFID service not found")
                return
            }

            log(.ble, "RFID service discovered")
            peripheral.discoverCharacteristics([commandUUID, eventsUUID, statusUUID], for: service)
        }
    }

    nonisolated func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        Task { @MainActor in
            if let error {
                log(.error, "Characteristic discovery failed: \(error.localizedDescription)")
                return
            }

            for characteristic in service.characteristics ?? [] {
                switch characteristic.uuid {
                case commandUUID:
                    commandCharacteristic = characteristic
                    log(.ble, "Command characteristic ready")
                case eventsUUID:
                    eventsCharacteristic = characteristic
                    peripheral.setNotifyValue(true, for: characteristic)
                    log(.ble, "Subscribed to events")
                case statusUUID:
                    statusCharacteristic = characteristic
                    peripheral.readValue(for: characteristic)
                    peripheral.setNotifyValue(true, for: characteristic)
                    log(.ble, "Status characteristic ready")
                default:
                    break
                }
            }

            if commandCharacteristic != nil {
                send(.getInfo)
                send(.status)
            }
        }
    }

    nonisolated func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
        Task { @MainActor in
            if let error {
                log(.error, "Characteristic update failed: \(error.localizedDescription)")
                return
            }

            guard let data = characteristic.value else {
                return
            }

            if characteristic.uuid == eventsUUID || characteristic.uuid == statusUUID {
                handleEventData(data)
            }
        }
    }

    nonisolated func peripheral(_ peripheral: CBPeripheral, didWriteValueFor characteristic: CBCharacteristic, error: Error?) {
        Task { @MainActor in
            if let error {
                log(.error, "Command write failed: \(error.localizedDescription)")
            } else {
                log(.ble, "Command write acknowledged")
            }
        }
    }
}
