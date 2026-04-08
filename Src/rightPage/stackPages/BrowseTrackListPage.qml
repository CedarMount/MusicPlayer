import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15
import BasicConfig 1.0

Rectangle {
    id: root
    property string browseTitle: ""
    /// "category" | "playlist"
    property string browseKind: "category"
    /// 命名歌单 id；分类浏览为 -1
    property int browsePlaylistId: -1

    objectName: browseKind === "playlist" ? "playlistTracksPage" : "categoryTracksPage"
    color: Window.window ? Window.window.colorMainBg : "#13131a"

    Component.onDestruction: {
        if (browseKind === "playlist")
            AppPlayer.clearPlaylistBrowse()
        else
            AppPlayer.clearCategoryBrowse()
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            Label {
                text: browseKind === "playlist" ? "歌单曲目" : "分类曲目"
                font.pixelSize: 22
                font.family: "微软雅黑 Light"
                color: Window.window ? Window.window.colorTextPrimary : "white"
            }
            Label {
                text: browseTitle
                font.pixelSize: 14
                font.family: "微软雅黑 Light"
                color: Window.window ? Window.window.colorTextMuted : "#7f7f85"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            Label {
                text: browseKind === "playlist"
                    ? "仅作索引，不中断当前播放；单击选中，双击可播放并打开歌词。右侧图标可将曲目从此歌单移除（不删曲库文件）。顶部「返回」关闭。"
                    : "单击选中，双击播放并打开歌词。顶部「返回」关闭此页并清除分类筛选。"
                font.pixelSize: 11
                font.family: "微软雅黑 Light"
                color: Window.window ? Window.window.colorTextMuted : "#5c5c66"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ListView {
                id: trackListView
                anchors.fill: parent
                clip: true
                model: browseKind === "playlist" ? AppPlayer.playlistBrowseModel : AppPlayer.displayTrackModel
                spacing: 2
                currentIndex: -1
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                delegate: Rectangle {
                width: trackListView.width
                height: 52
                radius: 8
                color: {
                    if (index === trackListView.currentIndex)
                        return Window.window ? Window.window.colorCardHover : "#2a3040"
                    if (browseKind === "category" && index === AppPlayer.currentDisplayRow)
                        return "#2d2430"
                    if (rowMa.containsMouse || delMa.containsMouse)
                        return Window.window ? Window.window.colorCardHover : "#1e1e28"
                    return "transparent"
                }
                border.width: (browseKind === "category" && index === AppPlayer.currentDisplayRow
                    || index === trackListView.currentIndex) ? 1 : 0
                border.color: index === trackListView.currentIndex ? "#6b9fff" : "#e93b63"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 16
                    anchors.rightMargin: (root.browseKind === "playlist" && root.browsePlaylistId > 0) ? 8 : 16
                    spacing: 8

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        RowLayout {
                            anchors.fill: parent
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
                            onClicked: trackListView.currentIndex = index
                            onDoubleClicked: {
                                trackListView.currentIndex = index
                                if (root.browseKind === "playlist") {
                                    if (path && path.length > 0)
                                        AppPlayer.playLocalPath(path)
                                } else {
                                    AppPlayer.playIndexFromFilterRow(index)
                                }
                                if (root.browseKind !== "playlist" || (path && path.length > 0))
                                    BasicConfig.pushLyricsPage()
                            }
                        }
                    }

                    Item {
                        Layout.preferredWidth: (root.browseKind === "playlist" && root.browsePlaylistId > 0) ? 36 : 0
                        Layout.preferredHeight: 36
                        Layout.alignment: Qt.AlignVCenter
                        visible: Layout.preferredWidth > 0

                        Image {
                            id: delImg
                            anchors.centerIn: parent
                            width: 16
                            height: 16
                            source: "qrc:/img/Resources/delete_5pyp42c3s0zy_16.png"
                            fillMode: Image.PreserveAspectFit
                            property color overlayColor: delMa.containsMouse ? "#e93b63" : "#7f7f85"
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
                            onClicked: {
                                if (root.browseKind === "playlist" && root.browsePlaylistId > 0
                                        && relPath && relPath.length > 0)
                                    AppPlayer.removeTrackFromBrowsePlaylist(relPath)
                            }
                        }
                    }
                }
            }
            }

            Label {
                anchors.centerIn: parent
                width: parent.width - 48
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                visible: trackListView.count === 0
                text: browseKind === "playlist"
                    ? "歌单为空或文件已不在曲库目录。"
                    : (AppPlayer.libraryTrackCount === 0 ? "曲库暂无曲目。" : "没有符合该分类的曲目。")
                font.pixelSize: 14
                font.family: "微软雅黑 Light"
                color: "#7f7f85"
            }
        }
    }
}
