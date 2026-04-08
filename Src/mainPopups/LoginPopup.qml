import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15
import "../basic"

//登录弹窗
Popup{
    id: logInPopup
    onOpened: UserSession.clearLastError()
    anchors.centerIn: parent
    width: 466
    height: 638
    clip: true
    closePolicy: Popup.NoAutoClose
    background: Rectangle{
        anchors.fill: parent
        color: "#1b1b23"
        radius: 10
        border.width: 1
        border.color: "#75777f"
        //关闭登陆弹窗按钮
        Image {
            id: logInPopup_close
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 30
            anchors.rightMargin: 30
            source: "qrc:/img/Resources/close_v8b3szi716rs_32.png"
            property color overlayColor: "#75777f"
            layer.enabled: true
            layer.effect: ColorOverlay{
                source: "logInPopup_close"
                color: logInPopup_close.overlayColor
            }
            MouseArea{
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    logInPopup_close.overlayColor = "white"
                    cursorShape = Qt.PointingHandCursor
                }
                onExited: {
                    logInPopup_close.overlayColor = "#75777f"
                    cursorShape = Qt.ArrowCursor
                }
                onClicked: {
                    logInPopup.close()
                }
            }
        }
        //标题
        Label{
            id: loginText
            text: "登录"
            color: "white"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: logInPopup_close.verticalCenter
            font.bold: true
            font.family: "黑体"
            font.pixelSize: 32
        }
        Row{
            id: titleRowItems
            spacing: 10
            anchors.top: loginText.bottom
            anchors.topMargin: 60
            anchors.horizontalCenter: parent.horizontalCenter
            Image {
                id: music_img
                source: "qrc:/img/Resources/music_t3h2tgcd0au4_32.png"
            }
            Label{
                color: "white"
                font.bold: true
                text: "MusicPlayer"
                font.pixelSize: 32
                font.family: "微软雅黑 Light"
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        Column{
            id: loginColumnItems
            anchors.top: titleRowItems.bottom
            anchors.topMargin: 60
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 30
            //账号输入框
            TextField{
                id: accountTextField
                width: 400
                height: 40
                font.pixelSize: height/2
                font.family: "微软雅黑 Light"
                color: "#d9d9da"
                placeholderText: "请输入账号"
                placeholderTextColor: "#a1a1a3"
                verticalAlignment: Text.AlignVCenter
                background: Rectangle{
                    anchors.fill: parent
                    radius: accountTextField.height/2
                    color: "#222"
                    border.width: 1
                    border.color: accountTextField.focus?"#222":"#303037"
                }
            }
            //密码输入框
            TextField{
                id: passwordTextField
                width: 400
                height: 40
                font.pixelSize: height/2
                font.family: "微软雅黑 Light"
                color: "#d9d9da"
                placeholderText: "请输入密码"
                placeholderTextColor: "#a1a1a3"
                verticalAlignment: Text.AlignVCenter
                echoMode: TextInput.Password
                background: Rectangle{
                    anchors.fill: parent
                    radius: passwordTextField.height/2
                    color: "#222"
                    border.width: 1
                    border.color: passwordTextField.focus?"#222":"#303037"
                }
            }
        }
        Label {
            id: loginErrorLabel
            anchors.horizontalCenter: loginColumnItems.horizontalCenter
            anchors.top: loginColumnItems.bottom
            anchors.topMargin: 16
            width: 400
            height: text.length > 0 ? implicitHeight : 0
            wrapMode: Text.WordWrap
            visible: text.length > 0
            text: UserSession.lastError
            color: "#e85570"
            font.pixelSize: 13
            font.family: "微软雅黑 Light"
        }
        //登录按键
        Rectangle{
            id: loginBtn
            height: 50
            width: 400
            anchors.top: loginErrorLabel.bottom
            anchors.topMargin: loginErrorLabel.text.length > 0 ? 12 : 40
            radius: height/2
            anchors.horizontalCenter: loginColumnItems.horizontalCenter
            gradient: Gradient{
                orientation: Gradient.Horizontal
                GradientStop{
                    color: "#e93b63"
                    position: 0
                }
                GradientStop{
                    color: "#e84f50"
                    position: 1
                }
            }
            Label{
                color: "white"
                text: "登录"
                font.pixelSize: 24
                font.family: "微软雅黑 Light"
                anchors.centerIn: parent
            }
            MouseArea{
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    loginBtn.opacity = 0.8
                    cursorShape = Qt.PointingHandCursor
                }
                onExited: {
                    loginBtn.opacity = 1
                    cursorShape = Qt.ArrowCursor
                }
                onClicked: {
                    UserSession.clearLastError()
                    if (UserSession.login(accountTextField.text, passwordTextField.text))
                        logInPopup.close()
                }
            }
        }
        //注册按钮
        Label{
            id: registBtn
            color: "#818186"
            text: "注册"
            font.pixelSize: 20
            font.family: "微软雅黑 Light"
            anchors.top: loginBtn.bottom
            anchors.topMargin: 40
            anchors.horizontalCenter: loginBtn.horizontalCenter
            MouseArea{
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    registBtn.color = "white"
                    cursorShape = Qt.PointingHandCursor
                }
                onExited: {
                    registBtn.color = "#818186"
                    cursorShape = Qt.ArrowCursor
                }
                onClicked: {
                    BasicConfig.openRegistPopup()
                }
            }
        }
    }
}