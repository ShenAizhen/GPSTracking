import QtQuick 2.0
import QtPositioning 5.0
import QtLocation 5.6
import GPSViewer 1.0

Item {
    objectName: "MainItem"
    id:mainContainer
    width: 791
    height: 800

    focus: true

    property color greenColor: "#75E44D";
    property color redColor: "#ED644D";
    property color greyColor: "#838383";
    property int lineCount: 5;
    property var states;

    signal setRectColor(int i, int isOn);
    onSetRectColor: {
        if (i >= 0 && i < lineCount) {
            if (states[i] > 0) { // ligado
                if (isOn)
                    repId.itemAt(i).color = greenColor;
                else // desligado
                    repId.itemAt(i).color = redColor;
            } else
                repId.itemAt(i).color = greyColor; // cinza
        }
    }

    function setLineCount(newLineCount){
        states = new Array(newLineCount);
        if(newLineCount > 0){
            for(var i = 0; i < newLineCount; i += 1){
                states[i] = 1;
            }
            lineCount = newLineCount;
            repId.model = lineCount;
            repId.update();
        }
    }

    function setThemeColors(fontColor){
        titleText.color = Qt.rgba(fontColor.x,
                                  fontColor.y,
                                  fontColor.z,
                                  1.0);
    }

    signal interrupt_gps();
    signal swap_state_line(int i, int isOn);

    Keys.onPressed: {
        if(event.key === Qt.Key_G && event.modifiers & Qt.ControlModifier){
            gpsView.swap_wire_frame_mode();
        }else if(event.key === Qt.Key_S && event.modifiers & Qt.ControlModifier){
            interrupt_gps();
        }else if(event.key === Qt.Key_D&& event.modifiers & Qt.ControlModifier){
            gpsView.swap_control_point_mode();
        }
    }

    Velocimeter{
        width: parent.width * 0.1
        height: parent.height * 0.1
        anchors.left: parent.left
        anchors.top: parent.top
        text: "20km/h"
        visible: false
    }

    SetButton{
        width: parent.width*0.1
        height: parent.height*0.1
        anchors.right: parent.right
        anchors.rightMargin: parent.width*0.01
        anchors.top: parent.top
        anchors.topMargin: parent.height*0.01
        source: "qrc:///qml/images/nav_adjusts_80x80.svg"
        visible: true
        onClicked: {
            gpsView.swap_view_mode();
        }
    }

    SetButton{
        id:zoomOut
        width: parent.width*0.1
        height: parent.height*0.1
        anchors.right: parent.right
        anchors.rightMargin: parent.width*0.01
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height*0.01
        source: "qrc:///qml/images/nav_zoom-out_80x80.svg"
        visible: true
        onClicked: {
            gpsView.zoom_out_request();
        }
    }

    SetButton{
        id:zoomIn
        width: parent.width*0.1
        height: parent.height*0.1
        anchors.right: parent.right
        anchors.rightMargin: parent.width*0.01
        anchors.bottom: zoomOut.top
        anchors.bottomMargin: parent.height*0.01
        source: "qrc:///qml/images/nav_zoom-in_80x80.svg"
        visible: true
        onClicked: {
            gpsView.zoom_in_request();
        }
    }

    Grid{
        id:rowId
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height * 0.01
        anchors.left: parent.left
        anchors.leftMargin: parent.width * 0.01
        width: parent.width * 0.6
        height: parent.height * 0.1
        rows: 2; columns: 40; spacing: 4

        Repeater{
            id:repId
            model: 20
            Rectangle{
                width: rowId.width / 40.1
                height: rowId.height / 2 - 2
                border.width: 1
                border.color: "black"
                radius: rowId.width*0.05
                color: greenColor

                MouseArea{
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        cursorShape = Qt.PointingHandCursor;
                    }

                    onExited: {
                        cursorShape = Qt.ArrowCursor;
                    }

                    onClicked: {
                        mainContainer.states[index] = 1 - mainContainer.states[index];
                        repId.itemAt(index).color = mainContainer.states[index] > 0 ? greenColor : greyColor;
                        swap_state_line(index, mainContainer.states[index]);
                    }
                }
            }
        }
    }

    Text{
        id:titleText
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        font.pointSize: 12
        text: "Work in progress"
        visible: true
        color: "#77000000"
    }

    GPSView {
        id: gpsView
        width: parent.width
        height: parent.height
        x: 0
        y: 0
    }
}
