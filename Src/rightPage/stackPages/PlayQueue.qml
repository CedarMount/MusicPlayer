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
        spacing: 16

        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4
                Label {
                    text: "播放队列"
                    font.pixelSize: 22
                    font.family: "微软雅黑 Light"
                    color: Window.window ? Window.window.colorTextPrimary : "white"
                }
                Label {
                    text: "当前待播顺序列表；单击选中，双击播放并打开歌词。支持顶部搜索过滤。从队列移除时，若该曲在完整曲库中则会同时从磁盘与数据库移除（与「我的歌单」不同）。"
                    font.pixelSize: 11
                    font.family: "微软雅黑 Light"
                    color: Window.window ? Window.window.colorTextMuted : "#5c5c66"
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }

            Button {
                id: removeBtn
                text: "从列表移除"
                font.family: "微软雅黑 Light"
                enabled: AppPlayer.queueFilteredTrackCount > 0 && listView.currentIndex >= 0
                onClicked: {
                    AppPlayer.removeSongAtQueueFilterRow(listView.currentIndex)
                    if (listView.currentIndex >= AppPlayer.queueFilteredTrackCount)
                        listView.currentIndex = Math.max(0, AppPlayer.queueFilteredTrackCount - 1)
                }

                background: Rectangle {
                    implicitWidth: 110
                    implicitHeight: 36
                    radius: 8
                    color: removeBtn.enabled
                        ? (Window.window ? Window.window.colorSecondaryBtn : "#2d2d37")
                        : (Window.window ? Window.window.colorSecondaryBtnDisabled : "#25252c")
                    border.width: 1
                    border.color: Window.window ? Window.window.colorBorder : "#45454e"
                }
                contentItem: Label {
                    text: removeBtn.text
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: removeBtn.enabled
                        ? (Window.window ? Window.window.colorTextPrimary : "#ddd")
                        : (Window.window ? Window.window.colorTextMuted : "#666")
                    font: removeBtn.font
                }
            }
        }

        Label {
            visible: AppPlayer.trackCount === 0
            text: "队列为空。请在「音乐曲库」上传或扫描曲目后再试。"
            font.pixelSize: 14
            font.family: "微软雅黑 Light"
            color: Window.window ? Window.window.colorTextMuted : "#7f7f85"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Label {
            visible: AppPlayer.trackCount > 0 && AppPlayer.queueFilteredTrackCount === 0
            text: "没有匹配当前搜索条件的歌曲。"
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
            model: AppPlayer.queueDisplayModel
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
                    if (index === AppPlayer.currentQueueDisplayRow)
                        return Window.window ? Window.window.colorRowNowPlaying : "#2d2430"
                    if (rowMa.containsMouse)
                        return Window.window ? Window.window.colorCardHover : "#1e1e28"
                    return "transparent"
                }
                border.width: (index === AppPlayer.currentQueueDisplayRow || index === listView.currentIndex) ? 1 : 0
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
                        color: Window.window ? Window.window.colorTextMuted : "#5c5c66"
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
                        AppPlayer.playIndexFromQueueFilterRow(index)
                        BasicConfig.pushLyricsPage()
                    }
                }
            }
        }
    }
}
