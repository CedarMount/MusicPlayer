import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtGraphicalEffects 1.15
import "./Src/leftPage"
import "./Src/rightPage"
import "./Src/playMusic"
import "./Src/commonUI"
import "./Src/basic"
import "./Src/mainPopups"

MPWindow {
    id: window
    x: 300
    y: 30
    width: 1317
    height: 933

    Connections{
        target: BasicConfig
        function onOpenLoginPopup(){
            logInPopup.open()
        }
    }

    Connections{
        target: BasicConfig
        function onOpenRegistPopup(){
            registPopup.open()
        }
    }

    Connections {
        target: BasicConfig
        function onOpenProfilePopup() {
            profilePopup.open()
        }
    }

    LeftPage{
        id: leftRect
        width: 255
        anchors.top: parent.top
        anchors.bottom: bottomRect.top
        anchors.left: parent.left
        color: window.colorNavBg
    }

    RightPage{
        id: rightRect
        anchors.left: leftRect.right
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: bottomRect.top
        color: window.colorMainBg
    }

    PlayMusic{
        id: bottomRect
        height: 100 + (AppPlayer.hasLyrics ? 108 : 0)
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        color: window.colorPlayerBg
    }

    //登录弹窗
    LoginPopup{
        id: logInPopup
    }

    //注册弹窗
    RegistPopup{
        id: registPopup
    }

    ProfilePopup {
        id: profilePopup
    }
}
