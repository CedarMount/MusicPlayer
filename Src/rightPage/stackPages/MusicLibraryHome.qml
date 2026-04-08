import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import BasicConfig 1.0

Rectangle {
    id: root
    objectName: "musicLibraryHome"
    color: Window.window ? Window.window.colorMainBg : "#13131a"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                Label {
                    text: "音乐曲库"
                    font.pixelSize: 22
                    font.family: "微软雅黑 Light"
                    color: Window.window ? Window.window.colorTextPrimary : "white"
                }
                Label {
                    visible: AppPlayer.musicStorageFolder.length > 0
                    text: "曲库目录（数据库中的歌曲均来自此目录）：" + AppPlayer.musicStorageFolder
                    font.pixelSize: 11
                    font.family: "微软雅黑 Light"
                    color: "#5c5c66"
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }
                Label {
                    visible: AppPlayer.libraryTrackCount > 0
                    text: "单击曲目可选中，双击播放并打开歌词页。"
                    font.pixelSize: 11
                    font.family: "微软雅黑 Light"
                    color: "#5c5c66"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            Button {
                id: uploadBtn
                text: "上传歌曲"
                font.family: "微软雅黑 Light"
                onClicked: AppPlayer.uploadSongs()

                background: Rectangle {
                    implicitWidth: 120
                    implicitHeight: 36
                    radius: 8
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: "#e93b63" }
                        GradientStop { position: 1; color: "#e84f50" }
                    }
                }
                contentItem: Label {
                    text: uploadBtn.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: "white"
                    font: uploadBtn.font
                }
            }

            Button {
                id: addPlBtn
                text: "加入歌单"
                font.family: "微软雅黑 Light"
                enabled: UserSession.loggedIn && AppPlayer.filteredTrackCount > 0 && listView.currentIndex >= 0
                onClicked: joinPlPopup.open()

                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 36
                    radius: 8
                    color: addPlBtn.enabled ? "#2d2d37" : "#25252c"
                    border.width: 1
                    border.color: "#45454e"
                }
                contentItem: Label {
                    text: addPlBtn.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: addPlBtn.enabled ? "#ddd" : "#666"
                    font: addPlBtn.font
                }
            }

            Button {
                id: removeBtn
                text: "删除歌曲"
                font.family: "微软雅黑 Light"
                enabled: AppPlayer.filteredTrackCount > 0 && listView.currentIndex >= 0
                onClicked: {
                    AppPlayer.removeSongAtLibraryFilterRow(listView.currentIndex)
                    if (listView.currentIndex >= AppPlayer.filteredTrackCount)
                        listView.currentIndex = Math.max(0, AppPlayer.filteredTrackCount - 1)
                }

                background: Rectangle {
                    implicitWidth: 100
                    implicitHeight: 36
                    radius: 8
                    color: removeBtn.enabled ? "#2d2d37" : "#25252c"
                    border.width: 1
                    border.color: "#45454e"
                }
                contentItem: Label {
                    text: removeBtn.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: removeBtn.enabled ? "#ddd" : "#666"
                    font: removeBtn.font
                }
            }
        }

        Popup {
            id: joinPlPopup
            width: 320
            height: 360
            modal: true
            anchors.centerIn: Overlay.overlay
            padding: 16
            onOpened: plPickView.currentIndex = -1

            background: Rectangle {
                color: Window.window ? Window.window.colorNavBg : "#2d2d37"
                radius: 10
                border.width: 1
                border.color: Window.window ? Window.window.colorBorder : "#45454e"
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                Label {
                    text: "选择歌单"
                    font.pixelSize: 16
                    font.family: "微软雅黑 Light"
                    color: Window.window ? Window.window.colorTextPrimary : "white"
                }
                Label {
                    visible: !UserSession.loggedIn
                    text: "请先登录"
                    color: Window.window ? Window.window.colorTextMuted : "#7f7f85"
                    font.family: "微软雅黑 Light"
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                }
                ListView {
                    id: plPickView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    visible: UserSession.loggedIn
                    model: UserSession.namedPlaylists
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Rectangle {
                        width: plPickView.width
                        height: 44
                        radius: 6
                        color: rowMa.containsMouse
                            ? (Window.window ? Window.window.colorCardHover : "#1e1e28")
                            : "transparent"
                        Label {
                            anchors.centerIn: parent
                            text: model.name
                            color: Window.window ? Window.window.colorTextPrimary : "white"
                            font.family: "微软雅黑 Light"
                        }
                        MouseArea {
                            id: rowMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                const row = listView.currentIndex
                                if (row < 0)
                                    return
                                const rel = listView.currentItem.trackRel
                                const t = listView.currentItem.trackTitle
                                const a = listView.currentItem.trackArtist
                                if (UserSession.addTrackToNamedPlaylist(model.playlistId, rel, t, a))
                                    joinPlPopup.close()
                            }
                        }
                    }
                }
                Button {
                    text: "取消"
                    Layout.alignment: Qt.AlignRight
                    font.family: "微软雅黑 Light"
                    onClicked: joinPlPopup.close()
                }
            }
        }

        Label {
            visible: AppPlayer.libraryTrackCount === 0
            text: "数据库中暂无曲目。启动时会扫描曲库目录；也可点击「上传歌曲」添加（文件会复制到上述目录）。"
            font.pixelSize: 14
            font.family: "微软雅黑 Light"
            color: "#7f7f85"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        ListView {
            id: listView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: AppPlayer.displayTrackModel
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
                    if (index === AppPlayer.currentDisplayRow)
                        return Window.window ? Window.window.colorRowNowPlaying : "#2d2430"
                    if (rowMa.containsMouse)
                        return Window.window ? Window.window.colorCardHover : "#1e1e28"
                    return "transparent"
                }
                border.width: (index === AppPlayer.currentDisplayRow || index === listView.currentIndex) ? 1 : 0
                border.color: index === listView.currentIndex ? "#6b9fff" : "#e93b63"

                property string trackRel: libraryRelPath
                property string trackTitle: title
                property string trackArtist: artist

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: 16
                    spacing: 12

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Label {
                            text: title
                            font.pixelSize: 15
                            font.family: "微软雅黑 Light"
                            color: Window.window ? Window.window.colorTextPrimary : "white"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: artist
                            font.pixelSize: 12
                            font.family: "微软雅黑 Light"
                            color: Window.window ? Window.window.colorTextMuted : "#a1a1a3"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }

                    Label {
                        text: (index + 1).toString()
                        font.pixelSize: 12
                        color: "#5c5c66"
                        font.family: "微软雅黑 Light"
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
                        AppPlayer.playIndexFromFilterRow(index)
                        BasicConfig.pushLyricsPage()
                    }
                }
            }
        }
    }
}
