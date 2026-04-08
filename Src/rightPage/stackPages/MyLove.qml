import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import BasicConfig 1.0

Rectangle {
    id: root
    color: Window.window ? Window.window.colorMainBg : "#13131a"

    Component.onCompleted: {
        if (UserSession.loggedIn)
            UserSession.pruneStaleFavoritesAndRecent()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: "我的喜欢"
            font.pixelSize: 22
            font.family: "微软雅黑 Light"
            color: Window.window ? Window.window.colorTextPrimary : "white"
            Layout.fillWidth: true
        }

        Label {
            visible: !UserSession.loggedIn
            text: "登录后，可使用播放条上的心形按钮收藏歌曲；每个账号的喜欢列表单独保存。"
            font.pixelSize: 14
            font.family: "微软雅黑 Light"
            color: Window.window ? Window.window.colorTextMuted : "#7f7f85"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        RowLayout {
            visible: UserSession.loggedIn && UserSession.favoriteTrackCount > 0
            Layout.fillWidth: true
            spacing: 12
            Label {
                text: "单击选中；双击从喜欢列表顺序播放该首并打开歌词。「播放全部喜欢」用喜欢顺序替换主队列。"
                font.pixelSize: 11
                font.family: "微软雅黑 Light"
                color: Window.window ? Window.window.colorTextMuted : "#5c5c66"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Button {
                id: playAllFavBtn
                text: "播放全部喜欢"
                font.family: "微软雅黑 Light"
                onClicked: {
                    if (AppPlayer.openFavoritesPlayback())
                        AppPlayer.playIndex(0)
                }
                background: Rectangle {
                    implicitWidth: 110
                    implicitHeight: 36
                    radius: 8
                    color: Window.window ? Window.window.colorSecondaryBtn : "#2d2d37"
                    border.width: 1
                    border.color: Window.window ? Window.window.colorBorder : "#45454e"
                }
                contentItem: Label {
                    text: playAllFavBtn.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: Window.window ? Window.window.colorTextPrimary : "#ddd"
                    font: playAllFavBtn.font
                }
            }
        }

        Label {
            visible: UserSession.loggedIn && UserSession.favoriteTrackCount === 0
            text: "还没有喜欢的歌曲。播放时点击底部「喜欢」即可加入。"
            font.pixelSize: 14
            font.family: "微软雅黑 Light"
            color: Window.window ? Window.window.colorTextMuted : "#7f7f85"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: UserSession.loggedIn
            model: UserSession.favoriteTracks
            spacing: 2
            currentIndex: -1
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: Rectangle {
                width: listView.width
                height: 52
                radius: 8
                color: {
                    if (index === listView.currentIndex)
                        return Window.window ? Window.window.colorCardHover : "#2a3040"
                    if (rowMa.containsMouse)
                        return Window.window ? Window.window.colorCardHover : "#1e1e28"
                    return "transparent"
                }
                border.width: index === listView.currentIndex ? 1 : 0
                border.color: "#6b9fff"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Label {
                            text: model.title
                            font.pixelSize: 15
                            font.family: "微软雅黑 Light"
                            color: Window.window ? Window.window.colorTextPrimary : "white"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: model.artist
                            font.pixelSize: 12
                            font.family: "微软雅黑 Light"
                            color: Window.window ? Window.window.colorTextMuted : "#a1a1a3"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }

                MouseArea {
                    id: rowMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: listView.currentIndex = index
                    onDoubleClicked: {
                        listView.currentIndex = index
                        if (AppPlayer.openFavoritesPlayback())
                            AppPlayer.playLocalPath(model.path)
                        BasicConfig.pushLyricsPage()
                    }
                }
            }
        }
    }
}
