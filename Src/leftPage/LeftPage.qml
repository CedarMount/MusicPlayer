import QtQuick 2.15
import QtQuick.Controls 2.15
import "../rightPage"
import "../basic"

Rectangle{
    id: leftRect
    //音乐播放器logo
    Rectangle{
        id: logoRec
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 80
        gradient: Gradient{
            orientation: Gradient.Horizontal
            GradientStop{color: "#21283d";position: 0}
            GradientStop{color: "#382635";position: 1}
        }
        Image {
            id: logo_img
            source: "qrc:/img/Resources/music_t3h2tgcd0au4_32.png"
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 20
        }
        Label{
            id: logo_label
            text: "MusicPlayer"
            font.pixelSize: 30
            color: "#ff6ec7"
            anchors.left: logo_img.right
            anchors.leftMargin: 10
            anchors.verticalCenter: logo_img.verticalCenter
            font.family: "楷体"
        }
    }
    Column{
        id: leftColumnItem
        anchors.top: logoRec.bottom
        anchors.topMargin: 40
        anchors.left: parent.left
        anchors.right: parent.right
        height: 680
        spacing: 16
        //音乐曲库
        Rectangle{
            id: localMusic
            height: 60
            width: 240
            radius: height/4
            anchors.horizontalCenter: leftColumnItem.horizontalCenter
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
            Image {
                id: localMusic_img
                scale: 1.0
                source: "qrc:/img/Resources/song_99tskq9rz1so_32.png"
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 20
            }
            Label{
                color: "white"
                text: "音乐曲库"
                font.pixelSize: 30
                font.family: "微软雅黑 Light"
                anchors.left: localMusic_img.right
                anchors.leftMargin: 30
                anchors.verticalCenter: localMusic_img.verticalCenter
            }
            MouseArea{
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    localMusic.opacity = 0.8
                    cursorShape = Qt.PointingHandCursor
                }
                onExited: {
                    localMusic.opacity = 1
                    cursorShape = Qt.ArrowCursor
                }
                onClicked: {
                    BasicConfig.switchPage("LocalMusic")
                }
            }
        }
        //音乐分类
        Rectangle{
            id: musicCategoryBtn
            height: 60
            width: 240
            radius: height/4
            anchors.horizontalCenter: leftColumnItem.horizontalCenter
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
            Image {
                id: musicCategory_img
                width: 32
                height: 32
                fillMode: Image.PreserveAspectFit
                source: "qrc:/img/Resources/seo_nuil7ltlrho1_512.png"
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 20
            }
            Label{
                color: "white"
                text: "音乐分类"
                font.pixelSize: 30
                font.family: "微软雅黑 Light"
                anchors.left: musicCategory_img.right
                anchors.leftMargin: 30
                anchors.verticalCenter: parent.verticalCenter
            }
            MouseArea{
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    musicCategoryBtn.opacity = 0.8
                    cursorShape = Qt.PointingHandCursor
                }
                onExited: {
                    musicCategoryBtn.opacity = 1
                    cursorShape = Qt.ArrowCursor
                }
                onClicked: {
                    BasicConfig.switchPage("MusicCategory")
                }
            }
        }
        //我的歌单
        Rectangle{
            id: playlistsHubBtn
            height: 60
            width: 240
            radius: height/4
            anchors.horizontalCenter: leftColumnItem.horizontalCenter
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
            Image {
                id: playlistsHub_img
                scale: 1
                source: "qrc:/img/Resources/my_location_zg4s2nk2q2y9_32.png"
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 20
            }
            Label{
                color: "white"
                text: "我的歌单"
                font.pixelSize: 30
                font.family: "微软雅黑 Light"
                anchors.left: playlistsHub_img.right
                anchors.leftMargin: 30
                anchors.verticalCenter: playlistsHub_img.verticalCenter
            }
            MouseArea{
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    playlistsHubBtn.opacity = 0.8
                    cursorShape = Qt.PointingHandCursor
                }
                onExited: {
                    playlistsHubBtn.opacity = 1
                    cursorShape = Qt.ArrowCursor
                }
                onClicked: {
                    BasicConfig.switchPage("PlaylistsHub")
                }
            }
        }
        //播放队列
        Rectangle{
            id: playQueueBtn
            height: 60
            width: 240
            radius: height/4
            anchors.horizontalCenter: leftColumnItem.horizontalCenter
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
            Image {
                id: playQueue_img
                scale: 1
                source: "qrc:/img/Resources/queue_2mwvfgp2mmp0_32.png"
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 20
            }
            Label{
                color: "white"
                text: "播放队列"
                font.pixelSize: 30
                font.family: "微软雅黑 Light"
                anchors.left: playQueue_img.right
                anchors.leftMargin: 30
                anchors.verticalCenter: playQueue_img.verticalCenter
            }
            MouseArea{
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    playQueueBtn.opacity = 0.8
                    cursorShape = Qt.PointingHandCursor
                }
                onExited: {
                    playQueueBtn.opacity = 1
                    cursorShape = Qt.ArrowCursor
                }
                onClicked: {
                    BasicConfig.switchPage("PlayQueue")
                }
            }
        }
        //最近播放
        Rectangle{
            id: recentlyPlayer
            height: 60
            width: 240
            radius: height/4
            anchors.horizontalCenter: leftColumnItem.horizontalCenter
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
            Image {
                id: recentlyPlayer_img
                scale: 1
                source: "qrc:/img/Resources/clock_aiaidwdgbcba_32.png"
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 20
            }
            Label{
                color: "white"
                text: "最近播放"
                font.pixelSize: 30
                font.family: "微软雅黑 Light"
                anchors.left: recentlyPlayer_img.right
                anchors.leftMargin: 30
                anchors.verticalCenter: recentlyPlayer_img.verticalCenter
            }
            MouseArea{
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    recentlyPlayer.opacity = 0.8
                    cursorShape = Qt.PointingHandCursor
                }
                onExited: {
                    recentlyPlayer.opacity = 1
                    cursorShape = Qt.ArrowCursor
                }
                onClicked: {
                    BasicConfig.switchPage("RecentlyPlayer")
                }
            }
        }
        //我的喜欢
        Rectangle{
            id: myLove
            height: 60
            width: 240
            radius: height/4
            anchors.horizontalCenter: leftColumnItem.horizontalCenter
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
            Image {
                id: myLove_img
                scale: 1
                source: "qrc:/img/Resources/heart_q3emnu6ucq65_32.png"
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 20
            }
            Label{
                color: "white"
                text: "我的喜欢"
                font.pixelSize: 30
                font.family: "微软雅黑 Light"
                anchors.left: myLove_img.right
                anchors.leftMargin: 30
                anchors.verticalCenter: myLove_img.verticalCenter
            }
            MouseArea{
                anchors.fill: parent
                hoverEnabled: true
                onEntered: {
                    myLove.opacity = 0.8
                    cursorShape = Qt.PointingHandCursor
                }
                onExited: {
                    myLove.opacity = 1
                    cursorShape = Qt.ArrowCursor
                }
                onClicked: {
                    BasicConfig.switchPage("MyLove")
                }
            }
        }
    }
}