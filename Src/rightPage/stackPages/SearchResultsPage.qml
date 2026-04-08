import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import BasicConfig 1.0

Rectangle {
    id: root
    objectName: "searchResultsPage"
    color: Window.window ? Window.window.colorMainBg : "#13131a"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 4
            Label {
                text: "搜索结果"
                font.pixelSize: 22
                font.family: "微软雅黑 Light"
                color: Window.window ? Window.window.colorTextPrimary : "white"
            }
            Label {
                text: "按歌名、歌手匹配；单击选中，双击播放并进入歌词页。使用顶部「返回」可回到上一页。"
                font.pixelSize: 12
                font.family: "微软雅黑 Light"
                color: Window.window ? Window.window.colorTextMuted : "#7f7f85"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        Label {
            visible: AppPlayer.libraryTrackCount > 0 && AppPlayer.filteredTrackCount === 0
            text: "没有匹配「" + AppPlayer.librarySearchFilter + "」的歌曲，请修改关键词后再次搜索。"
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
                        return "#2d2430"
                    if (rowMa.containsMouse)
                        return Window.window ? Window.window.colorCardHover : "#1e1e28"
                    return "transparent"
                }
                border.width: (index === AppPlayer.currentDisplayRow || index === listView.currentIndex) ? 1 : 0
                border.color: index === listView.currentIndex ? "#6b9fff" : "#e93b63"

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
