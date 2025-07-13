import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import OpusRipperGUI 1.0

Drawer {
    id: root
    
    property ConversionController controller: null
    
    width: Math.min(400, parent.width * 0.8)
    height: parent.height
    edge: Qt.RightEdge
    
    background: Rectangle {
        color: Style.backgroundColor
    }
    
    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        
        ColumnLayout {
            width: parent.width
            spacing: Style.largeSpacing
            
            // Header
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: Style.toolbarHeight
                
                Rectangle {
                    anchors.fill: parent
                    color: Style.surfaceColor
                }
                
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Style.mediumSpacing
                    anchors.rightMargin: Style.mediumSpacing
                    
                    Label {
                        text: qsTr("Settings")
                        font.pixelSize: Style.largeFontSize
                        font.bold: true
                        color: Style.textPrimary
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    Button {
                        icon.name: "window-close"
                        flat: true
                        onClicked: root.close()
                    }
                }
            }
            
            // Settings content
            StyledGroupBox {
                Layout.fillWidth: true
                Layout.margins: Style.mediumSpacing
                title: qsTr("Encoding Settings")
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: Style.mediumSpacing
                    
                    // Bitrate
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Style.smallSpacing
                        
                        Label {
                            text: qsTr("Bitrate (kbps)")
                            font.pixelSize: Style.regularFontSize
                            color: Style.textPrimary
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Slider {
                                id: bitrateSlider
                                Layout.fillWidth: true
                                from: 32
                                to: 256
                                stepSize: 8
                                value: controller ? controller.bitrate / 1000 : 128
                                
                                onValueChanged: {
                                    if (controller) {
                                        controller.bitrate = value * 1000
                                    }
                                }
                            }
                            
                            Label {
                                Layout.preferredWidth: 60
                                text: qsTr("%1").arg(Math.round(bitrateSlider.value))
                                font.pixelSize: Style.regularFontSize
                                color: Style.textSecondary
                                horizontalAlignment: Text.AlignRight
                            }
                        }
                    }
                    
                    // Complexity
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Style.smallSpacing
                        
                        Label {
                            text: qsTr("Encoding Complexity")
                            font.pixelSize: Style.regularFontSize
                            color: Style.textPrimary
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Slider {
                                id: complexitySlider
                                Layout.fillWidth: true
                                from: 0
                                to: 10
                                stepSize: 1
                                value: controller ? controller.complexity : 10
                                
                                onValueChanged: {
                                    if (controller) {
                                        controller.complexity = value
                                    }
                                }
                            }
                            
                            Label {
                                Layout.preferredWidth: 60
                                text: qsTr("%1").arg(complexitySlider.value)
                                font.pixelSize: Style.regularFontSize
                                color: Style.textSecondary
                                horizontalAlignment: Text.AlignRight
                            }
                        }
                        
                        Label {
                            text: qsTr("Higher complexity = better quality but slower encoding")
                            font.pixelSize: Style.smallFontSize
                            color: Style.textSecondary
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }
                    
                    // VBR
                    Switch {
                        id: vbrSwitch
                        text: qsTr("Variable Bitrate (VBR)")
                        checked: controller ? controller.vbr : true
                        
                        onToggled: {
                            if (controller) {
                                controller.vbr = checked
                            }
                        }
                    }
                }
            }
            
            StyledGroupBox {
                Layout.fillWidth: true
                Layout.margins: Style.mediumSpacing
                title: qsTr("Processing Settings")
                
                ColumnLayout {
                    anchors.fill: parent
                    spacing: Style.mediumSpacing
                    
                    // Thread count
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: Style.smallSpacing
                        
                        Label {
                            text: qsTr("Parallel Conversions")
                            font.pixelSize: Style.regularFontSize
                            color: Style.textPrimary
                        }
                        
                        RowLayout {
                            Layout.fillWidth: true
                            
                            Slider {
                                id: threadSlider
                                Layout.fillWidth: true
                                from: 1
                                to: controller ? controller.maxThreadCount : 8
                                stepSize: 1
                                value: controller ? controller.threadCount : 4
                                
                                onValueChanged: {
                                    if (controller) {
                                        controller.threadCount = value
                                    }
                                }
                            }
                            
                            Label {
                                Layout.preferredWidth: 60
                                text: qsTr("%1").arg(threadSlider.value)
                                font.pixelSize: Style.regularFontSize
                                color: Style.textSecondary
                                horizontalAlignment: Text.AlignRight
                            }
                        }
                    }
                    
                    // Preserve folder structure
                    Switch {
                        id: preserveStructureSwitch
                        text: qsTr("Preserve folder structure")
                        checked: controller ? controller.preserveFolderStructure : true
                        
                        onToggled: {
                            if (controller) {
                                controller.preserveFolderStructure = checked
                            }
                        }
                    }
                    
                    // Overwrite existing
                    Switch {
                        id: overwriteSwitch
                        text: qsTr("Overwrite existing files")
                        checked: controller ? controller.overwriteExisting : false
                        
                        onToggled: {
                            if (controller) {
                                controller.overwriteExisting = checked
                            }
                        }
                    }
                }
            }
            
            Item { Layout.fillHeight: true }
        }
    }
}