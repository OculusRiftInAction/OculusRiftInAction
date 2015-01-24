import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2

Item {
    id: channelRoot
    width: 1280
    height: 720

    CustomBorder {
        id: textureGroup
        width: channelRoot.width
        height: 384 + 24 + 48
        anchors.top: parent.top
        anchors.topMargin: 0
        CustomText {
            id: labelTextures
            text: qsTr("Textures")
            anchors.top: texturesColumn.top
            anchors.topMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 24
        }
        Column {
            id: texturesColumn
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            spacing: 16
            anchors.leftMargin: 256
            Row {
                spacing: 24
                TextureIcon { source: "qrc:/presets/tex00.jpg" }
                TextureIcon { source: "qrc:/presets/tex01.jpg" }
                TextureIcon { source: "qrc:/presets/tex02.jpg" }
                TextureIcon { source: "qrc:/presets/tex03.jpg" }
                TextureIcon { source: "qrc:/presets/tex04.jpg" }
                TextureIcon { source: "qrc:/presets/tex05.jpg" }
            }
            Row {
                spacing: 24
                TextureIcon { source: "qrc:/presets/tex06.jpg" }
                TextureIcon { source: "qrc:/presets/tex07.jpg" }
                TextureIcon { source: "qrc:/presets/tex08.jpg" }
                TextureIcon { source: "qrc:/presets/tex09.jpg" }
                TextureIcon { source: "qrc:/presets/tex10.png" }
                TextureIcon { source: "qrc:/presets/tex11.png" }
            }
            Row {
                spacing: 24
                TextureIcon { source: "qrc:/presets/tex12.png" }
                TextureIcon { source: "qrc:/presets/tex14.png" }
                TextureIcon { source: "qrc:/presets/tex15.png" }
                TextureIcon { source: "qrc:/presets/tex16.png" }
                CustomButton {
                    id: browse
                    width: 256 + 24
                    height: parent.height
                    text: qsTr("Browse")
                    enabled: false
                    onClicked: {
                    }
                }

            }
        }
    }

    CustomBorder {
        id: cubemapGroup
        width: channelRoot.width
        height: 128 + 48
        anchors.top: textureGroup.bottom
        anchors.topMargin: 8

        CustomText {
            id: labelCubemaps
            text: qsTr("Cubemaps")
            anchors.left: parent.left
            anchors.top: cubemapsRow.top
            anchors.topMargin: 0
            anchors.leftMargin: 24
        }
        Row {
            id: cubemapsRow
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            spacing: 24
            anchors.leftMargin: 256
            TextureIcon { channelType: 1; source: "qrc:/presets/cube00_0.jpg" }
            TextureIcon { channelType: 1; source: "qrc:/presets/cube01_0.png" }
            TextureIcon { channelType: 1; source: "qrc:/presets/cube02_0.jpg" }
            TextureIcon { channelType: 1; source: "qrc:/presets/cube03_0.png" }
            TextureIcon { channelType: 1; source: "qrc:/presets/cube04_0.png" }
            TextureIcon { channelType: 1; source: "qrc:/presets/cube05_0.png" }
        }
    }


    CustomButton {
        id: cancel
        width: 256
        text: qsTr("Cancel")
        anchors.top: cubemapGroup.bottom
        anchors.topMargin: 8
        anchors.right: parent.right
        anchors.rightMargin: 8
        onClicked: {
            root.setUiMode("edit");
        }
    }

}





