import CoreBluetooth
import Foundation

@MainActor
final class BLEManager: NSObject, ObservableObject {
    @Published private(set) var connectionState: ReaderConnectionState = .disconnected
    @Published private(set) var peripherals: [ReaderPeripheral] = []
    @Published private(set) var diagnostics: [DiagnosticEntry] = []
    @Published private(set) var tags: [TagRead] = []
    @Published private(set) var savedTags: [SavedTag] = []
    @Published private(set) var statusSummary = "Searching for the reader when Bluetooth is ready"
    @Published private(set) var latestTag: TagRead?
    @Published private(set) var isInventoryRunning = false
    @Published private(set) var lastWriteMessage = "Read the target tag, stop reading, then write or apply a saved tag."
    @Published var isAutoConnectEnabled = true
    @Published var selectedPowerDbm = 15

    private let serviceUUID = CBUUID(string: "63802432-69FC-406D-A538-FE33CEF32AEF")
    private let commandUUID = CBUUID(string: "DE0D7201-CC2B-46D9-8F92-564A209C37EF")
    private let eventsUUID = CBUUID(string: "456F5CDA-632A-4541-A2A5-6FAEC234075E")
    private let statusUUID = CBUUID(string: "0AF05FBF-ADAA-4D94-8DCF-44A62F82332B")
    private let savedTagsKey = "rfid-tools.saved-tags.v1"

    private var central: CBCentralManager?
    private var discoveredPeripherals: [UUID: CBPeripheral] = [:]
    private var connectedPeripheral: CBPeripheral?
    private var commandCharacteristic: CBCharacteristic?
    private var eventsCharacteristic: CBCharacteristic?
    private var statusCharacteristic: CBCharacteristic?
    private var nextCommandID = 1

    override init() {
        super.init()
        loadSavedTags()
        central = CBCentralManager(delegate: self, queue: nil)
    }

    func startScan() {
        isAutoConnectEnabled = true
        beginScan(connectionState: .scanning, message: "Looking for RFID Tools ESP32")
    }

    func rescan() {
        beginScan(connectionState: .scanning, message: "Looking for RFID Tools ESP32")
    }

    private func beginScan(connectionState nextState: ReaderConnectionState, message: String) {
        guard central?.state == .poweredOn else {
            connectionState = .bluetoothUnavailable
            statusSummary = "Turn on Bluetooth to connect to the reader"
            log(.error, "Bluetooth is not powered on")
            return
        }

        peripherals.removeAll()
        discoveredPeripherals.removeAll()
        connectionState = nextState
        statusSummary = message
        log(.app, message)
        central?.scanForPeripherals(withServices: [serviceUUID], options: [CBCentralManagerScanOptionAllowDuplicatesKey: false])
    }

    func stopScan() {
        isAutoConnectEnabled = false
        central?.stopScan()
        if case .scanning = connectionState {
            connectionState = .disconnected
        } else if case .reconnecting = connectionState {
            connectionState = .disconnected
        }
        statusSummary = "Auto-connect is off"
        log(.app, "Scan stopped")
    }

    func connect(to peripheral: ReaderPeripheral) {
        guard let cbPeripheral = discoveredPeripherals[peripheral.id] else {
            log(.error, "Peripheral is no longer available")
            return
        }

        central?.stopScan()
        connectionState = .connecting(peripheral.name)
        statusSummary = "Connecting to \(peripheral.name)"
        log(.app, "Connecting to \(peripheral.name)")
        connectedPeripheral = cbPeripheral
        cbPeripheral.delegate = self
        central?.connect(cbPeripheral)
    }

    func disconnect() {
        isAutoConnectEnabled = false
        isInventoryRunning = false
        guard let connectedPeripheral else {
            connectionState = .disconnected
            statusSummary = "Disconnected"
            return
        }

        log(.app, "Disconnect requested")
        central?.cancelPeripheralConnection(connectedPeripheral)
    }

    func startInventory() {
        tags.removeAll()
        latestTag = nil
        isInventoryRunning = true
        statusSummary = "Reading tag"
        send(.startInventory)
    }

    func stopInventory() {
        isInventoryRunning = false
        statusSummary = latestTag == nil ? "Scan stopped" : "Scan stopped with \(tags.count) tag\(tags.count == 1 ? "" : "s") found"
        send(.stopInventory)
    }

    func applyPower() {
        sendCommand(.setPower, values: ["dbm": selectedPowerDbm])
        statusSummary = "Setting reader power to \(selectedPowerDbm) dBm"
    }

