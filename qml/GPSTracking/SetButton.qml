import QtQuick 2.0

Rectangle{
    id:mainRect
    property string source;
    color:"transparent"
    border.width: 1
    border.color: "transparent"
    signal clicked();
    Image{
        source: parent.source
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
    }

    MouseArea{
        anchors.fill: parent
        onPressed: {
            parent.border.color = Qt.rgba(0.65, 0.6, 1.0, 1.0);
        }

        onReleased: {
            parent.border.color = "transparent";
        }

        onClicked: {
            mainRect.clicked();
        }
    }
}
