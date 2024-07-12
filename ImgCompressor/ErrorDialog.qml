import QtQuick 2.15

Rectangle {
    id: root
    width: 300
    height: 150
    color: "lightgray"
    border.color: "gray"
    border.width: 2
    radius: 10
    visible: false

    property alias errorMessage: messageText.text

    Text {
        id: messageText
        anchors.centerIn: parent
        width: parent.width - 20
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
    }

    Rectangle {
        id: okButton
        width: 80
        height: 30
        radius: 5
        color: "gray"
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        anchors.horizontalCenter: parent.horizontalCenter

        Text {
            anchors.centerIn: parent
            text: "OK"
            color: "white"
        }

        MouseArea {
            anchors.fill: parent
            onClicked: root.visible = false
            onPressed: parent.color = "darkgray"
            onReleased: parent.color = "gray"
        }
    }

    function open() {
        visible = true
    }
}