    func saveLatestTag(named name: String) {
        let trimmed = name.trimmingCharacters(in: .whitespacesAndNewlines)
        guard let latestTag else {
            statusSummary = "Read a tag before saving"
            return
        }
        guard !trimmed.isEmpty else {
            statusSummary = "Enter a name for this tag"
            return
        }

        let savedTag = SavedTag(name: trimmed, epc: latestTag.epc)
        savedTags.insert(savedTag, at: 0)
        persistSavedTags()
        statusSummary = "Saved \(trimmed)"
    }

    func deleteSavedTags(at offsets: IndexSet) {
        savedTags.remove(atOffsets: offsets)
        persistSavedTags()
    }

    func deleteSavedTag(_ savedTag: SavedTag) {
        savedTags.removeAll { $0.id == savedTag.id }
        persistSavedTags()
    }

    func applySavedTag(_ savedTag: SavedTag?) {
        guard let savedTag else {
            lastWriteMessage = "Choose a saved tag first"
            return
        }
        requestTagWrite(epc: savedTag.epc, label: savedTag.name)
    }

    func writeText(_ value: String) {
        let trimmed = value.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !trimmed.isEmpty else {
            lastWriteMessage = "Type the text to write"
            return
        }
        requestTagWrite(epc: trimmed.utf8HexEncoded.replacingOccurrences(of: " ", with: ""), label: trimmed)
    }

    func send(_ command: RFIDCommand) {
        sendCommand(command)
    }

    private func sendCommand(_ command: RFIDCommand, values: [String: Any] = [:]) {
        var payload: [String: Any] = [
            "v": 1,
            "id": nextCommandID,
            "cmd": command.rawValue
        ]

        switch command {
        case .setPower:
            payload["dbm"] = values["dbm"] ?? selectedPowerDbm
        case .setRegion:
            payload["region"] = "US"
        case .writeEpc:
            payload["epc"] = values["epc"] ?? ""
        case .getInfo, .status, .getPower, .getRegion, .startInventory, .stopInventory:
            break
        }

        nextCommandID += 1
        send(payload)
    }

    private func requestTagWrite(epc: String, label: String) {
        guard commandCharacteristic != nil, connectedPeripheral != nil else {
            lastWriteMessage = "Connect to the reader before writing"
            return
        }

        if isInventoryRunning {
            lastWriteMessage = "Stop reading before writing"
            return
        }

        sendCommand(.writeEpc, values: ["epc": epc])
        lastWriteMessage = "Writing \(label)..."
    }

    private func send(_ payload: [String: Any]) {
        guard let connectedPeripheral, let commandCharacteristic else {
            statusSummary = "Reader is not ready yet"
            log(.error, "Command characteristic is not ready")
            return
        }

        do {
            let data = try JSONSerialization.data(withJSONObject: payload, options: [.sortedKeys])
            connectedPeripheral.writeValue(data, for: commandCharacteristic, type: .withResponse)
            if let command = payload["cmd"] as? String {
                log(.app, "Sent \(command)")
            }
        } catch {
            statusSummary = "Could not send command"
            log(.error, "Failed to encode command: \(error.localizedDescription)")
        }
    }

    private func handleEventData(_ data: Data) {
        guard let text = String(data: data, encoding: .utf8) else {
            log(.error, "Received non-UTF8 event payload")
            return
        }

        guard let object = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
              let event = object["evt"] as? String else {
            log(.firmware, text)
            return
        }

        if event == "info" {
            statusSummary = "Reader ready"
            log(.firmware, "Reader info received")
        } else if event == "status" {
            updateStatusSummary(from: object)
        } else if event == "tagSeen", let epc = object["epc"] as? String {
            upsertTag(from: object, epc: epc)
        } else if event == "commandAck" {
            log(.firmware, "Command acknowledged")
        } else if event == "result" {
            handleCommandResult(object)
        } else if event == "scanStopped" {
            isInventoryRunning = false
            if latestTag == nil {
                statusSummary = "Read complete"
            }
        } else if event == "error" {
            let message = object["message"] as? String ?? "Reader reported an error"
            isInventoryRunning = false
            statusSummary = message
            lastWriteMessage = message
            log(.error, message)
        }
    }

    private func handleCommandResult(_ object: [String: Any]) {
        guard let command = object["cmd"] as? String else {
            return
        }

        if command == RFIDCommand.writeEpc.rawValue {
            let epc = object["epc"] as? String
            if let epc {
                latestTag = TagRead(id: epc, epc: epc, pc: nil, rssi: nil, crc: nil, seenCount: 1, updatedAt: Date())
            }
            statusSummary = "Tag written"
            lastWriteMessage = "Tag written"
        } else if command == RFIDCommand.setPower.rawValue {
            statusSummary = "Power set to \(selectedPowerDbm) dBm"
        }
        log(.firmware, "\(command) completed")
    }

