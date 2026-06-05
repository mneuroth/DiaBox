import QtQuick

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    Text {
        id: name
        text: qsTr("Some content !\tQt Version: ")+qtVersion
    }

    Component.onCompleted: {
        Qt.callLater(() => {
                   console.log("Qt Version:", qtVersion)
                   // Qt, Qt.version => undefined ?
               })
    }
}
