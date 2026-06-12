import SwiftUI

struct ContentView: View {
    @EnvironmentObject private var bleManager: BLEManager
    @State private var tagName = ""
    @State private var customText = ""
    @State private var tagDisplayFormat: TagDisplayFormat = .hex
    @State private var selectedSavedTagID: UUID?

    private var selectedSavedTag: SavedTag? {
        guard let selectedSavedTagID else {
            return bleManager.savedTags.first
        }
        return bleManager.savedTags.first { $0.id == selectedSavedTagID }
    }

    var body: some View {
        NavigationStack {
            ScrollView {
                VStack(alignment: .leading, spacing: 14) {
                    connectionSection
                    powerSection
                    readSection
                    saveSection
                    writeSection
                    savedTagsSection
                }
                .frame(maxWidth: .infinity, alignment: .leading)
                .padding(.horizontal, 14)
                .padding(.vertical, 12)
            }
            .background(Color(.systemGroupedBackground))
            .navigationTitle("RFID Tools")
            .navigationBarTitleDisplayMode(.inline)
            .dynamicTypeSize(.small ... .large)
            .toolbar {
                ToolbarItem(placement: .topBarTrailing) {
                    NavigationLink {
                        DiagnosticsView()
                            .environmentObject(bleManager)
                    } label: {
                        Image(systemName: "waveform.path.ecg")
                    }
                    .accessibilityLabel("Diagnostics")
                }
            }
        }
    }

    private var connectionSection: some View {
        VStack(alignment: .leading, spacing: 10) {
            Label(bleManager.connectionState.label, systemImage: connectionIcon)
                .font(.subheadline.weight(.semibold))
                .foregroundStyle(connectionColor)
                .lineLimit(1)
                .minimumScaleFactor(0.8)

            Text(bleManager.statusSummary)
                .font(.footnote)
                .foregroundStyle(.secondary)
                .lineLimit(2)

            Toggle("Auto-connect", isOn: Binding(
                get: { bleManager.isAutoConnectEnabled },
                set: { enabled in
                    if enabled {
                        bleManager.isAutoConnectEnabled = true
                        bleManager.rescan()
                    } else {
                        bleManager.stopScan()
                    }
                }
            ))
            .font(.subheadline)

            HStack(spacing: 10) {
                Button {
                    bleManager.rescan()
                } label: {
                    Label("Find", systemImage: "antenna.radiowaves.left.and.right")
                        .frame(maxWidth: .infinity)
                }
                .buttonStyle(.bordered)
                .controlSize(.regular)

                Button {
                    bleManager.disconnect()
                } label: {
                    Label("Disconnect", systemImage: "xmark.circle")
                        .frame(maxWidth: .infinity)
                }
                .buttonStyle(.bordered)
                .controlSize(.regular)
            }
            .labelStyle(.titleAndIcon)
            .lineLimit(1)
            .minimumScaleFactor(0.75)
        }
        .sectionStyle()
    }

    private var powerSection: some View {
        VStack(alignment: .leading, spacing: 10) {
            HStack {
                Label("Power", systemImage: "bolt.fill")
                    .font(.subheadline.weight(.semibold))
                Spacer()
                Text("\(bleManager.selectedPowerDbm) dBm")
                    .font(.subheadline.weight(.semibold).monospacedDigit())
            }

            Slider(
                value: Binding(
                    get: { Double(bleManager.selectedPowerDbm) },
                    set: { bleManager.selectedPowerDbm = Int($0.rounded()) }
                ),
                in: 15...26,
                step: 1
            )

            Button {
                bleManager.applyPower()
            } label: {
                Label("Set Power", systemImage: "checkmark.circle")
                    .frame(maxWidth: .infinity)
            }
            .buttonStyle(.borderedProminent)
            .controlSize(.regular)
        }
        .sectionStyle()
    }

    private var readSection: some View {
        VStack(alignment: .leading, spacing: 10) {
            HStack {
                Label("Read Tag", systemImage: "tag")
                    .font(.subheadline.weight(.semibold))
                Spacer()
                Picker("Format", selection: $tagDisplayFormat) {
                    ForEach(TagDisplayFormat.allCases) { format in
                        Text(format.rawValue).tag(format)
                    }
                }
                .pickerStyle(.segmented)
                .frame(maxWidth: 180)
            }

            latestTagView

            HStack(spacing: 10) {
                Button {
                    bleManager.startInventory()
                } label: {
                    Label("Read", systemImage: "dot.radiowaves.left.and.right")
                        .frame(maxWidth: .infinity)
                }
                .buttonStyle(.borderedProminent)

                Button {
                    bleManager.stopInventory()
                } label: {
                    Label("Stop", systemImage: "stop.fill")
                        .frame(maxWidth: .infinity)
                }
                .buttonStyle(.bordered)
            }
            .lineLimit(1)
            .minimumScaleFactor(0.8)
        }
        .sectionStyle()
    }

