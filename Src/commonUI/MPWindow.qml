import QtQuick 2.15
import QtQuick.Window 2.15
import "../basic"

Window {
    id: window
    readonly property bool appDark: UserSession.prefersDarkTheme
    readonly property color colorMainBg: appDark ? "#13131a" : "#f4f6fb"
    readonly property color colorNavBg: appDark ? "#1a1a21" : "#e8ebf2"
    readonly property color colorPlayerBg: appDark ? "#2d2d37" : "#dde2ec"
    readonly property color colorTextPrimary: appDark ? "#ffffff" : "#1a1d26"
    readonly property color colorTextMuted: appDark ? "#a1a1a3" : "#5a6170"
    readonly property color colorAccent: "#e93b63"
    readonly property color colorCardHover: appDark ? "#1e1e28" : "#eceff5"
    readonly property color colorBorder: appDark ? "#45454e" : "#c8ced9"
    /// 列表「当前正在播放」行背景（与选中行区分）
    readonly property color colorRowNowPlaying: appDark ? "#2d2430" : "#f2e6ec"
    readonly property color colorSecondaryBtn: appDark ? "#2d2d37" : "#dde2ec"
    readonly property color colorSecondaryBtnDisabled: appDark ? "#25252c" : "#e2e5ec"

    visible: true
    title: qsTr("Hello World")
    flags: Qt.FramelessWindowHint | Qt.Window | Qt.WindowSystemMenuHint |
           Qt.WindowMaximizeButtonHint | Qt.WindowMinimizeButtonHint        //设置无边框属性

    MouseArea{
        anchors.fill: parent
        property point clickPos: "0,0"
        onPressed: function(mouse){
            clickPos = Qt.point(mouse.x,mouse.y)
            //console.log(clickPos)
        }
        onPositionChanged: function(mouse){
            let delta = Qt.point(mouse.x-clickPos.x,mouse.y-clickPos.y)
            window.x += delta.x
            window.y += delta.y
        }
        onClicked: {
            BasicConfig.blankAreaClicked()
        }
    }
}
