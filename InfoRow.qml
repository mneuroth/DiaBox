// InfoRow.qml
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

RowLayout {
    property string leftText: ""
    property string rightText: ""
    property color textColor: "white"

    id: root
    width: parent ? parent.width : implicitWidth
    Layout.fillWidth: true
    spacing: 0

    Label {
        id: leftLabel
        text: leftText
        color: textColor
        font.pixelSize: 11
    }

    Item {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
    }

    Label {
        id: rightLabel
        text: rightText
        color: textColor
        font.pixelSize: 11
        horizontalAlignment: Text.AlignRight
        Layout.alignment: Qt.AlignRight
        Layout.minimumWidth: 0
    }
}
