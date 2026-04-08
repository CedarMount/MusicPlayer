import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import BasicConfig 1.0
import QtGraphicalEffects 1.15

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

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Label {
                text: "最近播放"
                font.pixelSize: 22
                font.family: "微软雅黑 Light"
                color: Window.window ? Window.window.colorTextPrimary : "white"
                Layout.fillWidth: true
            }

            Button {
                id: clearRecentBtn
                visible: UserSession.loggedIn && UserSession.recentTrackCount > 0
                text: "清空列表"
                font.family: "微软雅黑 Light"
                font.pixelSize: 13
                onClicked: UserSession.clearRecentPlays()

                background: Rectangle {
                    implicitWidth: 88
                    implicitHeight: 32
                    radius: 8
                    color: clearRecentBtn.down
                        ? (Window.window && Window.window.appDark ? "#3a3a45" : "#cfd5e0")
                        : (Window.window ? Window.window.colorSecondaryBtn : "#2d2d37")
                    border.width: 1
                    border.color: Window.window ? Window.window.colorBorder : "#45454e"
                }
                contentItem: Label {
                    text: clearRecentBtn.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: Window.window ? Window.window.colorTextPrimary : "#ddd"
                    font: clearRecentBtn.font
                }
            }
        }

        Label {
            visible: !UserSession.loggedIn
            text: "登录后，将按账号记录你播放过的歌曲（同一台电脑、不同用户数据相互独立）。"
            font.pixelSize: 14
            font.family: "微软雅黑 Light"
            color: Window.window ? Window.window.colorTextMuted : "#7f7f85"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Label {
            visible: UserSession.loggedIn && UserSession.recentTrackCount > 0
            text: "单击选中，双击播放并打开歌词页；右侧图标可移除该条记录。"
            font.pixelSize: 11
            font.family: "微软雅黑 Light"
            color: Window.window ? Window.window.colorTextMuted : "#5c5c66"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Label {
            visible: UserSession.loggedIn && UserSession.recentTrackCount === 0
            text: "暂无记录。在「音乐曲库」播放歌曲后，会出现在这里。"
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
            model: UserSession.recentTracks
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
                    if (playMa.containsMouse || delMa.containsMouse)
                        return Window.window ? Window.window.colorCardHover : "#1e1e28"
                    return "transparent"
                }
                border.width: (index === listView.currentIndex) ? 1 : 0
                border.color: "#6b9fff"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 8
                    spacing: 4

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
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

                        MouseArea {
                            id: playMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: listView.currentIndex = index
                            onDoubleClicked: {
                                listView.currentIndex = index
                                AppPlayer.playLocalPath(model.path)
                                BasicConfig.pushLyricsPage()
                            }
                        }
                    }

                    Item {
                        Layout.preferredWidth: 36
                        Layout.preferredHeight: 36
                        Layout.alignment: Qt.AlignVCenter

                        Image {
                            id: delImg
                            anchors.centerIn: parent
                            width: 16
                            height: 16
                            source: "qrc:/img/Resources/delete_5pyp42c3s0zy_16.png"
                            fillMode: Image.PreserveAspectFit
                            property color overlayColor: delMa.containsMouse
                                ? (Window.window ? Window.window.colorAccent : "#e93b63")
                                : (Window.window ? Window.window.colorTextMuted : "#7f7f85")
                            layer.enabled: true
                            layer.effect: ColorOverlay {
                                source: delImg
                                color: delImg.overlayColor
                            }
                        }

                        MouseArea {
                            id: delMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: UserSession.removeRecentPlay(model.path)
                        }
                    }
                }
            }
        }
    }
}
