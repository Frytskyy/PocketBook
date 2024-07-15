import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Image Compression UI Assesment")

    ListView {
        id: listView
        anchors.fill: parent
        model: fileModel
        delegate: Rectangle {
            width: listView.width
            height: 40
            color: index % 2 ? "#f0f0f0" : "white"

            Text {
                anchors {
                    verticalCenter: parent.verticalCenter
                    left: parent.left
                    leftMargin: 10
                }
                text: name + " (" + size_str + ") " + state_str
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    fileModel.processFile(index)
                }
            }
        }

    }
}
