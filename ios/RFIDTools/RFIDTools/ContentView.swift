import SwiftUI
import UIKit

struct ContentView: View {
    @EnvironmentObject private var bleManager: BLEManager
    @State private var tagName = ""
    @State private var customText = ""
    @State private var tagDisplayFormat: TagDisplayFormat = .hex
    @State private var selectedSavedTagID: UUID?
    @State private var isShowingSettings = false
    @State private var copiedTagID: String?

    private var selectedSavedTag: SavedTag? {
        guard let selectedSavedTagID else {
            return bleManager.savedTags.first
        }
        return bleManager.savedTags.first { $0.id == selectedSavedTagID }
    }

    var body: some View {
        GeometryReader { proxy in
            VStack(spacing: 0) {
                header

                ScrollView {
                    VStack(alignment: .leading, spacing: 10) {
                        statusSection
                        readSection
                        saveSection
                        writeSection
                        savedTagsSection
                    }
                    .frame(maxWidth: .infinity, alignment: .leading)
                    .padding(.horizontal, 12)
                    .padding(.vertical, 10)
                    .frame(minHeight: proxy.size.height - proxy.safeAreaInsets.top - proxy.safeAreaInsets.bottom, alignment: .top)
                }
            }
            .frame(width: proxy.size.width, height: proxy.size.height, alignment: .top)
            .background(Color(.systemGroupedBackground))
        }
        .ignoresSafeArea(.keyboard, edges: .bottom)
        .dynamicTypeSize(.medium)
        .sheet(isPresented: $isShowingSettings) {
            NavigationStack {
                SettingsView()
                    .environmentObject(bleManager)
            }
        }
    }

    private var header: some View {
        HStack(spacing: 12) {
            Text("RFID Tools")
                .font(.title3.weight(.semibold))
                .lineLimit(1)

            Spacer()

            Button {
                isShowingSettings = true
            } label: {
                Image(systemName: "gearshape")
                    .font(.body.weight(.semibold))
                    .frame(width: 36, height: 36)
                    .background(Color(.secondarySystemGroupedBackground))
                    .clipShape(Circle())
            }
            .buttonStyle(.plain)
            .accessibilityLabel("Settings")
        }
        .padding(.horizontal, 14)
        .padding(.top, 8)
        .padding(.bottom, 8)
        .background(Color(.systemGroupedBackground))
    }

    private var statusSection: some View {
        HStack(spacing: 8) {
            Label(connectionStatusText(for: bleManager.connectionState), systemImage: connectionIcon(for: bleManager.connectionState))
                .font(.footnote.weight(.semibold))
                .foregroundStyle(connectionColor(for: bleManager.connectionState))
                .lineLimit(1)

            Spacer()

            if bleManager.isInventoryRunning {
                Text("Reading")
                    .font(.caption.weight(.semibold))
                    .foregroundStyle(.secondary)
            }
        }
        .sectionStyle()
    }

    private var readSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Label("Read Tag", systemImage: "tag")
                    .font(.footnote.weight(.semibold))
                Spacer()
                Picker("Format", selection: $tagDisplayFormat) {
                    ForEach(TagDisplayFormat.allCases) { format in
                        Text(format.rawValue).tag(format)
                    }
                }
                .pickerStyle(.segmented)
                .frame(maxWidth: 140)
            }

            detectedTagsView

            HStack(spacing: 10) {
                Spacer()

                Button {
                    bleManager.startInventory()
                } label: {
                    Label("Read", systemImage: "dot.radiowaves.left.and.right")
                        .font(.subheadline.weight(.semibold))
                        .frame(maxWidth: .infinity)
                }
                .buttonStyle(.borderedProminent)
                .controlSize(.small)
            }
            .lineLimit(1)
            .minimumScaleFactor(0.8)
        }
        .sectionStyle()
    }

    @ViewBuilder
    private var detectedTagsView: some View {
        if bleManager.tags.isEmpty {
            Text("No tag read yet")
                .font(.footnote)
                .foregroundStyle(.secondary)
                .frame(maxWidth: .infinity, alignment: .leading)
        } else {
            VStack(alignment: .leading, spacing: 8) {
                ForEach(bleManager.tags.prefix(8)) { tag in
                    VStack(alignment: .leading, spacing: 5) {
                        HStack(alignment: .top, spacing: 8) {
                            Text(tag.displayValue(format: tagDisplayFormat))
                                .font(.system(.subheadline, design: tagDisplayFormat == .hex ? .monospaced : .default))
                                .lineLimit(2)
                                .minimumScaleFactor(0.8)
                                .textSelection(.enabled)
                                .frame(maxWidth: .infinity, alignment: .leading)

                            Button {
                                copyTag(tag)
                            } label: {
                                Image(systemName: copiedTagID == tag.id ? "checkmark" : "doc.on.doc")
                                    .font(.caption.weight(.semibold))
                                    .frame(width: 30, height: 30)
                            }
                            .buttonStyle(.borderless)
                            .accessibilityLabel("Copy tag")
                        }

                        HStack(spacing: 12) {
                            Text("Seen \(tag.seenCount)")
                            if let rssi = tag.rssi {
                                Text("RSSI \(rssi)")
                            }
                            Text(tag.updatedAt, style: .time)
                        }
                        .font(.caption)
                        .foregroundStyle(.secondary)
                    }

                    if tag.id != bleManager.tags.prefix(8).last?.id {
                        Divider()
                    }
                }
            }
            .padding(10)
            .frame(maxWidth: .infinity, alignment: .leading)
            .background(Color(.secondarySystemGroupedBackground))
            .clipShape(RoundedRectangle(cornerRadius: 8))
        }
    }

    private func copyTag(_ tag: TagRead) {
        UIPasteboard.general.string = tag.epc
        copiedTagID = tag.id

        DispatchQueue.main.asyncAfter(deadline: .now() + 1.2) {
            if copiedTagID == tag.id {
                copiedTagID = nil
            }
        }
    }

    private var saveSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            Label("Save Current Tag", systemImage: "tray.and.arrow.down")
                .font(.footnote.weight(.semibold))

            TextField("Tag name", text: $tagName)
                .textInputAutocapitalization(.words)
                .textFieldStyle(.roundedBorder)

            Button {
                bleManager.saveLatestTag(named: tagName)
                tagName = ""
            } label: {
                Label("Save Tag", systemImage: "plus.circle")
                    .font(.subheadline.weight(.semibold))
                    .frame(maxWidth: .infinity)
            }
            .buttonStyle(.borderedProminent)
            .controlSize(.small)
            .disabled(bleManager.latestTag == nil)
        }
        .sectionStyle()
    }

    private var writeSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            Label("Write Tag", systemImage: "square.and.pencil")
                .font(.footnote.weight(.semibold))

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
                        .font(.subheadline.weight(.semibold))
                        .frame(maxWidth: .infinity)
                }
                .buttonStyle(.borderedProminent)
                .controlSize(.small)
            }

            TextField("Text to write", text: $customText)
                .textInputAutocapitalization(.never)
                .autocorrectionDisabled()
                .textFieldStyle(.roundedBorder)

            Button {
                bleManager.writeText(customText)
            } label: {
                Label("Write Text", systemImage: "text.cursor")
                    .font(.subheadline.weight(.semibold))
                    .frame(maxWidth: .infinity)
            }
            .buttonStyle(.bordered)
            .controlSize(.small)

            Text(bleManager.lastWriteMessage)
                .font(.caption)
                .foregroundStyle(.secondary)
        }
        .sectionStyle()
    }

    private var savedTagsSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            Label("Saved Tags", systemImage: "bookmark")
                .font(.footnote.weight(.semibold))

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

}

