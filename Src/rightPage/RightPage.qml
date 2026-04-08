import QtQuick 2.15
import QtQuick.Window 2.15
import QtGraphicalEffects 1.15
import QtQuick.Controls 2.15
import "../basic"
import "./stackPages"

Rectangle{
    id: rightRect

    function topStackItem() {
        if (mainStackView.depth < 1)
            return null
        return mainStackView.get(mainStackView.depth - 1)
    }

    function musicStackOnTop() {
        const top = topStackItem()
        if (!top)
            return false
        const n = top.objectName
        return n === "musicLibraryHome" || n === "searchResultsPage" || n === "lyricsPage"
            || n === "categoryTracksPage" || n === "playlistTracksPage"
    }

    function runSearchFromToolbar() {
        var q = searchTextField.text.trim()
        if (!q.length) {
            UserSession.reloadSearchHistory()
            search_popup.open()
            return
        }
        UserSession.recordSearchQuery(q)
        AppPlayer.applySearchQuery(q)
        if (!musicStackOnTop()) {
            mainStackView.clear()
            mainStackView.push("qrc:/Src/rightPage/stackPages/MusicLibraryHome.qml")
            mainStackView.push("qrc:/Src/rightPage/stackPages/SearchResultsPage.qml")
            return
        }
        var top = topStackItem()
        if (top && top.objectName === "lyricsPage")
            mainStackView.pop()
        top = topStackItem()
        if (top && top.objectName === "searchResultsPage")
            return
        mainStackView.push("qrc:/Src/rightPage/stackPages/SearchResultsPage.qml")
    }

    function popStackOne() {
        if (mainStackView.depth <= 1)
            return
        mainStackView.pop()
        if (mainStackView.depth === 1) {
            AppPlayer.clearSearchQuery()
            searchTextField.text = ""
        }
    }

    function resetStackToPage(url) {
        searchTextField.text = ""
        AppPlayer.clearSearchQuery()
        mainStackView.clear()
        mainStackView.push(url)
    }

    //右侧页面顶部
    Item{
        id: rightPage_top
        anchors.right: parent.right
        anchors.left: parent.left
        anchors.top: parent.top
        height: 60
        //顶部右半部分
        Row{
            id: rightPage_top_left
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 0.02*window.width
            spacing: 15
            //返回
            Rectangle{
                id: back1
                width: 24
                height: 35
                radius: 4
                color: "transparent"
                border.width: 1
                border.color: "#2b2b31"
                Image {
                    id: back1_img
                    scale: 0.036
                    anchors.centerIn: parent
                    source: "qrc:/img/Resources/left_arrow_w8wv4is89unx_512.png"
                    property color overlayerColor: "#75777f"
                    layer.enabled: true
                    layer.effect: ColorOverlay{
                        source:back1_img
                        color: back1_img.overlayerColor
                    }
                    MouseArea{
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: {
                            back1_img.overlayerColor = "white"
                            back1.border.color = "white"
                        }
                        onExited: {
                            back1_img.overlayerColor = "#75777f"
                            back1.border.color = "#2b2b31"
                        }
                        onClicked: rightRect.popStackOne()
                    }
                }
            }
            //搜索框
            TextField{
                id: searchTextField
                height: back1.height
                width: 260
                leftPadding: 35
                color: "white"
                placeholderText: "输入歌名或歌手，点「搜索」"
                font.pixelSize: 16
                font.family: "微软雅黑 Light"
                Connections{
                    target: BasicConfig
                    function onBlankAreaClicked(){
                        innerRec.gradientStopPos = 1
                    }
                }
                background: Rectangle{
                    anchors.fill: parent
                    radius: 8
                    gradient: Gradient{
                        orientation: Gradient.Horizontal
                        GradientStop{color: "#21283d";position: 0}
                        GradientStop{color: "#382635";position: 1}
                    }
                    Rectangle{
                        id: innerRec
                        anchors.fill: parent
                        anchors.margins: 1
                        property real gradientStopPos: 1
                        gradient: Gradient{
                            orientation: Gradient.Horizontal
                            GradientStop{color: "#1a1d29";position: 0}
                            GradientStop{color: "#241c26";position: innerRec.gradientStopPos}
                        }
                    }
                    Image{
                        id: search_img
                        scale: 0.033
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: -240
                        source: "qrc:/img/Resources/seo_nuil7ltlrho1_512.png"
                        property color overlayColor: "#75777f"
                        layer.enabled: true
                        layer.effect: ColorOverlay{
                            source: search_img
                            color: search_img.overlayColor
                        }
                        MouseArea{
                            anchors.fill: parent
                            onClicked: {
                                innerRec.gradientStopPos = 0
                                UserSession.reloadSearchHistory()
                                search_popup.open()
                            }
                        }
                    }
                }
                //搜索历史框
                Popup{
                    id: search_popup
                    width: parent.width
                    height: 400
                    y: searchTextField.height + 10
                    onOpened: UserSession.reloadSearchHistory()
                    background: Rectangle{
                        anchors.fill: parent
                        radius: 10
                        color: "#2d2d37"
                        Item {
                            id: histroyItem
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.top: parent.top
                            anchors.topMargin: 20
                            anchors.leftMargin: 20
                            Label{
                                id: searchLabel
                                color: "#7f7f85"
                                text: "搜索历史"
                                font.pixelSize: 18
                                font.family: "微软雅黑 Light"
                            }
                            //删除搜索历史按钮
                            Image {
                                id: removeHistroy_img
                                scale: 1.5
                                anchors.right: parent.right
                                anchors.rightMargin: 20
                                anchors.verticalCenter: searchLabel.verticalCenter
                                source: "qrc:/img/Resources/delete_5pyp42c3s0zy_16.png"
                                property color overlayColor: "#7f7f85"
                                layer.enabled: true
                                layer.effect: ColorOverlay{
                                    source: removeHistroy_img
                                    color: removeHistroy_img.overlayColor
                                }
                                MouseArea{
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    onEntered: {
                                        removeHistroy_img.overlayColor = "white"
                                        cursorShape = Qt.PointingHandCursor
                                    }
                                    onExited: {
                                        removeHistroy_img.overlayColor = "#7f7f85"
                                        cursorShape = Qt.ArrowCursor
                                    }
                                    onClicked: UserSession.clearSearchHistory()
                                }
                            }
                        }
                        //搜索历史记录
                        Flow{
                            id:singFlow
                            anchors.top: histroyItem.top
                            anchors.left: histroyItem.left
                            anchors.right: histroyItem.right
                            anchors.topMargin: 40
                            spacing: 10
                            Repeater{
                                id: histroyRep
                                anchors.fill: parent
                                model: UserSession.searchHistory
                                delegate: Rectangle{
                                    width: dataLable.implicitWidth + 20
                                    height: 40
                                    border.width: 1
                                    border.color: "#45454e"
                                    color: "#2d2d37"
                                    radius: 15
                                    Label{
                                        id:dataLable
                                        text: modelData
                                        font.pixelSize: 20
                                        anchors.centerIn: parent
                                        color: "#ddd"
                                        font.family: "微软雅黑 Light"
                                        height: 25
                                    }
                                    MouseArea{
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onEntered: {
                                            dataLable.color = "white"
                                            parent.color = "#393943"
                                            cursorShape = Qt.PointingHandCursor
                                        }
                                        onExited: {
                                            dataLable.color = "#ddd"
                                            parent.color = "#2d2d37"
                                            cursorShape = Qt.ArrowCursor
                                        }
                                        onClicked: {
                                            searchTextField.text = modelData
                                            search_popup.close()
                                            rightRect.runSearchFromToolbar()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            Button {
                id: searchRunBtn
                text: "搜索"
                height: back1.height
                font.family: "微软雅黑 Light"
                font.pixelSize: 14
                onClicked: rightRect.runSearchFromToolbar()
                background: Rectangle {
                    implicitWidth: 64
                    radius: 8
                    color: searchRunBtn.down ? "#c42d52" : "#e93b63"
                }
                contentItem: Label {
                    text: searchRunBtn.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font: searchRunBtn.font
                }
            }
        }
        //顶部左半部分
        Row{
            id: rightPage_top_right
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 0.02*window.width
            spacing: 15
            //用户（已登录：头像 + 昵称；未登录：默认图标 +「未登录」）
            Rectangle{
                id: user1
                width: 27
                height: width
                radius: width/2
                color: "#2d2d37"
                clip: true

                Image {
                    id: userAvatarImg
                    anchors.fill: parent
                    visible: UserSession.loggedIn && UserSession.avatarUrl.length > 0
                    source: visible ? UserSession.avatarUrl : ""
                    fillMode: Image.PreserveAspectCrop
                    smooth: true
                    asynchronous: true
                }
                Image {
                    id: user1_img1
                    anchors.centerIn: parent
                    width: UserSession.loggedIn && UserSession.avatarUrl.length > 0 ? 0 : 18
                    height: width
                    fillMode: Image.PreserveAspectFit
                    visible: width > 0
                    source: "qrc:/img/Resources/user_sq6d5lnpzo1p_512.png"
                    property color overlayColor: "#75777f"
                    layer.enabled: true
                    layer.effect: ColorOverlay{
                        source: user1_img1
                        color: user1_img1.overlayColor
                    }
                }
                MouseArea{
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        if (user1_img1.visible) {
                            user1_img1.overlayColor = "white"
                        } else {
                            user1.border.width = 1
                            user1.border.color = "#ffffff"
                        }
                    }
                    onExited: {
                        if (user1_img1.visible)
                            user1_img1.overlayColor = "#75777f"
                        user1.border.width = 0
                    }
                    onClicked: {
                        if (UserSession.loggedIn)
                            BasicConfig.openProfilePopup()
                        else
                            BasicConfig.openLoginPopup()
                    }
                }
                border.width: 0
                border.color: "transparent"
            }
            //未登录 / 昵称（displayName，无昵称则为账号）（已登录点击打开与头像相同的资料弹窗）
            Text {
                id: logInText
                text: UserSession.loggedIn ? UserSession.displayName : "未登录"
                width: UserSession.loggedIn ? 160 : implicitWidth
                elide: UserSession.loggedIn ? Text.ElideRight : Text.ElideNone
                color: "#75777f"
                font.pixelSize: 14
                font.family: "微软雅黑 Light"
                anchors.verticalCenter: user1.verticalCenter
                MouseArea{
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        parent.color = "white"
                    }
                    onExited: {
                        parent.color = "#75777f"
                    }
                    onClicked: {
                        if (UserSession.loggedIn)
                            BasicConfig.openProfilePopup()
                        else
                            BasicConfig.openLoginPopup()
                    }
                }
            }
            //最小化
            Rectangle{
                id: min1
                anchors.verticalCenter: parent.verticalCenter
                width: 20
                height: 4
                color: "#75777f"
                MouseArea{
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        parent.color = "white"
                    }
                    onExited: {
                        parent.color = "#75777f"
                    }
                    onClicked: {
                        window.showMinimized()
                    }
                }
            }
            //最大化
            Rectangle{
                id: max1
                anchors.verticalCenter: parent.verticalCenter
                width: 20
                height: 20
                radius: 2
                border.width: 2
                border.color: "#75777f"
                color: "transparent"
                MouseArea{
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        parent.border.color = "white"
                    }
                    onExited: {
                        parent.border.color = "#75777f"
                    }
                    onClicked: {
                        if (window.visibility === Window.Maximized)
                            window.showNormal()
                        else
                            window.showMaximized()
                    }
                }
            }
            //关闭
            Image {
                id: close1
                width: 30
                height: 30
                anchors.verticalCenter: parent.verticalCenter
                source: 'qrc:/img/Resources/close_w4ifjubscsqi_512.png'
                property color overlayColor: "#75777f"
                layer.enabled: true
                layer.effect: ColorOverlay{
                    source: close1
                    color: close1.overlayColor
                }
                MouseArea{
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        close1.overlayColor = "white"
                    }
                    onExited: {
                        close1.overlayColor = "#75777f"
                    }
                    onClicked: {
                        Qt.quit()
                    }
                }
            }
        }
    }
    //右侧页面主要部分
    StackView{
        id: mainStackView
        anchors.top: rightPage_top.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 100
        clip: true
        initialItem: "qrc:/Src/rightPage/stackPages/MusicLibraryHome.qml"
        Connections{
            target: BasicConfig
            function onSwitchPage(pageName){
                if(pageName === "LocalMusic"){
                    rightRect.resetStackToPage("qrc:/Src/rightPage/stackPages/MusicLibraryHome.qml")
                }
                else if(pageName === "RecentlyPlayer"){
                    rightRect.resetStackToPage("qrc:/Src/rightPage/stackPages/RecentlyPlayer.qml")
                }
                else if(pageName === "MyLove"){
                    rightRect.resetStackToPage("qrc:/Src/rightPage/stackPages/MyLove.qml")
                }
                else if(pageName === "PlayQueue"){
                    rightRect.resetStackToPage("qrc:/Src/rightPage/stackPages/PlayQueue.qml")
                }
                else if(pageName === "MusicCategory"){
                    rightRect.resetStackToPage("qrc:/Src/rightPage/stackPages/MusicCategory.qml")
                }
                else if(pageName === "PlaylistsHub"){
                    rightRect.resetStackToPage("qrc:/Src/rightPage/stackPages/PlaylistsHub.qml")
                }
            }
            function onPushLyricsPage(){
                mainStackView.push("qrc:/Src/rightPage/stackPages/NowPlayingLyrics.qml")
            }
            function onPushCategoryTracksPage(title) {
                mainStackView.push("qrc:/Src/rightPage/stackPages/BrowseTrackListPage.qml", {
                    browseTitle: title,
                    browseKind: "category"
                })
            }
            function onPushPlaylistTracksPage(title, playlistId) {
                mainStackView.push("qrc:/Src/rightPage/stackPages/BrowseTrackListPage.qml", {
                    browseTitle: title,
                    browseKind: "playlist",
                    browsePlaylistId: playlistId
                })
            }
        }
    }
}