    private func updateStatusSummary(from object: [String: Any]) {
        let connected = object["yrm100Connected"] as? Bool
        let inventory = object["inventoryRunning"] as? Bool
        if let inventory {
            isInventoryRunning = inventory
        }

        switch (connected, inventory) {
        case (.some(true), .some(true)):
            statusSummary = "Reader connected and scanning"
        case (.some(true), _):
            statusSummary = "Reader connected"
        case (.some(false), _):
            statusSummary = "RFID module is not responding"
        default:
            statusSummary = "Reader status updated"
        }
    }

    private func upsertTag(from object: [String: Any], epc: String) {
        let pc = object["pc"] as? String
        let rssi = object["rssi"] as? Int
        let crc = object["crc"] as? String

        if let index = tags.firstIndex(where: { $0.epc == epc }) {
            var updatedTag = tags.remove(at: index)
            updatedTag.pc = pc ?? updatedTag.pc
            updatedTag.rssi = rssi ?? updatedTag.rssi
            updatedTag.crc = crc ?? updatedTag.crc
            updatedTag.seenCount += 1
            updatedTag.updatedAt = Date()
            tags.insert(updatedTag, at: 0)
        } else {
            tags.insert(
                TagRead(id: epc, epc: epc, pc: pc, rssi: rssi, crc: crc, seenCount: 1, updatedAt: Date()),
                at: 0
            )
        }
        latestTag = tags.first(where: { $0.epc == epc })
        isInventoryRunning = false
        statusSummary = "Read \(tags.count) unique tag\(tags.count == 1 ? "" : "s")"
    }

    private func resetConnectionState() {
        connectedPeripheral = nil
        commandCharacteristic = nil
        eventsCharacteristic = nil
        statusCharacteristic = nil
    }

    private func reconnectSoon() {
        guard isAutoConnectEnabled else {
            return
        }

        connectionState = .reconnecting
        statusSummary = "Connection lost. Reconnecting..."
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) { [weak self] in
            Task { @MainActor in
                self?.beginScan(connectionState: .reconnecting, message: "Reconnecting to RFID Tools ESP32")
            }
        }
    }

    private func loadSavedTags() {
        guard let data = UserDefaults.standard.data(forKey: savedTagsKey),
              let decoded = try? JSONDecoder().decode([SavedTag].self, from: data) else {
            return
        }
        savedTags = decoded
    }

    private func persistSavedTags() {
        guard let data = try? JSONEncoder().encode(savedTags) else {
            return
        }
        UserDefaults.standard.set(data, forKey: savedTagsKey)
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
                if isAutoConnectEnabled {
                    beginScan(connectionState: .scanning, message: "Looking for RFID Tools ESP32")
                }
            case .poweredOff:
                connectionState = .bluetoothUnavailable
                statusSummary = "Bluetooth is off"
                log(.ble, "Bluetooth powered off")
            case .unauthorized:
                connectionState = .error("Bluetooth permission denied")
                statusSummary = "Bluetooth permission denied"
                log(.error, "Bluetooth permission denied")
            case .unsupported:
                connectionState = .error("Bluetooth unsupported")
                statusSummary = "Bluetooth is not supported on this device"
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

            if isAutoConnectEnabled {
                connect(to: item)
            }
        }
    }

    nonisolated func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        Task { @MainActor in
            connectionState = .connected(peripheral.name ?? "RFID reader")
            statusSummary = "Connected. Preparing reader..."
            log(.ble, "Connected")
            peripheral.discoverServices([serviceUUID])
        }
    }

    nonisolated func centralManager(_ central: CBCentralManager, didFailToConnect peripheral: CBPeripheral, error: Error?) {
        Task { @MainActor in
            resetConnectionState()
            connectionState = .error(error?.localizedDescription ?? "Failed to connect")
            statusSummary = "Could not connect. Retrying..."
            log(.error, "Connection failed: \(error?.localizedDescription ?? "unknown error")")
            reconnectSoon()
        }
    }

    nonisolated func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        Task { @MainActor in
            resetConnectionState()
            isInventoryRunning = false
            connectionState = .disconnected
            statusSummary = error == nil ? "Disconnected" : "Connection lost"
            log(.ble, "Disconnected\(error.map { ": \($0.localizedDescription)" } ?? "")")
            reconnectSoon()
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
                applyPower()
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
