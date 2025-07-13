import QtQuick 2.15
import QtQuick.Controls 2.15
import OpusRipperGUI 1.0

GroupBox {
    id: control
    
    label: Label {
        text: control.title
        color: Style.textPrimary
        font.pixelSize: Style.mediumFontSize
        font.bold: true
        padding: Style.smallSpacing
    }
    
    background: Rectangle {
        y: control.topPadding - control.bottomPadding + 4
        width: parent.width
        height: parent.height - control.topPadding + control.bottomPadding - 4
        color: "transparent"
        border.color: Style.dividerColor
        radius: Style.radius
    }
}