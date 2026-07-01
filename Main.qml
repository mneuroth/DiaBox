import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Layouts
import QtQuick.Dialogs
import Qt.labs.settings

import DiaBox

import Perf
import Meta

ApplicationWindow {
    id: window
    width:       1200
    height:      800
    minimumWidth:  640
    minimumHeight: 420
    visible: true
    title:   "DiaBox"

    property string textColor: "#cccccc" //"#ffffff"
    property string grayTextColor: "#555555"
    property string darkTextColor: "#888888"
    property string backColor: "#000000"
    property string highlightColor: "#5566cc"
    property string darkColor: "#2a2a3a"
    property string darkDarkColor: "#1a1a2e"

    Universal.theme:  Universal.Dark
    Universal.accent: highlightColor

    // ── Persistent settings (written to %APPDATA%\PredictiveServices\ImageViewer.ini) ──
    Settings {
        id: appSettings
        property string imageFolder: "C:/images"
    }

    // ── Backend: C++ directory watcher / model ───────────────────────────────
    DirectoryModel {
        id: dirModel
        folder: appSettings.imageFolder

        // When the list of files changes (dir change, files added/deleted),
        // auto-select the first item.
        onCountChanged: {
            if (count > 0) {
                window.selectedIndex = 0
            } else {
                window.selectedIndex = -1
            }
        }
    }

    // ── Single source of truth for the selected file ─────────────────────────
    property int    selectedIndex: -1
    property url    currentImageUrl:  selectedIndex >= 0 ? dirModel.fileUrl(selectedIndex)  : ""
    property string currentImageName: selectedIndex >= 0 ? dirModel.fileName(selectedIndex) : ""

    // ── Folder picker dialog ─────────────────────────────────────────────────
    FolderDialog {
        id: folderDialog
        title: "Bildverzeichnis wählen"
        onAccepted: {
            // FolderDialog returns a file:// URL – convert to a plain local path
            var raw  = selectedFolder.toString()
            var path = raw.replace(/^file:\/\/\//, "")   // Windows: file:///C:/…
                          .replace(/^file:\/\//, "")     // Unix:    file:///home/…
                          .replace(/%20/g, " ")
            dirModel.folder        = path
            appSettings.imageFolder = path
            dirField.text           = path
        }
    }

    // ── Root content: resizable two-panel layout ─────────────────────────────
    SplitView {
        anchors.fill: parent
        orientation:  Qt.Horizontal
        handle: Rectangle {
            implicitWidth: 4
            color: SplitHandle.hovered || SplitHandle.pressed ? highlightColor : darkColor
        }

        // ── Left panel: file list ─────────────────────────────────────────────
        Rectangle {
            SplitView.preferredWidth: 260
            SplitView.minimumWidth:   140
            SplitView.maximumWidth:   480
            color: darkDarkColor

            ColumnLayout {
                anchors.top:     parent.top
                anchors.left:    parent.left
                anchors.right:   parent.right
                anchors.margins: 8
                spacing: 6

                Label {
                    text: qsTr("Infos: Qt Version: ")+qtVersion
                    color: textColor
                    Layout.alignment: Qt.AlignHCenter
                }

                // ── Folder selector ───────────────────────────────────────────
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    TextField {
                        id: dirField
                        Layout.fillWidth: true
                        text: appSettings.imageFolder
                        placeholderText: "Pfad eingeben…"
                        font.pixelSize: 11
                        onAccepted: {
                            var path = text.replace(/^file:\/\/\//, "")
                                           .replace(/^file:\/\//, "")
                            dirModel.folder        = path
                            appSettings.imageFolder = path
                            text = path
                        }
                    }

                    Button {
                        text:          "…"
                        implicitWidth: 36
                        onClicked:     folderDialog.open()
                        ToolTip.text:    "Verzeichnis auswählen"
                        ToolTip.visible: hovered
                        ToolTip.delay:   600
                    }
                }

                Rectangle {
                    id: treeViewRect
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    width: parent.width
                    height: 400
                    border.color: textColor
                    color: darkColor

                    TreeView {
                        id: treeView
                        anchors.fill: parent
                        clip: true
                        model: fileSystemModel
                        rootIndex: rootModelIndex

                        delegate: Item {
                            implicitHeight: 28
                            implicitWidth: treeView.width

                            //required property bool isTreeNode
                            required property bool expanded
                            required property bool hasChildren
                            required property int depth
                            required property int row
                            required property int column
                            required property var model

                            Rectangle {
                                anchors.fill: parent
                                color: mouseArea.containsMouse ? "#f0f6ff" : "transparent"
                            }

                            Row {
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: 6
                                x: depth * 20 + 6

                                Text {
                                    text: hasChildren ? (expanded ? "▼" : "▶") : "•"
                                    width: 16
                                }

                                Text {
                                    text: model.fileName !== "" ? model.fileName : model.filePath
                                    color: model.isDir ? grayTextColor : textColor
                                }
                            }

                            MouseArea {
                                id: mouseArea
                                anchors.fill: parent
                                hoverEnabled: true

                                onClicked: {
                                    let idx = treeView.index(row, column)
                                    if (hasChildren) {
                                        treeView.toggleExpanded(row)
                                    }
                                    console.log(">>>"+model.fileName + " "+model.filePath+ " "+ model.isDir)
                                    console.log("SET NEW PATH")
                                    dirModel.folder = model.filePath
                                    dirField.text = model.filePath
                                    window.selectedIndex = -1
                                    Qt.callLater(function() { window.selectedIndex = 0 })
                                    // gulp
                                }
                            }
                        }
                    }
                }

                SplitView {
                    id: sidebarSplit
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.preferredHeight: 240
                    Layout.minimumHeight: 140
                    orientation: Qt.Vertical
                    handle: Rectangle {
                        implicitHeight: 8
                        color: SplitHandle.hovered || SplitHandle.pressed ? highlightColor : "#444"
                        border.color: "#666"
                        radius: 2
                    }

                    Rectangle {
                        id: infoBox
                        SplitView.preferredHeight: 150
                        SplitView.minimumHeight: 100
                        color: darkColor
                        border.color: textColor
                        border.width: 1
                        radius: 4

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 6

                            Label {
                                text:  dirModel.count + " Bild(er)"
                                color: darkTextColor
                                font.pixelSize: 11
                                Layout.alignment: Qt.AlignHCenter
                            }

                            Label {
                                text:  Math.round(imageViewport.zoomFactor * 100) + " %"
                                color: darkTextColor
                                font.pixelSize: 11
                                Layout.alignment: Qt.AlignHCenter
                            }

                            Label {
                                text: window.currentImageUrl !== ""
                                      ? Math.round(imagePreview.paintedWidth) + " × "
                                        + Math.round(imagePreview.paintedHeight) + " px"
                                      : ""
                                color: darkTextColor
                                font.pixelSize: 11
                                Layout.alignment: Qt.AlignHCenter
                            }

                            Label {
                                text: window.screen.width + " × " + window.screen.height + " px"
                                color: darkTextColor
                                font.pixelSize: 11
                                Layout.alignment: Qt.AlignHCenter
                            }

                            Label {
                                text: Math.round(window.screen.devicePixelRatio * 100) + " %"
                                color: darkTextColor
                                font.pixelSize: 11
                                Layout.alignment: Qt.AlignHCenter
                            }

                            Label {
                                text: window.currentImageUrl !== "" && imagePreview.status === Image.Ready
                                      ? imagePreview.implicitWidth + " × " + imagePreview.implicitHeight + " px"
                                      : ""
                                color: darkTextColor
                                font.pixelSize: 11
                                Layout.alignment: Qt.AlignHCenter
                            }
                        }
                    }

                    Rectangle {
                        id: exifBox
                        SplitView.fillHeight: true
                        color: darkColor
                        border.color: textColor
                        border.width: 1
                        radius: 4

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 6

                            Label {
                                text: "EXIF"
                                color: textColor
                                font.pixelSize: 12
                                font.bold: true
                                Layout.alignment: Qt.AlignHCenter
                            }

                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true

                                Column {
                                    width: parent.width
                                    spacing: 6

                                    Repeater {
                                        model: exifModel
                                        delegate: Text {
                                            text: model.key + ": " + model.value
                                            color: "white"
                                            font.pixelSize: 13
                                            wrapMode: Text.Wrap
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                ListModel {
                    id: exifModel
                }

                ExifReader {
                    id: reader
                }
            }
        }

        // ── Right panel: image preview ────────────────────────────────────────
        Rectangle {
            SplitView.fillWidth: true
            color: "#0d0d1a"

            ColumnLayout {
                anchors.fill: parent
                anchors.bottomMargin: 28  // Space for caption bar
                spacing: 0

                // ── File list view (top) ──────────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 120
                    color: "#1a1a2e"
                    border { color: "#2a2a3a"; width: 1 }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4

                        Label {
                            text: dirModel.count + " Bild(er) im Verzeichnis"
                            color: "#aaaaaa"
                            font.pixelSize: 11
                            Layout.alignment: Qt.AlignHCenter
                        }

                        ListView {
                            id: fileList
                            Layout.fillWidth:  true
                            Layout.fillHeight: true
                            clip:  true
                            model: dirModel
                            currentIndex: window.selectedIndex
                            keyNavigationEnabled: true
                            focus: true
                            orientation: Qt.Horizontal

                            Keys.onUpPressed:   if (currentIndex > 0)         window.selectedIndex--
                            Keys.onDownPressed: if (currentIndex < count - 1) window.selectedIndex++
                            Keys.onPressed: (event) => {
                                if (event.key === Qt.Key_Home && count > 0) {
                                    window.selectedIndex = 0
                                    event.accepted = true
                                } else if (event.key === Qt.Key_End && count > 0) {
                                    window.selectedIndex = count - 1
                                    event.accepted = true
                                }
                            }

                            ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

                            delegate: ItemDelegate {
                                id: del
                                required property int    index
                                required property string fileName
                                required property url    thumbnailUrl
                                required property var    thumbnailImg

                                width:         100
                                height:        ListView.view.height
                                highlighted:   ListView.view.currentIndex === index
                                padding:       0
                                topPadding:    4
                                bottomPadding: 4
                                leftPadding:   4
                                rightPadding:  4

                                contentItem: Column {
                                    anchors.fill: parent
                                    spacing: 4

                                    Image {
                                        source:       del.thumbnailImg
                                        width:        parent.width
                                        height:       60
                                        fillMode:     Image.PreserveAspectFit
                                        smooth:       true
                                        asynchronous: true
                                        cache:        true
                                        visible:      del.thumbnailImg.toString() !== ""
                                    }

                                    Text {
                                        width:                    parent.width
                                        text:                     del.fileName
                                        color:                    del.highlighted ? textColor : darkTextColor
                                        font { pixelSize: 9; bold: del.highlighted }
                                        elide:                    Text.ElideMiddle
                                        horizontalAlignment:      Text.AlignHCenter
                                        wrapMode:                 Text.Wrap
                                    }
                                }

                                background: Rectangle {
                                    color: del.highlighted ? "#3a3a6e"
                                         : del.hovered     ? "#252548"
                                         :                   "transparent"
                                    radius: 3
                                }

                                onClicked: window.selectedIndex = index
                            }
                        }
                    }
                }

                // ── Placeholder when no image is selected ──────────────────────
                Label {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    text: dirModel.count === 0
                          ? "Verzeichnis enthält keine Bilder.\nWählen Sie ein Verzeichnis mit dem ‹…›-Button."
                          : "Wählen Sie ein Bild aus der Liste."
                    color: grayTextColor
                    font.pixelSize: 16
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    lineHeight: 1.6
                    visible: window.currentImageUrl.toString() === ""
                }

                // ── Zoomable / pannable image viewport ──────────────────────
                Item {
                    id: imageViewport
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    visible: window.currentImageUrl.toString() !== ""

                    property real zoomFactor: 1.0
                    property real panX: 0.0
                    property real panY: 0.0

                    function resetView() {
                        zoomFactor = 1.0
                        panX = 0.0
                        panY = 0.0
                    }

                    // Reset whenever a different image is selected
                    Connections {
                        target: window
                        function onSelectedIndexChanged() { imageViewport.resetView() }
                    }

                    Stopwatch {
                        id: sw
                    }

                    // ── The image itself
                    Image {
                        id: imagePreview
                        width:           imageViewport.width
                        height:          imageViewport.height
                        x:               imageViewport.panX
                        y:               imageViewport.panY
                        scale:           imageViewport.zoomFactor
                        transformOrigin: Item.TopLeft
                        source:          window.currentImageUrl
                        fillMode:        Image.PreserveAspectFit
                        smooth:          true
                        asynchronous:    true
                        cache:           false

                        onSourceChanged: {
                            sw.start()   // Startpunkt
                        }

                        onStatusChanged: {
                            if (status === Image.Ready) {
                                let ns = sw.elapsedNs()
                                let ms = ns / 1e6
                                console.log("Image geladen in", ms, "ms")
                            }

                            let sPath = "" + Qt.resolvedUrl(source)
                            sPath = sPath.slice(8)  // remove: file:/// from URL
                            let exif = reader.readExif(sPath)
                            exifModel.clear()

                            for (let key in exif) {
                               exifModel.append({ key: key, value: exif[key] })
                            }
                        }
                    }

                    // ── Mouse-wheel zoom
                    WheelHandler {
                        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                        onWheel: (event) => {
                            var delta   = event.angleDelta.y !== 0
                                          ? event.angleDelta.y : event.angleDelta.x
                            var factor  = delta > 0 ? 1.15 : (1.0 / 1.15)
                            var newZoom = Math.max(0.05, Math.min(20.0,
                                             imageViewport.zoomFactor * factor))
                            var ratio   = newZoom / imageViewport.zoomFactor
                            // Keep the pixel under the cursor fixed
                            imageViewport.panX = event.x * (1.0 - ratio) + imageViewport.panX * ratio
                            imageViewport.panY = event.y * (1.0 - ratio) + imageViewport.panY * ratio
                            imageViewport.zoomFactor = newZoom
                            event.accepted = true
                        }
                    }

                    // ── Drag to pan (mouse + 1-finger touch)
                    DragHandler {
                        id: dragHandler
                        target: null
                        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchScreen

                        property real startPanX: 0.0
                        property real startPanY: 0.0

                        onActiveChanged: {
                            if (active) {
                                startPanX = imageViewport.panX
                                startPanY = imageViewport.panY
                            }
                        }
                        onTranslationChanged: {
                            if (active) {
                                imageViewport.panX = startPanX + translation.x
                                imageViewport.panY = startPanY + translation.y
                            }
                        }
                        cursorShape: active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                    }

                    // ── Pinch to zoom + pan (2-finger touch / trackpad)
                    PinchHandler {
                        id: pinchHandler
                        target: null
                        acceptedDevices: PointerDevice.TouchScreen | PointerDevice.TouchPad
                        minimumPointCount: 2

                        property real  startZoom:     1.0
                        property real  startPanX:     0.0
                        property real  startPanY:     0.0
                        property point startCentroid: Qt.point(0, 0)

                        onActiveChanged: {
                            if (active) {
                                startZoom     = imageViewport.zoomFactor
                                startPanX     = imageViewport.panX
                                startPanY     = imageViewport.panY
                                startCentroid = centroid.position
                            }
                        }

                        // Called when either scale OR centroid changes
                        onActiveScaleChanged: applyPinch()
                        onCentroidChanged:    if (active) applyPinch()

                        function applyPinch() {
                            var newZoom = Math.max(0.05, Math.min(20.0,
                                             startZoom * activeScale))
                            var s = newZoom / startZoom
                            // Keep the start-centroid image point under the
                            // current centroid, and honour centroid translation
                            imageViewport.panX = centroid.position.x
                                               - (startCentroid.x - startPanX) * s
                            imageViewport.panY = centroid.position.y
                                               - (startCentroid.y - startPanY) * s
                            imageViewport.zoomFactor = newZoom
                        }
                    }

                    // ── Double-click / double-tap resets view
                    TapHandler {
                        acceptedButtons: Qt.LeftButton
                        gesturePolicy:   TapHandler.DoubleTapWithinBounds
                        onDoubleTapped:  imageViewport.resetView()
                    }

                    // ── Zoom-level indicator (top-right corner)
                    Rectangle {
                        anchors { top: parent.top; right: parent.right; margins: 6 }
                        width:   zoomLabel.implicitWidth + 10
                        height:  zoomLabel.implicitHeight + 6
                        radius:  3
                        color:   darkColor
                        visible: imageViewport.zoomFactor !== 1.0

                        Label {
                            id: zoomLabel
                            anchors.centerIn: parent
                            text:  Math.round(imageViewport.zoomFactor * 100) + " %"
                            color: textColor
                            font.pixelSize: 11
                        }
                    }
                }
            }

            // ── Loading spinner
            BusyIndicator {
                anchors.centerIn: parent
                running: imagePreview.status === Image.Loading
                visible: running
            }

            // ── Load-error message
            Label {
                anchors.centerIn: parent
                text:  "Bild konnte nicht geladen werden."
                color: textColor
                font.pixelSize: 14
                visible: imagePreview.status === Image.Error
            }

            // ── Caption bar
            Rectangle {
                id: captionBar
                anchors { bottom: parent.bottom; left: parent.left; right: parent.right }
                height: 28
                color:  darkColor

                Label {
                    anchors.centerIn: parent
                    width: parent.width - 24
                    text:  window.currentImageName
                    color: darkTextColor
                    font.pixelSize: 12
                    elide: Text.ElideMiddle
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }
}
