import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.platform 1.1 as Platform
import OpusRipperGUI 1.0

Item {
    id: root
    
    property string label: ""
    property string directory: ""
    property string placeholderText: qsTr("Select directory...")
    
    signal directorySelected(string directory)
    
    implicitHeight: column.implicitHeight
    
    ColumnLayout {
        id: column
        anchors.fill: parent
        spacing: Style.smallSpacing
        
        Label {
            text: root.label
            font.pixelSize: Style.regularFontSize
            color: Style.textPrimary
            visible: root.label !== ""
        }
        
        RowLayout {
            Layout.fillWidth: true
            spacing: Style.smallSpacing
            
            TextField {
                id: pathField
                Layout.fillWidth: true
                text: root.directory
                placeholderText: root.placeholderText
                readOnly: true
                color: Style.textPrimary
                placeholderTextColor: Style.textSecondary
                
                background: Rectangle {
                    color: pathField.hovered ? Qt.lighter(Style.surfaceColor, 1.1) : Style.surfaceColor
                    border.color: pathField.activeFocus ? Style.primaryColor : Style.dividerColor
                    border.width: pathField.activeFocus ? 2 : 1
                    radius: Style.radius
                }
            }
            
            Button {
                text: qsTr("Browse...")
                onClicked: folderDialog.open()
                
                Layout.preferredWidth: 100
            }
        }
    }
    
    Platform.FolderDialog {
        id: folderDialog
        title: root.label !== "" ? root.label : qsTr("Select Directory")
        folder: root.directory !== "" ? Platform.StandardPaths.writableLocation(Platform.StandardPaths.HomeLocation) : root.directory
        
        onAccepted: {
            root.directory = folder.toString().replace("file://", "")
            root.directorySelected(root.directory)
        }
    }
}