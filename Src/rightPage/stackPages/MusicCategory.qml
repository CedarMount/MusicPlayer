import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import BasicConfig 1.0

Rectangle {
    id: root
    color: Window.window ? Window.window.colorMainBg : "#13131a"

    function refreshLists() {
        artistList.model = AppPlayer.distinctLibraryArtists()
        albumList.model = AppPlayer.distinctLibraryAlbums()
        genreList.model = AppPlayer.distinctLibraryGenres()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 12

        Label {
            text: "音乐分类"
            font.pixelSize: 22
            font.family: "微软雅黑 Light"
            color: Window.window ? Window.window.colorTextPrimary : "white"
        }
        Label {
            text: "点选下面任一项会在栈中打开曲目列表；在该页按顶部「返回」即清除分类筛选。可与顶部搜索关键词组合。数据来自文件标签与曲库字段。"
            font.pixelSize: 12
            font.family: "微软雅黑 Light"
            color: Window.window ? Window.window.colorTextMuted : "#7f7f85"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            background: Rectangle { color: "transparent" }
            TabButton {
                text: "歌手"
                font.family: "微软雅黑 Light"
            }
            TabButton {
                text: "专辑"
                font.family: "微软雅黑 Light"
            }
            TabButton {
                text: "风格"
                font.family: "微软雅黑 Light"
            }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            ListView {
                id: artistList
                clip: true
                model: []
                spacing: 2
                ScrollBar.vertical: ScrollBar {}
                delegate: Rectangle {
                    width: parent.width
                    height: 44
                    radius: 8
                    color: ma.containsMouse
                        ? (Window.window ? Window.window.colorCardHover : "#1e1e28")
                        : "transparent"
                    Label {
                        anchors.centerIn: parent
                        text: modelData
                        color: Window.window ? Window.window.colorTextPrimary : "white"
                        font.family: "微软雅黑 Light"
                    }
                    MouseArea {
                        id: ma
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            AppPlayer.setCategoryBrowse(modelData, "", "")
                            BasicConfig.pushCategoryTracksPage("歌手：" + modelData)
                        }
                    }
                }
            }
            ListView {
                id: albumList
                clip: true
                model: []
                spacing: 2
                ScrollBar.vertical: ScrollBar {}
                delegate: Rectangle {
                    width: parent.width
                    height: 44
                    radius: 8
                    color: ma.containsMouse
                        ? (Window.window ? Window.window.colorCardHover : "#1e1e28")
                        : "transparent"
                    Label {
                        anchors.centerIn: parent
                        text: modelData
                        color: Window.window ? Window.window.colorTextPrimary : "white"
                        font.family: "微软雅黑 Light"
                    }
                    MouseArea {
                        id: ma
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            AppPlayer.setCategoryBrowse("", modelData, "")
                            BasicConfig.pushCategoryTracksPage("专辑：" + modelData)
                        }
                    }
                }
            }
            ListView {
                id: genreList
                clip: true
                model: []
                spacing: 2
                ScrollBar.vertical: ScrollBar {}
                delegate: Rectangle {
                    width: parent.width
                    height: 44
                    radius: 8
                    color: ma.containsMouse
                        ? (Window.window ? Window.window.colorCardHover : "#1e1e28")
                        : "transparent"
                    Label {
                        anchors.centerIn: parent
                        text: modelData
                        color: Window.window ? Window.window.colorTextPrimary : "white"
                        font.family: "微软雅黑 Light"
                    }
                    MouseArea {
                        id: ma
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            AppPlayer.setCategoryBrowse("", "", modelData)
                            BasicConfig.pushCategoryTracksPage("风格：" + modelData)
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: refreshLists()

    Connections {
        target: AppPlayer
        function onTrackCountChanged() {
            root.refreshLists()
        }
    }
}
