import QtQuick

Window {
    id: main
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")

    Column {
        Text {
            id: name
            text: qsTr("Infos:\nQt Version: ")+qtVersion
            height: 40
            width: 100
        }

        Image {
            id: aImage
            source: "images/yosemite.jpg"
            fillMode: Image.PreserveAspectFit
            width: main.width
        }
    }

    Component.onCompleted: {
        Qt.callLater(() => {
                   console.log("Qt Version:", qtVersion)
                   // Qt, Qt.version => undefined ?
               })
    }
}
