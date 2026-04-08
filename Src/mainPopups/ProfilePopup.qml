import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: root
    width: 420
    padding: 20
    modal: true
    anchors.centerIn: Overlay.overlay

    background: Rectangle {
        color: Window.window ? Window.window.colorNavBg : "#2d2d37"
        radius: 12
        border.width: 1
        border.color: Window.window ? Window.window.colorBorder : "#45454e"
    }

    onOpened: {
        nickField.text = UserSession.displayName !== UserSession.currentUsername
            ? UserSession.displayName
            : ""
    }

    ColumnLayout {
        width: parent.width
        spacing: 14

        Label {
            text: "用户信息"
            font.pixelSize: 20
            font.family: "微软雅黑 Light"
            color: Window.window ? Window.window.colorTextPrimary : "white"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 16
            Rectangle {
                width: 72
                height: 72
                radius: 36
                color: "#45454e"
                clip: true
                Image {
                    anchors.fill: parent
                    source: UserSession.avatarUrl.length > 0 ? UserSession.avatarUrl : ""
                    fillMode: Image.PreserveAspectCrop
                    visible: UserSession.avatarUrl.length > 0
                }
                Label {
                    anchors.centerIn: parent
                    visible: UserSession.avatarUrl.length === 0
                    text: "无"
                    color: "#aaa"
                    font.pixelSize: 12
                }
            }
            Button {
                text: "选择头像"
                enabled: UserSession.loggedIn
                font.family: "微软雅黑 Light"
                onClicked: UserSession.pickAvatar()
            }
        }

        Label {
            text: "昵称（显示名，可与登录账号不同）"
            font.pixelSize: 12
            color: Window.window ? Window.window.colorTextMuted : "#a1a1a3"
            font.family: "微软雅黑 Light"
        }
        TextField {
            id: nickField
            Layout.fillWidth: true
            placeholderText: UserSession.loggedIn ? UserSession.currentUsername : "请先登录"
            enabled: UserSession.loggedIn
            color: Window.window ? Window.window.colorTextPrimary : "white"
            font.family: "微软雅黑 Light"
            background: Rectangle {
                radius: 6
                color: Window.window ? Window.window.colorMainBg : "#13131a"
                border.color: Window.window ? Window.window.colorBorder : "#45454e"
            }
        }
        Button {
            text: "保存昵称"
            enabled: UserSession.loggedIn
            font.family: "微软雅黑 Light"
            onClicked: UserSession.saveNickname(nickField.text)
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Window.window ? Window.window.colorBorder : "#45454e"
        }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: "深色主题"
                font.family: "微软雅黑 Light"
                color: Window.window ? Window.window.colorTextPrimary : "white"
                Layout.fillWidth: true
            }
            Switch {
                checked: UserSession.prefersDarkTheme
                // Q_PROPERTY WRITE 在 QML 中通过赋值生效，不能调用 setPrefersDarkTheme()
                onToggled: UserSession.prefersDarkTheme = checked
            }
        }
        Label {
            text: "浅色/深色会立即应用到主界面；已登录时主题写入账号。"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            font.pixelSize: 11
            color: Window.window ? Window.window.colorTextMuted : "#7f7f85"
            font.family: "微软雅黑 Light"
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Button {
                text: "退出登录"
                enabled: UserSession.loggedIn
                font.family: "微软雅黑 Light"
                onClicked: {
                    UserSession.logout()
                    root.close()
                }
                background: Rectangle {
                    implicitWidth: 88
                    implicitHeight: 36
                    radius: 6
                    color: parent.down ? "#3d2028" : "transparent"
                    border.width: 1
                    border.color: Window.window ? Window.window.colorAccent : "#e93b63"
                }
                contentItem: Label {
                    text: parent.text
                    font: parent.font
                    color: Window.window ? Window.window.colorAccent : "#e93b63"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            Item { Layout.fillWidth: true }
            Button {
                text: "关闭"
                font.family: "微软雅黑 Light"
                onClicked: root.close()
            }
        }
    }
}
