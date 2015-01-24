import QtQuick 2.3

//BorderImage {
//    source: "tron_black_bg_no_corners.sci"
//    clip: true
//}

Rectangle {
    property int margin: 5
    color: "black"
    border.color: "cyan"
    border.width: 5
    radius: border.width * 2
}

