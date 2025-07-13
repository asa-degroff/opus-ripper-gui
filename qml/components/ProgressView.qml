import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import OpusRipperGUI 1.0

Item {
    id: root
    
    property ProgressModel model: null
    
    ColumnLayout {
        anchors.fill: parent
        spacing: Style.mediumSpacing
        
        // Overall progress
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            
            ColumnLayout {
                anchors.fill: parent
                spacing: Style.smallSpacing
                
                RowLayout {
                    Layout.fillWidth: true
                    
                    Label {
                        text: qsTr("Overall Progress")
                        font.pixelSize: Style.mediumFontSize
                        font.bold: true
                        color: Style.textPrimary
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    Label {
                        text: model ? qsTr("%1 / %2").arg(model.filesCompleted).arg(model.totalFiles) : "0 / 0"
                        font.pixelSize: Style.regularFontSize
                        color: Style.textSecondary
                    }
                }
                
                ProgressBar {
                    id: overallProgress
                    Layout.fillWidth: true
                    Layout.preferredHeight: 8
                    value: model ? model.overallProgress : 0
                    
                    background: Rectangle {
                        color: Style.dividerColor
                        radius: height / 2
                    }
                    
                    contentItem: Item {
                        Rectangle {
                            width: overallProgress.visualPosition * parent.width
                            height: parent.height
                            radius: height / 2
                            color: Style.primaryColor
                            
                            Behavior on width {
                                NumberAnimation { duration: Style.shortDuration }
                            }
                        }
                    }
                }
                
                RowLayout {
                    Layout.fillWidth: true
                    visible: model && model.isConverting
                    
                    Label {
                        text: model ? model.timeElapsed : ""
                        font.pixelSize: Style.smallFontSize
                        color: Style.textSecondary
                    }
                    
                    Item { Layout.fillWidth: true }
                    
                    Label {
                        text: model ? qsTr("Estimated time remaining: %1").arg(model.timeRemaining) : ""
                        font.pixelSize: Style.smallFontSize
                        color: Style.textSecondary
                    }
                }
            }
        }
        
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Style.dividerColor
        }
        
        // Current file progress
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            visible: model && model.currentFile !== ""
            
            ColumnLayout {
                anchors.fill: parent
                spacing: Style.smallSpacing
                
                Label {
                    text: qsTr("Current File")
                    font.pixelSize: Style.regularFontSize
                    color: Style.textPrimary
                }
                
                Label {
                    Layout.fillWidth: true
                    text: model ? model.currentFile : ""
                    font.pixelSize: Style.smallFontSize
                    color: Style.textSecondary
                    elide: Text.ElideMiddle
                }
                
                ProgressBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 4
                    value: model ? model.currentFileProgress : 0
                    
                    background: Rectangle {
                        color: Style.dividerColor
                        radius: height / 2
                    }
                    
                    contentItem: Item {
                        Rectangle {
                            width: parent.width * parent.parent.value
                            height: parent.height
                            radius: height / 2
                            color: Style.accentColor
                        }
                    }
                }
            }
        }
        
        // File list
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            
            ListView {
                id: fileListView
                model: root.model ? root.model.fileList : null
                spacing: 2
                
                delegate: Rectangle {
                    width: ListView.view.width
                    height: 40
                    color: mouseArea.containsMouse ? Qt.lighter(Style.surfaceColor, 1.02) : "transparent"
                    radius: Style.radius
                    
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Style.mediumSpacing
                        anchors.rightMargin: Style.mediumSpacing
                        
                        Rectangle {
                            width: 8
                            height: 8
                            radius: 4
                            color: {
                                switch(modelData.status) {
                                case "pending":
                                    return Style.textSecondary
                                case "converting":
                                    return Style.primaryColor
                                case "completed":
                                    return Style.successColor
                                case "failed":
                                    return Style.errorColor
                                default:
                                    return Style.textSecondary
                                }
                            }
                        }
                        
                        Label {
                            Layout.fillWidth: true
                            text: modelData.fileName
                            font.pixelSize: Style.smallFontSize
                            color: modelData.status === "failed" ? Style.errorColor : Style.textPrimary
                            elide: Text.ElideMiddle
                        }
                        
                        Label {
                            text: modelData.status === "converting" ? qsTr("%1%").arg(modelData.progress) : ""
                            font.pixelSize: Style.smallFontSize
                            color: Style.textSecondary
                            visible: modelData.status === "converting"
                        }
                    }
                    
                    MouseArea {
                        id: mouseArea
                        anchors.fill: parent
                        hoverEnabled: true
                        
                        ToolTip {
                            visible: mouseArea.containsMouse && modelData.error !== ""
                            text: modelData.error
                            delay: 500
                        }
                    }
                }
            }
        }
    }
}