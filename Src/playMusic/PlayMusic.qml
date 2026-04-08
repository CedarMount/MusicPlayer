import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15
import BasicConfig 1.0

Rectangle {
    id: bottomRect
    color: Window.window ? Window.window.colorPlayerBg : "#2d2d37"

    property bool progressPressed: false
    property real progressRatioWhileDrag: 0

    // 左侧：歌曲信息
    Item {
        id: leftSection
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        width: 300
        height: 80

        Rectangle {
            id: albumCover
            width: 60
            height: 60
            radius: 4
            color: "#45454e"
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter

            Image {
                anchors.fill: parent
                source: "qrc:/img/Resources/music_t3h2tgcd0au4_32.png"
                fillMode: Image.PreserveAspectFit
            }
        }

        Column {
            anchors.left: albumCover.right
            anchors.leftMargin: 15
            anchors.verticalCenter: parent.verticalCenter
            spacing: 5
            width: parent.width - albumCover.width - 15

            Label {
                text: AppPlayer.currentTitle
                color: Window.window ? Window.window.colorTextPrimary : "white"
                font.pixelSize: 16
                font.family: "微软雅黑 Light"
                font.bold: true
                elide: Text.ElideRight
                width: parent.width
            }

            Label {
                text: AppPlayer.currentArtist
                color: Window.window ? Window.window.colorTextMuted : "#a1a1a3"
                font.pixelSize: 12
                font.family: "微软雅黑 Light"
                elide: Text.ElideRight
                width: parent.width
            }

            Label {
                visible: AppPlayer.playbackError.length > 0
                width: parent.width
                wrapMode: Text.WordWrap
                maximumLineCount: 4
                elide: Text.ElideRight
                text: AppPlayer.playbackError
                font.pixelSize: 11
                font.family: "微软雅黑 Light"
                color: "#ff8a8a"
                ToolTip.visible: tipMa.containsMouse && AppPlayer.playbackError.length > 0
                ToolTip.text: AppPlayer.playbackError

                MouseArea {
                    id: tipMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: AppPlayer.clearPlaybackError()
                }
            }
        }
    }

    // 中间：进度条 + 歌词（与音频同目录同名 .lrc）
    Item {
        id: progressSection
        anchors.left: leftSection.right
        anchors.right: rightSection.left
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        height: AppPlayer.hasLyrics ? 168 : 80

        Column {
            anchors.fill: parent
            spacing: 6

            Item {
                width: parent.width
                height: AppPlayer.hasLyrics ? 100 : 0
                visible: AppPlayer.hasLyrics

                ListView {
                    id: lyricList
                    anchors.fill: parent
                    clip: true
                    spacing: 4
                    interactive: false
                    model: AppPlayer.lyricLines
                    currentIndex: AppPlayer.lyricHighlightIndex
                    highlightRangeMode: ListView.ApplyRange
                    preferredHighlightBegin: height * 0.32
                    preferredHighlightEnd: height * 0.68
                    delegate: Label {
                        width: lyricList.width
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        text: modelData
                        font.pixelSize: index === AppPlayer.lyricHighlightIndex ? 13 : 11
                        font.bold: index === AppPlayer.lyricHighlightIndex
                        font.family: "微软雅黑 Light"
                        color: index === AppPlayer.lyricHighlightIndex
                            ? (Window.window ? Window.window.colorTextPrimary : "#ffffff")
                            : (Window.window ? Window.window.colorTextMuted : "#5c5c66")
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: BasicConfig.pushLyricsPage()
                }
            }

            Rectangle {
                id: progressSlider
                color: "#4d4d56"
                height: 6
                radius: height / 2
                width: parent.width

                property real value: {
                    if (bottomRect.progressPressed)
                        return bottomRect.progressRatioWhileDrag
                    if (!AppPlayer.hasCurrentTrack || AppPlayer.duration <= 0)
                        return 0
                    return AppPlayer.position / AppPlayer.duration
                }
                readonly property real maxValue: 1

                Rectangle {
                    id: progressContentRect
                    anchors.left: parent.left
                    anchors.top: parent.top
                    radius: parent.height / 2
                    anchors.bottom: parent.bottom
                    color: "#e93b63"
                    width: progressSlider.width * progressSlider.value
                }

                Rectangle {
                    id: currentPosRect
                    height: parent.height + 8
                    width: height
                    radius: width / 2
                    color: "#e93b63"
                    visible: false
                    anchors.right: progressContentRect.right
                    anchors.rightMargin: -width / 2
                    anchors.verticalCenter: progressContentRect.verticalCenter
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: currentPosRect.visible = true
                    onExited: {
                        if (!pressed)
                            currentPosRect.visible = false
                    }
                    onPressed: {
                        bottomRect.progressPressed = true
                        bottomRect.progressRatioWhileDrag = Math.max(0, Math.min(1, mouseX / progressSlider.width))
                    }
                    onReleased: {
                        AppPlayer.seek(bottomRect.progressRatioWhileDrag)
                        bottomRect.progressPressed = false
                        if (!containsMouse)
                            currentPosRect.visible = false
                    }
                    onPositionChanged: {
                        if (pressed) {
                            bottomRect.progressRatioWhileDrag = Math.max(0, Math.min(1, mouseX / progressSlider.width))
                        }
                    }
                }
            }

            RowLayout {
                width: parent.width
                spacing: 8

                Label {
                    id: curTimeLabel
                    text: AppPlayer.formatMs(AppPlayer.position)
                    color: Window.window ? Window.window.colorTextMuted : "#a1a1a3"
                    font.pixelSize: 12
                    font.family: "微软雅黑 Light"
                }

                Item {
                    Layout.fillWidth: true
                    Layout.minimumWidth: 4
                }

                Label {
                    id: totalTimeLabel
                    text: AppPlayer.formatMs(AppPlayer.duration)
                    color: Window.window ? Window.window.colorTextMuted : "#a1a1a3"
                    font.pixelSize: 12
                    font.family: "微软雅黑 Light"
                }
            }
        }
    }

    // 右侧：控制按钮
    Row {
        id: rightSection
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        spacing: 20

        ControlButton {
            id: prevBtn
            iconSource: "qrc:/img/Resources/previous_9kzdc9ujd96m_32.png"
            tooltipText: "上一首"
            onClicked: AppPlayer.playPrevious()
        }

        ControlButton {
            id: playBtn
            iconSource: "qrc:/img/Resources/play_shlymgjj9i8v_32.png"
            tooltipText: AppPlayer.playing ? "暂停" : "播放"
            usePauseBarsWhenPlaying: true
            isPlaying: AppPlayer.playing
            onClicked: AppPlayer.playPause()
        }

        ControlButton {
            id: nextBtn
            iconSource: "qrc:/img/Resources/next_button_y8sb5ntkpuyx_32.png"
            tooltipText: "下一首"
            onClicked: AppPlayer.playNext()
        }

        Rectangle {
            width: 1
            height: 30
            color: "#45454e"
        }

        ControlButton {
            id: favoriteBtn
            iconSource: AppPlayer.currentIsFavorite
                ? "qrc:/img/Resources/heart_q3emnu6ucq65_32.png"
                : "qrc:/img/Resources/heart_fos2dkdz7xg8_32.png"
            tooltipText: UserSession.loggedIn
                ? (AppPlayer.currentIsFavorite ? "取消喜欢" : "加入我的喜欢")
                : "登录后使用喜欢"
            enabled: UserSession.loggedIn && AppPlayer.hasCurrentTrack
            opacity: enabled ? 1 : 0.4
            isFavorite: AppPlayer.currentIsFavorite
            onClicked: AppPlayer.toggleFavoriteCurrent()
        }

        ControlButton {
            id: modeBtn
            iconSource: "qrc:/img/Resources/loop_dpenn2wrrl6l_32.png"
            tooltipText: AppPlayer.playbackModeLabel + "（点击切换）"
            onClicked: AppPlayer.cyclePlaybackMode()
        }

        Rectangle {
            width: 1
            height: 30
            color: "#45454e"
        }

        Row {
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            ControlButton {
                id: volumeBtn
                iconSource: "qrc:/img/Resources/volume_d5m1z3kicyav_32.png"
                tooltipText: "音量"
            }

            Rectangle {
                id: volumeBar
                width: 80
                height: 4
                radius: 2
                color: "#4d4d56"
                anchors.verticalCenter: parent.verticalCenter

                property bool dragging: false

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    color: "#e93b63"
                    width: parent.width * AppPlayer.volume
                }

                MouseArea {
                    anchors.fill: parent
                    onPressed: {
                        volumeBar.dragging = true
                        AppPlayer.volume = Math.max(0, Math.min(1, mouseX / volumeBar.width))
                    }
                    onReleased: volumeBar.dragging = false
                    onPositionChanged: {
                        if (pressed)
                            AppPlayer.volume = Math.max(0, Math.min(1, mouseX / volumeBar.width))
                    }
                }
            }
        }
    }
}
