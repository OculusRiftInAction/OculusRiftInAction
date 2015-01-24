import QtQuick 2.0


Image {
    height: 128
    width: 128
    property int channelType: 0
    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.channelTextureSelected(parent.channelType, parent.source);
        }
    }
}

