#include <QApplication>
#include <QCoreApplication>
#include <QFont>
#include <QMetaType>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQml>

#include "musiccontroller.h"
#include "trackfiltermodel.h"
#include "tracklistmodel.h"
#include "usertracksmodels.h" // NamedPlaylistsModel for qmlRegisterUncreatableType
#include "usersession.h"

int main(int argc, char *argv[])
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    QCoreApplication::setApplicationName(QStringLiteral("MusicPlayer"));
    QCoreApplication::setOrganizationName(QStringLiteral("MusicPlayer"));

    QApplication app(argc, argv);
    {
        QFont uiFont;
        uiFont.setFamily(QStringLiteral("微软雅黑 Light"));
        app.setFont(uiFont);
    }

    // QML 读取 UserSession.recentTracks / favoriteTracks 指针属性前，必须注册类型
    qRegisterMetaType<RecentTracksModel *>("RecentTracksModel*");
    qRegisterMetaType<FavoriteTracksModel *>("FavoriteTracksModel*");
    qRegisterMetaType<NamedPlaylistsModel *>("NamedPlaylistsModel*");
    qRegisterMetaType<PlaylistItemsModel *>("PlaylistItemsModel*");
    qRegisterMetaType<TrackListModel *>("TrackListModel*");
    qRegisterMetaType<TrackFilterModel *>("TrackFilterModel*");
    qmlRegisterUncreatableType<RecentTracksModel>(
        "MusicPlayer.Models",
        1,
        0,
        "RecentTracksModel",
        QStringLiteral("Use UserSession.recentTracks"));
    qmlRegisterUncreatableType<FavoriteTracksModel>(
        "MusicPlayer.Models",
        1,
        0,
        "FavoriteTracksModel",
        QStringLiteral("Use UserSession.favoriteTracks"));
    qmlRegisterUncreatableType<NamedPlaylistsModel>(
        "MusicPlayer.Models",
        1,
        0,
        "NamedPlaylistsModel",
        QStringLiteral("Use UserSession.namedPlaylists"));
    qmlRegisterUncreatableType<PlaylistItemsModel>(
        "MusicPlayer.Models",
        1,
        0,
        "PlaylistItemsModel",
        QStringLiteral("Use AppPlayer.playlistBrowseModel"));
    qmlRegisterUncreatableType<TrackListModel>(
        "MusicPlayer.Models",
        1,
        0,
        "TrackListModel",
        QStringLiteral("Use AppPlayer.trackModel"));
    qmlRegisterUncreatableType<TrackFilterModel>(
        "MusicPlayer.Models",
        1,
        0,
        "TrackFilterModel",
        QStringLiteral("Use AppPlayer.displayTrackModel / queueDisplayModel"));

    UserSession userSession;
    if (!userSession.openDatabase())
        qWarning("User database could not be opened; login/favorites/recent will be unavailable.");

    MusicController musicController;
    musicController.setUserSession(&userSession);
    if (userSession.sqlDatabase().isOpen()) {
        musicController.setLibraryDatabase(userSession.sqlDatabase());
        musicController.loadMusicLibraryFromDatabase();
    }

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("AppPlayer"), &musicController);
    engine.rootContext()->setContextProperty(QStringLiteral("UserSession"), &userSession);

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    qmlRegisterSingletonType(
        QUrl(QStringLiteral("qrc:/Src/basic/BasicConfig.qml")),
        "BasicConfig",
        1,
        0,
        "BasicConfig");
    engine.load(url);

    return QCoreApplication::exec();
}
