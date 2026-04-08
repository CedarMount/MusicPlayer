import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import BasicConfig 1.0

Rectangle {
    id: root
    color: Window.window ? Window.window.colorMainBg : "#13131a"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        Label {
            text: "我的歌单"
            font.pixelSize: 22
            font.family: "微软雅黑 Light"
            color: Window.window ? Window.window.colorTextPrimary : "white"
        }
        Label {
            visible: UserSession.loggedIn
            text: "单击选中；「打开歌单」仅浏览曲目（不改变当前播放队列）。「播放此歌单」会用歌单顺序替换主播放队列，上一首/下一首按歌单走。"
            font.pixelSize: 12
            font.family: "微软雅黑 Light"
            color: Window.window ? Window.window.colorTextMuted : "#7f7f85"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
        Label {
            visible: !UserSession.loggedIn
            text: "登录后可创建、重命名、删除歌单，并将音乐曲库中的歌曲加入歌单。"
            font.pixelSize: 14
            font.family: "微软雅黑 Light"
            color: Window.window ? Window.window.colorTextMuted : "#7f7f85"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        RowLayout {
            visible: UserSession.loggedIn
            Layout.fillWidth: true
            spacing: 8
            TextField {
                id: newPlName
                Layout.fillWidth: true
                placeholderText: "新歌单名称"
                color: Window.window ? Window.window.colorTextPrimary : "white"
                font.family: "微软雅黑 Light"
                background: Rectangle {
                    radius: 6
                    color: Window.window ? Window.window.colorNavBg : "#2d2d37"
                    border.color: Window.window ? Window.window.colorBorder : "#45454e"
                }
            }
            Button {
                text: "新建"
                font.family: "微软雅黑 Light"
                onClicked: {
                    if (UserSession.createNamedPlaylist(newPlName.text))
                        newPlName.text = ""
                }
            }
        }

        RowLayout {
            visible: UserSession.loggedIn
            Layout.fillWidth: true
            spacing: 8
            TextField {
                id: renameField
                Layout.fillWidth: true
                placeholderText: "重命名为…"
                color: Window.window ? Window.window.colorTextPrimary : "white"
                font.family: "微软雅黑 Light"
                background: Rectangle {
                    radius: 6
                    color: Window.window ? Window.window.colorNavBg : "#2d2d37"
                    border.color: Window.window ? Window.window.colorBorder : "#45454e"
                }
            }
            Button {
                text: "重命名选中"
                font.family: "微软雅黑 Light"
                enabled: plView.currentIndex >= 0
                onClicked: {
                    var pid = plView.currentItem ? plView.currentItem.playlistIdVal : -1
                    if (pid > 0)
                        UserSession.renameNamedPlaylist(pid, renameField.text)
                }
            }
            Button {
                text: "删除选中"
                font.family: "微软雅黑 Light"
                enabled: plView.currentIndex >= 0
                onClicked: {
                    var pid = plView.currentItem ? plView.currentItem.playlistIdVal : -1
                    if (pid > 0)
                        UserSession.deleteNamedPlaylist(pid)
                }
            }
            Button {
                text: "打开歌单"
                font.family: "微软雅黑 Light"
                enabled: plView.currentIndex >= 0
                onClicked: {
                    var pid = plView.currentItem ? plView.currentItem.playlistIdVal : -1
                    var pname = plView.currentItem ? plView.currentItem.playlistNameText : ""
                    if (pid > 0 && AppPlayer.loadPlaylistBrowse(pid))
                        BasicConfig.pushPlaylistTracksPage(pname, pid)
                }
            }
            Button {
                text: "播放此歌单"
                font.family: "微软雅黑 Light"
                enabled: plView.currentIndex >= 0
                onClicked: {
                    var pid = plView.currentItem ? plView.currentItem.playlistIdVal : -1
                    var pname = plView.currentItem ? plView.currentItem.playlistNameText : ""
                    if (pid > 0 && AppPlayer.openUserPlaylistPlayback(pid)) {
                        AppPlayer.loadPlaylistBrowse(pid)
                        BasicConfig.pushPlaylistTracksPage(pname, pid)
                        AppPlayer.playIndex(0)
                    }
                }
            }
        }

        ListView {
            id: plView
            visible: UserSession.loggedIn
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: UserSession.namedPlaylists
            spacing: 2
            currentIndex: -1
            ScrollBar.vertical: ScrollBar {}

            delegate: Rectangle {
                width: plView.width
                height: 48
                radius: 8
                color: index === plView.currentIndex
                    ? (Window.window ? Window.window.colorCardHover : "#2a3040")
                    : (rowMa.containsMouse ? (Window.window ? Window.window.colorCardHover : "#1e1e28") : "transparent")
                border.width: index === plView.currentIndex ? 1 : 0
                border.color: Window.window ? Window.window.colorAccent : "#e93b63"

                property int playlistIdVal: model.playlistId
                property string playlistNameText: model.name

                Label {
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    text: model.name
                    color: Window.window ? Window.window.colorTextPrimary : "white"
                    font.pixelSize: 15
                    font.family: "微软雅黑 Light"
                }
                MouseArea {
                    id: rowMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: plView.currentIndex = index
                    onDoubleClicked: {
                        plView.currentIndex = index
                        if (AppPlayer.loadPlaylistBrowse(playlistIdVal))
                            BasicConfig.pushPlaylistTracksPage(model.name, playlistIdVal)
                    }
                }
            }
        }
    }
}
