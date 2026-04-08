import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: root
    objectName: "lyricsPage"
    color: Window.window ? Window.window.colorMainBg : "#13131a"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            Label {
                text: "歌词"
                font.pixelSize: 22
                font.family: "微软雅黑 Light"
                color: Window.window ? Window.window.colorTextPrimary : "white"
            }
            Label {
                text: AppPlayer.currentTitle
                font.pixelSize: 16
                font.family: "微软雅黑 Light"
                font.bold: true
                color: Window.window ? Window.window.colorTextPrimary : "white"
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Label {
                text: AppPlayer.currentArtist
                font.pixelSize: 13
                font.family: "微软雅黑 Light"
                color: Window.window ? Window.window.colorTextMuted : "#a1a1a3"
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        Label {
            visible: !AppPlayer.hasLyrics
            text: "当前歌曲无 LRC 歌词。请将同名 .lrc 放在音频同目录（与上传规则一致）。"
            font.pixelSize: 14
            font.family: "微软雅黑 Light"
            color: "#7f7f85"
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        ListView {
            id: lyricList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: AppPlayer.hasLyrics
            spacing: 8
            model: AppPlayer.lyricLines
            currentIndex: AppPlayer.lyricHighlightIndex
            highlightRangeMode: ListView.ApplyRange
            preferredHighlightBegin: height * 0.38
            preferredHighlightEnd: height * 0.62
            ScrollBar.vertical: ScrollBar {}

            delegate: Label {
                width: lyricList.width
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: modelData
                font.pixelSize: index === AppPlayer.lyricHighlightIndex ? 18 : 14
                font.bold: index === AppPlayer.lyricHighlightIndex
                font.family: "微软雅黑 Light"
                color: index === AppPlayer.lyricHighlightIndex
                    ? (Window.window ? Window.window.colorAccent : "#e93b63")
                    : (Window.window ? Window.window.colorTextMuted : "#7f7f85")
            }
        }
    }
}
