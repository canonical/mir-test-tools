import QtQuick 2.0

Item {
    width: 500
    height: 500

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0.17254902, 0.0, 0.117647059, 1.0)

        Rectangle {
            color: Qt.rgba(0.866666667, 0.282352941, 0.141414141, 1.0)
            anchors.fill: parent

            SequentialAnimation on rotation {
                loops: Animation.Infinite
                NumberAnimation { from: 0; to: 360; duration: 10000; }
            }
        }
    }
}

