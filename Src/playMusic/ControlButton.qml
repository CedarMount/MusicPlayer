import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15

Rectangle {
    id: button
    width: 40
    height: 40
    radius: width / 2
    color: "transparent"
    enabled: true

    property string iconSource: ""
    property string tooltipText: ""
    property bool isPlaying: false
    property bool isFavorite: false
    /// 为 true 且 isPlaying 为 true 时用双竖条代替图标（播放/暂停键）
    property bool usePauseBarsWhenPlaying: false

    signal clicked()

    Image {
        id: buttonIcon
        anchors.centerIn: parent
        visible: !(usePauseBarsWhenPlaying && isPlaying)
        source: iconSource
        width: 24
        height: 24
        fillMode: Image.PreserveAspectFit
        opacity: 0.7  // 初始半透明
    }

    Row {
        anchors.centerIn: parent
        spacing: 4
        visible: usePauseBarsWhenPlaying && isPlaying
        Repeater {
            model: 2
            Rectangle {
                width: 4
                height: 16
                radius: 1
                color: "#cfcfd4"
            }
        }
    }

    MouseArea {
        id: buttonMouse
        anchors.fill: parent
        hoverEnabled: button.enabled
        enabled: button.enabled
        onEntered: {
            if (!button.enabled)
                return
            button.color = "#45454e"
            buttonIcon.opacity = 1.0  // hover 时完全不透明
            cursorShape = Qt.PointingHandCursor
        }
        onExited: {
            button.color = "transparent"
            buttonIcon.opacity = button.enabled ? 0.7 : 0.35
        }
        onClicked: {
            if (button.enabled)
                button.clicked()
        }
    }

    // 独立 ToolTip（Popup）才有 font；附加在 MouseArea 上的 ToolTip 在 Qt5 无 font 属性
    ToolTip {
        parent: button
        visible: button.enabled && button.tooltipText.length > 0 && buttonMouse.containsMouse
        text: button.tooltipText
        font.family: "微软雅黑 Light"
        font.pixelSize: 12
        delay: 400
    }
}