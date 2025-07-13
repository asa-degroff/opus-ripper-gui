import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import OpusRipperGUI 1.0
import "components"

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 900
    height: 600
    minimumWidth: 700
    minimumHeight: 500
    title: qsTr("Opus Ripper GUI")
    color: Style.backgroundColor
    
    property ConversionController conversionController: ConversionController {}
    
    header: ToolBar {
        height: Style.toolbarHeight
        background: Rectangle {
            color: Style.surfaceColor
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Style.mediumSpacing
            anchors.rightMargin: Style.mediumSpacing
            
            Label {
                text: qsTr("Opus Ripper")
                font.pixelSize: Style.largeFontSize
                font.bold: true
                color: Style.textPrimary
            }
            
            Item { Layout.fillWidth: true }
            
            Button {
                text: qsTr("Settings")
                icon.name: "preferences-system"
                onClicked: settingsPanel.open()
            }
        }
    }
    
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Style.largeSpacing
        spacing: Style.mediumSpacing
        
        // Input/Output directory selection
        StyledGroupBox {
            title: qsTr("Directories")
            Layout.fillWidth: true
            
            ColumnLayout {
                anchors.fill: parent
                spacing: Style.mediumSpacing
                
                DirectoryPicker {
                    id: inputDirPicker
                    Layout.fillWidth: true
                    label: qsTr("Input Directory (FLAC files)")
                    placeholderText: qsTr("Select directory containing FLAC files...")
                    onDirectorySelected: function(directory) {
                        conversionController.setInputDirectory(directory)
                    }
                }
                
                DirectoryPicker {
                    id: outputDirPicker
                    Layout.fillWidth: true
                    label: qsTr("Output Directory (Opus files)")
                    placeholderText: qsTr("Select output directory...")
                    onDirectorySelected: function(directory) {
                        conversionController.setOutputDirectory(directory)
                    }
                }
            }
        }
        
        // Progress and file information
        StyledGroupBox {
            title: qsTr("Conversion Progress")
            Layout.fillWidth: true
            Layout.fillHeight: true
            
            ProgressView {
                id: progressView
                anchors.fill: parent
                model: conversionController.progressModel
            }
        }
        
        // Action buttons
        RowLayout {
            Layout.fillWidth: true
            spacing: Style.mediumSpacing
            
            Button {
                id: scanButton
                text: qsTr("Scan for Files")
                enabled: inputDirPicker.directory !== "" && !conversionController.isScanning
                onClicked: conversionController.scanForFiles()
                
                Layout.preferredWidth: 150
            }
            
            Item { Layout.fillWidth: true }
            
            Button {
                id: convertButton
                text: conversionController.isConverting ? qsTr("Stop") : qsTr("Start Conversion")
                enabled: conversionController.filesFound > 0
                highlighted: true
                
                onClicked: {
                    if (conversionController.isConverting) {
                        conversionController.stopConversion()
                    } else {
                        conversionController.startConversion()
                    }
                }
                
                Layout.preferredWidth: 150
            }
        }
    }
    
    // Settings panel
    SettingsPanel {
        id: settingsPanel
        controller: conversionController
    }
    
    // Status bar
    footer: ToolBar {
        height: 30
        background: Rectangle {
            color: Style.surfaceColor
        }
        
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Style.smallSpacing
            anchors.rightMargin: Style.smallSpacing
            
            Label {
                text: {
                    if (conversionController.isScanning) {
                        return qsTr("Scanning for files...")
                    } else if (conversionController.filesFound > 0) {
                        return qsTr("%1 FLAC files found").arg(conversionController.filesFound)
                    } else {
                        return qsTr("Ready")
                    }
                }
                font.pixelSize: Style.smallFontSize
                color: Style.textSecondary
            }
            
            Item { Layout.fillWidth: true }
            
            Label {
                visible: conversionController.isConverting
                text: qsTr("%1 / %2 completed").arg(conversionController.filesCompleted).arg(conversionController.filesFound)
                font.pixelSize: Style.smallFontSize
                color: Style.textSecondary
            }
        }
    }
}