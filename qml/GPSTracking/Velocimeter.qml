import QtQuick 2.0

Item {
    property alias text: data.text
    Rectangle{
        width: parent.width
        height: parent.height
        color: "#EEEEEE"
        radius: parent.width/2
        border.width: 1
        border.color: "#CCCCCC"

        Text{
            id:data
            anchors.centerIn: parent
        }
    }
}
