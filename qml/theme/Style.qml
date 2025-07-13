pragma Singleton
import QtQuick 2.15

QtObject {
    // Colors - Dark theme
    readonly property color primaryColor: "#2196F3"
    readonly property color primaryDark: "#1976D2"
    readonly property color primaryLight: "#64B5F6"
    readonly property color accentColor: "#FF5722"
    readonly property color backgroundColor: "#1E1E1E"
    readonly property color surfaceColor: "#2D2D2D"
    readonly property color errorColor: "#F44336"
    readonly property color successColor: "#4CAF50"
    readonly property color textPrimary: "#FFFFFF"
    readonly property color textSecondary: "#B0B0B0"
    readonly property color dividerColor: "#404040"
    
    // Spacing
    readonly property int smallSpacing: 8
    readonly property int mediumSpacing: 16
    readonly property int largeSpacing: 24
    readonly property int extraLargeSpacing: 32
    
    // Font sizes
    readonly property int smallFontSize: 12
    readonly property int regularFontSize: 14
    readonly property int mediumFontSize: 16
    readonly property int largeFontSize: 20
    readonly property int headerFontSize: 24
    
    // Component dimensions
    readonly property int buttonHeight: 36
    readonly property int inputHeight: 40
    readonly property int toolbarHeight: 56
    readonly property int radius: 4
    
    // Animations
    readonly property int shortDuration: 150
    readonly property int mediumDuration: 250
    readonly property int longDuration: 400
}