private struct SettingsView: View {
    @Environment(\.dismiss) private var dismiss
    @EnvironmentObject private var bleManager: BLEManager
    @State private var isShowingDiagnostics = false

    var body: some View {
        List {
            Section {
                HStack {
                    Label(connectionStatusText(for: bleManager.connectionState), systemImage: connectionIcon(for: bleManager.connectionState))
                        .foregroundStyle(connectionColor(for: bleManager.connectionState))
                    Spacer()
                }

                Text(bleManager.statusSummary)
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }

            Section {
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

                HStack(spacing: 10) {
                    Button {
                        bleManager.rescan()
                    } label: {
                        Label("Find", systemImage: "antenna.radiowaves.left.and.right")
                            .font(.subheadline.weight(.semibold))
                            .frame(maxWidth: .infinity)
                    }
                    .buttonStyle(.bordered)
                    .controlSize(.small)

                    Button {
                        bleManager.disconnect()
                    } label: {
                        Label("Disconnect", systemImage: "xmark.circle")
                            .font(.subheadline.weight(.semibold))
                            .frame(maxWidth: .infinity)
                    }
                    .buttonStyle(.bordered)
                    .controlSize(.small)
                }
                .listRowInsets(EdgeInsets(top: 8, leading: 16, bottom: 8, trailing: 16))
            }

            Section {
                HStack {
                    Label("Power", systemImage: "bolt.fill")
                    Spacer()
                    Text("\(bleManager.selectedPowerDbm) dBm")
                        .font(.body.weight(.semibold).monospacedDigit())
                }

                HStack(spacing: 10) {
                    Slider(
                        value: Binding(
                            get: { Double(bleManager.selectedPowerDbm) },
                            set: { bleManager.selectedPowerDbm = Int($0.rounded()) }
                        ),
                        in: 0...26,
                        step: 1
                    )

                    Button {
                        bleManager.applyPower()
                    } label: {
                        Image(systemName: "checkmark")
                            .frame(width: 28, height: 28)
                    }
                    .buttonStyle(.borderedProminent)
                    .accessibilityLabel("Set Power")
                }
            }

            Section {
                Button {
                    isShowingDiagnostics = true
                } label: {
                    Label("Diagnostics", systemImage: "waveform.path.ecg")
                }
            }
        }
        .navigationTitle("Settings")
        .toolbar {
            ToolbarItem(placement: .topBarTrailing) {
                Button("Done") {
                    dismiss()
                }
            }
        }
        .sheet(isPresented: $isShowingDiagnostics) {
            NavigationStack {
                DiagnosticsView()
                    .environmentObject(bleManager)
            }
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

private func connectionStatusText(for state: ReaderConnectionState) -> String {
    switch state {
    case .bluetoothUnavailable:
        return "Bluetooth off"
    case .disconnected:
        return "Disconnected"
    case .reconnecting:
        return "Disconnected"
    case .scanning:
        return "Disconnected"
    case .connecting:
        return "Connecting"
    case .connected:
        return "Connected"
    case .error:
        return "Error"
    }
}

private func connectionIcon(for state: ReaderConnectionState) -> String {
    switch state {
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

private func connectionColor(for state: ReaderConnectionState) -> Color {
    switch state {
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

private extension View {
    func sectionStyle() -> some View {
        self
            .padding(10)
            .frame(maxWidth: .infinity, alignment: .leading)
            .background(Color(.systemBackground))
            .clipShape(RoundedRectangle(cornerRadius: 8))
    }
}
