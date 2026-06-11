import SwiftUI

struct ContentView: View {
    @EnvironmentObject private var bleManager: BLEManager

    var body: some View {
        NavigationStack {
            List {
                Section("Reader") {
                    Text(bleManager.connectionState.label)
                        .font(.headline)

                    HStack {
                        Button("Scan") {
                            bleManager.startScan()
                        }
                        .buttonStyle(.borderedProminent)

                        Button("Stop") {
                            bleManager.stopScan()
                        }
                        .buttonStyle(.bordered)

                        Button("Disconnect") {
                            bleManager.disconnect()
                        }
                        .buttonStyle(.bordered)
                    }

                    Text(bleManager.statusSummary)
                        .font(.caption)
                        .foregroundStyle(.secondary)
                        .textSelection(.enabled)
                }

                if !bleManager.peripherals.isEmpty {
                    Section("Peripherals") {
                        ForEach(bleManager.peripherals) { peripheral in
                            Button {
                                bleManager.connect(to: peripheral)
                            } label: {
                                HStack {
                                    VStack(alignment: .leading) {
                                        Text(peripheral.name)
                                        Text(peripheral.id.uuidString)
                                            .font(.caption2)
                                            .foregroundStyle(.secondary)
                                    }
                                    Spacer()
                                    Text("\(peripheral.rssi)")
                                        .font(.caption)
                                        .foregroundStyle(.secondary)
                                }
                            }
                        }
                    }
                }

                Section("Commands") {
                    LazyVGrid(columns: [GridItem(.adaptive(minimum: 132), spacing: 8)], spacing: 8) {
                        ForEach(RFIDCommand.allCases) { command in
                            Button(command.rawValue) {
                                bleManager.send(command)
                            }
                            .buttonStyle(.bordered)
                        }
                    }
                    .padding(.vertical, 4)
                }

                Section("Live Tags") {
                    if bleManager.tags.isEmpty {
                        Text("No tags")
                            .foregroundStyle(.secondary)
                    } else {
                        ForEach(bleManager.tags) { tag in
                            VStack(alignment: .leading, spacing: 4) {
                                Text(tag.epc)
                                    .font(.system(.body, design: .monospaced))
                                    .textSelection(.enabled)
                                HStack {
                                    Text("Seen \(tag.seenCount)")
                                    if let rssi = tag.rssi {
                                        Text("RSSI \(rssi)")
                                    }
                                    if let pc = tag.pc {
                                        Text("PC \(pc)")
                                    }
                                }
                                .font(.caption)
                                .foregroundStyle(.secondary)
                            }
                        }
                    }
                }

                Section("Diagnostics") {
                    ForEach(bleManager.diagnostics) { entry in
                        VStack(alignment: .leading, spacing: 3) {
                            Text("\(entry.prefix) \(entry.timestamp.formatted(date: .omitted, time: .standard))")
                                .font(.caption2)
                                .foregroundStyle(.secondary)
                            Text(entry.message)
                                .font(.system(.caption, design: .monospaced))
                                .textSelection(.enabled)
                        }
                        .padding(.vertical, 2)
                    }
                }
            }
            .navigationTitle("RFID Tools")
        }
    }
}
