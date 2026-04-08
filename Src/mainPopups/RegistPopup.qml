import QtQuick 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15
import "../basic"


//注册弹窗
Popup{
    id: registPopup
    property string localFormError: ""
    onOpened: {
        localFormError = ""
        UserSession.clearLastError()
    }
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
        //返回登录弹窗按钮
        Image {
            id: back_login
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 30
            anchors.rightMargin: 30
            source: "qrc:/img/Resources/left_arrow_2yb1ya71hm1a_32.png"
            property color overlayColor: "#75777f"
            layer.enabled: true
            layer.effect: ColorOverlay{
                source: "back_login"
                color: back_login.overlayColor
            }
            MouseArea{
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    back_login.overlayColor = "white"
                    cursorShape = Qt.PointingHandCursor
                }
                onExited: {
                    back_login.overlayColor = "#75777f"
                    cursorShape = Qt.ArrowCursor
                }
                onClicked: {
                    registPopup.close()
                }
            }
        }
        //标题
        Label{
            id: registText
            text: "注册"
            color: "white"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: back_login.verticalCenter
            font.bold: true
            font.family: "黑体"
            font.pixelSize: 32
        }
        Row{
            id: titleRowItems1
            spacing: 10
            anchors.top: registText.bottom
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
            id: registColumnItems
            anchors.top: titleRowItems1.bottom
            anchors.topMargin: 60
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 30
            //账号输入框
            TextField{
                id: regist_accountTextField
                width: 400
                height: 40
                font.pixelSize: height/2
                font.family: "微软雅黑 Light"
                color: "#d9d9da"
                placeholderText: "请注册账号"
                placeholderTextColor: "#a1a1a3"
                verticalAlignment: Text.AlignVCenter
                background: Rectangle{
                    anchors.fill: parent
                    radius: regist_accountTextField.height/2
                    color: "#222"
                    border.width: 1
                    border.color: regist_accountTextField.focus?"#222":"#303037"
                }
            }
            //密码输入框
            TextField{
                id: regist_passwordTextField
                width: 400
                height: 40
                font.pixelSize: height/2
                font.family: "微软雅黑 Light"
                color: "#d9d9da"
                placeholderText: "请注册密码"
                placeholderTextColor: "#a1a1a3"
                verticalAlignment: Text.AlignVCenter
                echoMode: TextInput.Password
                background: Rectangle{
                    anchors.fill: parent
                    radius: regist_passwordTextField.height/2
                    color: "#222"
                    border.width: 1
                    border.color: regist_passwordTextField.focus?"#222":"#303037"
                }
            }
            //密码确认框
            TextField{
                id: confirm_passwordTextField
                width: 400
                height: 40
                font.pixelSize: height/2
                font.family: "微软雅黑 Light"
                color: "#d9d9da"
                placeholderText: "请确认密码"
                placeholderTextColor: "#a1a1a3"
                verticalAlignment: Text.AlignVCenter
                echoMode: TextInput.Password
                background: Rectangle{
                    anchors.fill: parent
                    radius: confirm_passwordTextField.height/2
                    color: "#222"
                    border.width: 1
                    border.color: confirm_passwordTextField.focus?"#222":"#303037"
                }
            }
        }
        Label {
            id: registErrorLabel
            anchors.horizontalCenter: registColumnItems.horizontalCenter
            anchors.top: registColumnItems.bottom
            anchors.topMargin: 16
            width: 400
            height: (localFormError.length + UserSession.lastError.length) > 0 ? implicitHeight : 0
            wrapMode: Text.WordWrap
            visible: (localFormError.length + UserSession.lastError.length) > 0
            text: localFormError.length ? localFormError : UserSession.lastError
            color: "#e85570"
            font.pixelSize: 13
            font.family: "微软雅黑 Light"
        }
        //注册按钮
        Rectangle{
            id: registBtn1
            height: 50
            width: 400
            anchors.top: registErrorLabel.bottom
            anchors.topMargin: registErrorLabel.visible ? 12 : 40
            radius: height/2
            anchors.horizontalCenter: registColumnItems.horizontalCenter
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
                text: "注册"
                font.pixelSize: 24
                font.family: "微软雅黑 Light"
                anchors.centerIn: parent
            }
            MouseArea{
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    registBtn1.opacity = 0.8
                    cursorShape = Qt.PointingHandCursor
                }
                onExited: {
                    registBtn1.opacity = 1
                    cursorShape = Qt.ArrowCursor
                }
                onClicked: {
                    localFormError = ""
                    UserSession.clearLastError()
                    if (regist_passwordTextField.text !== confirm_passwordTextField.text) {
                        localFormError = "两次输入的密码不一致"
                        return
                    }
                    if (UserSession.registerAccount(regist_accountTextField.text, regist_passwordTextField.text)) {
                        registPopup.close()
                        BasicConfig.openLoginPopup()
                    }
                }
            }
        }
    }
}