    @ViewBuilder
    private var latestTagView: some View {
        if let latestTag = bleManager.latestTag {
            VStack(alignment: .leading, spacing: 8) {
                Text(latestTag.displayValue(format: tagDisplayFormat))
                    .font(.system(.subheadline, design: tagDisplayFormat == .hex ? .monospaced : .default))
                    .textSelection(.enabled)
                    .frame(maxWidth: .infinity, alignment: .leading)

                HStack(spacing: 14) {
                    Text("Seen \(latestTag.seenCount)")
                    if let rssi = latestTag.rssi {
                        Text("RSSI \(rssi)")
                    }
                    Text(latestTag.updatedAt, style: .time)
                }
                .font(.caption)
                .foregroundStyle(.secondary)
            }
            .padding(10)
            .frame(maxWidth: .infinity, alignment: .leading)
            .background(Color(.secondarySystemGroupedBackground))
            .clipShape(RoundedRectangle(cornerRadius: 8))
        } else {
            Text("No tag read yet")
                .font(.footnote)
                .foregroundStyle(.secondary)
                .frame(maxWidth: .infinity, alignment: .leading)
        }
    }

    private var saveSection: some View {
        VStack(alignment: .leading, spacing: 10) {
            Label("Save Current Tag", systemImage: "tray.and.arrow.down")
                .font(.subheadline.weight(.semibold))

            TextField("Tag name", text: $tagName)
                .textInputAutocapitalization(.words)
                .textFieldStyle(.roundedBorder)

            Button {
                bleManager.saveLatestTag(named: tagName)
                tagName = ""
            } label: {
                Label("Save Tag", systemImage: "plus.circle")
                    .frame(maxWidth: .infinity)
            }
            .buttonStyle(.borderedProminent)
            .controlSize(.regular)
            .disabled(bleManager.latestTag == nil)
        }
        .sectionStyle()
    }

    private var writeSection: some View {
        VStack(alignment: .leading, spacing: 10) {
            Label("Write Tag", systemImage: "square.and.pencil")
                .font(.subheadline.weight(.semibold))

            if bleManager.savedTags.isEmpty {
                Text("Saved tags will appear here after you read and save one.")
                    .font(.footnote)
                    .foregroundStyle(.secondary)
            } else {
                Picker("Saved tag", selection: Binding(
                    get: { selectedSavedTagID ?? bleManager.savedTags.first?.id },
                    set: { selectedSavedTagID = $0 }
                )) {
                    ForEach(bleManager.savedTags) { tag in
                        Text(tag.name).tag(Optional(tag.id))
                    }
                }
                .pickerStyle(.menu)

                Button {
                    bleManager.applySavedTag(selectedSavedTag)
                } label: {
                    Label("Apply Saved Tag", systemImage: "arrow.down.doc")
                        .frame(maxWidth: .infinity)
                }
                .buttonStyle(.borderedProminent)
            }

            TextField("Text to write", text: $customText)
                .textInputAutocapitalization(.never)
                .autocorrectionDisabled()
                .textFieldStyle(.roundedBorder)

            Button {
                bleManager.writeText(customText)
            } label: {
                Label("Write Text", systemImage: "text.cursor")
                    .frame(maxWidth: .infinity)
            }
            .buttonStyle(.bordered)

            Text(bleManager.lastWriteMessage)
                .font(.caption)
                .foregroundStyle(.secondary)
        }
        .sectionStyle()
    }

    private var savedTagsSection: some View {
        VStack(alignment: .leading, spacing: 10) {
            Label("Saved Tags", systemImage: "bookmark")
                .font(.subheadline.weight(.semibold))

            if bleManager.savedTags.isEmpty {
                Text("No saved tags")
                    .font(.footnote)
                    .foregroundStyle(.secondary)
            } else {
                ForEach(bleManager.savedTags) { tag in
                    HStack(alignment: .top, spacing: 12) {
                        VStack(alignment: .leading, spacing: 5) {
                            Text(tag.name)
                                .font(.body.weight(.semibold))
                            Text(tag.epc)
                                .font(.system(.caption, design: .monospaced))
                                .foregroundStyle(.secondary)
                                .textSelection(.enabled)
                        }
                        Spacer()
                        Button {
                            bleManager.deleteSavedTag(tag)
                        } label: {
                            Image(systemName: "trash")
                        }
                        .buttonStyle(.borderless)
                        .foregroundStyle(.red)
                        .accessibilityLabel("Delete \(tag.name)")
                    }
                    .padding(.vertical, 8)
                    Divider()
                }
            }
        }
        .sectionStyle()
    }

    private var connectionIcon: String {
        switch bleManager.connectionState {
        case .connected:
            return "checkmark.circle.fill"
        case .scanning, .connecting, .reconnecting:
            return "arrow.triangle.2.circlepath"
        case .bluetoothUnavailable, .error:
            return "exclamationmark.triangle.fill"
        case .disconnected:
            return "circle"
        }
    }

    private var connectionColor: Color {
        switch bleManager.connectionState {
        case .connected:
            return .green
        case .bluetoothUnavailable, .error:
            return .red
        case .scanning, .connecting, .reconnecting:
            return .orange
        case .disconnected:
            return .secondary
        }
    }
}

private struct DiagnosticsView: View {
    @EnvironmentObject private var bleManager: BLEManager

    var body: some View {
        List {
            ForEach(bleManager.diagnostics) { entry in
                VStack(alignment: .leading, spacing: 4) {
                    Text("\(entry.prefix) \(entry.timestamp.formatted(date: .omitted, time: .standard))")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                    Text(entry.message)
                        .font(.callout)
                        .textSelection(.enabled)
                }
            }
        }
        .navigationTitle("Diagnostics")
    }
}

private extension View {
    func sectionStyle() -> some View {
        self
            .padding(12)
            .frame(maxWidth: .infinity, alignment: .leading)
            .background(Color(.systemBackground))
            .clipShape(RoundedRectangle(cornerRadius: 8))
    }
}
