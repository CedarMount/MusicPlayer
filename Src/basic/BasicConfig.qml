pragma Singleton
import QtQuick 2.15

QtObject {
    signal blankAreaClicked()   //空白区域被点击
    signal openLoginPopup()     //打开登录弹窗
    signal openRegistPopup()    //打开注册弹窗
    signal openProfilePopup()   //用户信息 / 主题
    signal switchPage(string pageName)      //切换页面
    signal pushLyricsPage()                 // 右侧 StackView 进入歌词页
    signal pushCategoryTracksPage(string title)
    signal pushPlaylistTracksPage(string title, int playlistId)
}
