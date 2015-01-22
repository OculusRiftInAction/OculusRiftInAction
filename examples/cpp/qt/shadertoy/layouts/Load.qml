import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2
import QtQuick.Layouts 1.0
import Qt.labs.folderlistmodel 1.0


Item {
    id: loadRoot
    objectName: "loadRoot"
    width: 1280
    height: 720
    signal channelSelect(var msg)
    property string activeShaderString;
    property var activeShader;

    onActiveShaderStringChanged: {
        if (activeShaderString) {
            activeShader = JSON.parse(activeShaderString);
            var shaderInfo = activeShader.Shader[0].info;
            grid1.visible = true;
            shaderInfoDescription.text = shaderInfo.description;
            shaderInfoName.text = shaderInfo.name;
        } else {
            grid1.visible = false;
            activeShader = null
        }
    }

    Component {
        id: highlight
        Rectangle {
            width: 256 - 24; height: 40
            color: "slategray"; radius: 5
//            Behavior on y {
//                SpringAnimation {
//                    spring: 3
//                    damping: 0.2
//                }
//            }
        }
    }

    CustomBorder {
        id: presetsBorder
        width: 256
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0
        anchors.bottom: buttonRow.top
        anchors.bottomMargin: 8

        ListView {
            id: presets
            anchors.rightMargin: 12
            anchors.leftMargin: 12
            anchors.bottomMargin: 12
            anchors.topMargin: 12
            anchors.fill: parent
            highlight: highlight
            highlightFollowsCurrentItem: true
            focus: true
            delegate: CustomText {
                text: modelData
                MouseArea {
                    z: 1
                    anchors.fill: parent
                    onClicked: {
                        presets.currentIndex = index
                        userShaders.currentIndex = -1;
                    }
                    onDoubleClicked: {
                        root.loadPreset(presets.currentIndex)
                        root.setUiMode("edit");
                    }
                }
            }
            model: presetsModel
        }
    }

    CustomBorder {
        id: userShadersBorder
        width: 256
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0
        anchors.bottom: buttonRow.top
        anchors.bottomMargin: 8

        FolderListModel {
            id: userPresetsModel
            objectName: "userPresetsModel"
            showDotAndDotDot : true
            nameFilters : ["*.xml", "*.json"]
            folder:  userPresetsFolder
        }

        ListView {
            clip: true
            id: userShaders
            anchors.rightMargin: 12
            anchors.leftMargin: 12
            anchors.bottomMargin: 12
            anchors.topMargin: 12
            anchors.fill: parent
            highlight: highlight
            highlightFollowsCurrentItem: true
            highlightMoveDuration: 10
            focus: true
            model: userPresetsModel
            delegate: CustomText {
                text: fileName
                MouseArea {
                    z: 1
                    anchors.fill: parent
                    onClicked: {
                        userShaders.currentIndex = index
                        presets.currentIndex = -1;
                    }
                    onDoubleClicked: {
                        console.log("Current folder: " + userPresetsModel.folder);
                        var filePath = userPresetsModel.get(index, "filePath");
                        if (userPresetsModel.isFolder(index)) {
                            console.log("Current filepath: " + filePath);
                            root.newShaderFilepath(filePath);
                        } else {
                            root.loadShaderFile(filePath);
                            root.setUiMode("edit");
                       }
                    }
                }
            }

            onCurrentItemChanged: {
                if (userPresetsModel.isFolder(userShaders.currentIndex)) {
                    previewImage.source = ""
                } else {
                    var name = userPresetsModel.get(userShaders.currentIndex, "filePath");
                    root.newShaderHighlighted(name);
                }
            }

        }
    }

    Row {
        id: buttonRow
        height: 64
        layoutDirection: Qt.RightToLeft
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        spacing: 8
        anchors.margins: 8

        CustomButton {
            width: 192
            id: load
            text: qsTr("Load")
            onClicked: {
                if (presets.currentIndex != -1) {
                    root.loadPreset(presets.currentIndex)
                } else if (userShaders.currentIndex != -1) {
                    console.log(userPresetsModel.get(userShaders.currentIndex, "filePath"))
                    root.loadShaderFile(userPresetsModel.get(userShaders.currentIndex, "filePath"))
                }

                root.setUiMode("edit");
           }
        }

        CustomButton {
            id: cancel
            width: 192
            text: qsTr("Cancel")
            onClicked: {
                root.setUiMode("edit");
            }
        }
    }

    CustomBorder {
        id: shaderInfoBorder
        anchors.bottom: buttonRow.top
        anchors.bottomMargin: 8
        anchors.right: userShadersBorder.left
        anchors.rightMargin: 8
        anchors.left: presetsBorder.right
        anchors.leftMargin: 8
        anchors.top: parent.top
        anchors.topMargin: 0

        Image {
            id: previewImage
            objectName: "previewImage"
            x: 16
            width: 720
            height: 405
            anchors.top: parent.top
            anchors.topMargin: 12
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Item {
            id: grid1
            anchors.right: parent.right
            anchors.rightMargin: 12
            visible: false
            anchors.top: previewImage.bottom
            anchors.topMargin: 8
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            CustomText { id: label1; width: 192; text: "Name"; anchors.top: parent.top; anchors.topMargin: 0; anchors.left: parent.left; anchors.leftMargin: 0 }
            CustomText { id:shaderInfoName; text: " " ; anchors.right: parent.right; anchors.rightMargin: 0; anchors.left: label1.right;anchors.leftMargin: 8 }
            CustomText { id: label2; width: 192; text: "Description"; anchors.left: parent.left; anchors.leftMargin: 0; anchors.top: label1.bottom; anchors.topMargin: 8 }
            CustomTextArea {
                clip: true
                id:shaderInfoDescription;
                text: " " ; anchors.bottom: parent.bottom; anchors.bottomMargin: 0; anchors.right: parent.right; anchors.rightMargin: 0; anchors.left: label2.right; anchors.leftMargin: 8; anchors.top: label2.top;anchors.topMargin: 0
            }
        }
    }

}




