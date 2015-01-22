import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Window 2.2



Item {
    id: root
    width: 1280
    height: 720
    FontLoader { id: tronFont; source: "fonts/uifont.ttf" }
    property int activeChannel: -1
    property alias text: editor.text

    signal channelTextureChanged(int channel, int type, string url)
    signal shaderSourceChanged(string shaderSource)

    signal loadPreset(int presetIndex)
    signal loadNextPreset()
    signal loadPreviousPreset()

    signal loadShaderFile(string shaderPath)
    signal saveShaderXml(string shaderName)

    signal modifyTextureResolution(double scale)
    signal resetPositionScale()
    signal modifyPositionScale(double scale)
    signal newShaderFilepath(string newPath);
    signal newShaderHighlighted(string newPath);

    signal toggleUi()
    signal toggleEyePerFrame()

    signal startShutdown()
    signal restartShader()
    signal recenterPose()

    signal epfModeChanged(bool checked)

    Keys.onPressed: {
        switch (event.key) {
        case Qt.Key_Q:
            if (Qt.ControlModifier == event.modifiers) {
                startShutdown();
                event.accepted = true;
                break;
            }

        case Qt.Key_Escape:
        case Qt.Key_F1:
            toggleUi();
            event.accepted = true;
            break;

        case Qt.Key_F3:
            toggleEyePerFrame();
            event.accepted = true;
            break;

        case Qt.Key_F4:
            if (Qt.ShiftModifier == event.modifiers) {
                restartShader()
            } else {
                shaderSourceChanged(text);
            }
            event.accepted = true;
            break;

        case Qt.Key_F5:
            if (Qt.ShiftModifier == event.modifiers) {
                modifyPositionScale(Math.SQRT1_2);
            } else {
                modifyTextureResolution(Math.SQRT1_2);
            }
            event.accepted = true;
            break;

        case Qt.Key_F6:
            if (Qt.ShiftModifier == event.modifiers) {
                resetPositionScale();
            } else {
                modifyTextureResolution(0.95);
            }
            event.accepted = true;
            break;

        case Qt.Key_F7:
            if (Qt.ShiftModifier == event.modifiers) {
                resetPositionScale();
            } else {
                modifyTextureResolution(1.05);
            }
            event.accepted = true;
            break;

        case Qt.Key_F8:
            if (Qt.ShiftModifier == event.modifiers) {
                modifyPositionScale(Math.SQRT2);
            } else {
                modifyTextureResolution(Math.SQRT2);
            }
            event.accepted = true;
            break;

        case Qt.Key_F9:
            loadPreviousPreset();
            event.accepted = true;
            break;

        case Qt.Key_F10:
            loadNextPreset();
            event.accepted = true;
            break;

        case Qt.Key_F11:
            saveShader();
            event.accepted = true;
            break;

        case Qt.Key_F2:
        case Qt.Key_F12:
            recenterPose();
            event.accepted = true;
            break;
        }
    }

    function channelSelect(channel) {
        activeChannel = channel;
        console.log(channel);
        channelSelect.visible = true;
        editor.visible = false;
    }

    function channelTextureSelected(type, textureUrl) {
        editor.setChannelIcon(activeChannel, textureUrl);
        channelTextureChanged(activeChannel, type, textureUrl);
        channelSelect.visible = false;
        editor.visible = true;
    }

    function setUiMode(mode) {
        editor.visible = false;
        channelSelect.visible = false;
        loader.visible = false;
        saver.visible = false;
        if (mode == "edit") {
            editor.visible = true;
        } else if (mode == "load") {
            loader.visible = true;
        } else if (mode == "save") {
            saver.visible = true;
        } else if (mode == "channelSelect") {
            channelSelect.visible = true;
        } else {
            editor.visible = true;
        }
    }

    CustomBorder {
        z: 1
        width: 1200
        height: 128
        border.color: "red"
        color: "#222"
        anchors.top: parent.top
        anchors.topMargin: 200
        border.width: 10
        anchors.horizontalCenter: parent.horizontalCenter
        visible: false
        CustomText {
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            anchors.fill: parent
        }
    }


    ChannelSelect {
        id: channelSelect
        visible: false
        anchors.fill: parent
    }

    Editor {
        id: editor
        visible: true
        anchors.fill: parent
    }

    Load {
        id: loader
        visible: false
        anchors.fill: parent
    }

    Save {
        id: saver
        visible: false
        anchors.fill: parent
    }
}




