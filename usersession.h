#ifndef USERSESSION_H
#define USERSESSION_H

#include <QObject>
#include <QSqlDatabase>

#include "usertracksmodels.h"

class UserSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loggedIn READ isLoggedIn NOTIFY loginStateChanged)
    Q_PROPERTY(QString currentUsername READ currentUsername NOTIFY loginStateChanged)
    Q_PROPERTY(int currentUserId READ currentUserId NOTIFY loginStateChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(RecentTracksModel *recentTracks READ recentTracks CONSTANT)
    Q_PROPERTY(FavoriteTracksModel *favoriteTracks READ favoriteTracks CONSTANT)
    Q_PROPERTY(int recentTrackCount READ recentTrackCount NOTIFY trackListsChanged)
    Q_PROPERTY(int favoriteTrackCount READ favoriteTrackCount NOTIFY trackListsChanged)
    Q_PROPERTY(QString displayName READ displayName NOTIFY profileChanged)
    Q_PROPERTY(QString avatarUrl READ avatarUrl NOTIFY profileChanged)
    Q_PROPERTY(bool prefersDarkTheme READ prefersDarkTheme WRITE setPrefersDarkTheme NOTIFY prefersDarkThemeChanged)
    Q_PROPERTY(NamedPlaylistsModel *namedPlaylists READ namedPlaylists CONSTANT)
    Q_PROPERTY(int namedPlaylistCount READ namedPlaylistCount NOTIFY playlistsModelChanged)
    Q_PROPERTY(QStringList searchHistory READ searchHistory NOTIFY searchHistoryChanged)

public:
    explicit UserSession(QObject *parent = nullptr);
    ~UserSession() override;

    bool isLoggedIn() const { return m_userId > 0; }
    QString currentUsername() const { return m_username; }
    int currentUserId() const { return m_userId; }
    QString lastError() const { return m_lastError; }

    RecentTracksModel *recentTracks() const { return m_recentModel; }
    FavoriteTracksModel *favoriteTracks() const { return m_favoriteModel; }
    int recentTrackCount() const;
    int favoriteTrackCount() const;
    QString displayName() const;
    QString avatarUrl() const;
    bool prefersDarkTheme() const;
    void setPrefersDarkTheme(bool dark);
    NamedPlaylistsModel *namedPlaylists() const { return m_namedPlaylists; }
    int namedPlaylistCount() const;
    QStringList searchHistory() const { return m_searchHistory; }

    /// 已打开的 SQLite 连接（与 userdata.db 同库）；未 openDatabase 时无效
    QSqlDatabase sqlDatabase() const { return m_db; }

    Q_INVOKABLE bool openDatabase();
    Q_INVOKABLE bool registerAccount(const QString &username, const QString &password);
    Q_INVOKABLE bool login(const QString &username, const QString &password);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void clearLastError();

    bool isFavoritePath(const QString &localPath) const;
    Q_INVOKABLE void setFavorite(const QString &localPath, bool favorite, const QString &title,
                                 const QString &artist);
    Q_INVOKABLE void toggleFavorite(const QString &localPath, const QString &title,
                                    const QString &artist);
    void recordRecentPlay(const QString &localPath, const QString &title, const QString &artist);
    Q_INVOKABLE void removeRecentPlay(const QString &localPath);
    Q_INVOKABLE void clearRecentPlays();

    /// 从「喜欢 / 最近」中移除指向已删除文件的记录（扫描全部账号；登录时会刷新列表）
    Q_INVOKABLE void pruneStaleFavoritesAndRecent();

    /// 按与 favorites / recent_plays 表一致的存储键删除（曲库删歌、扫描缺失文件时调用）
    void removeFavoritesRecentForStorageKey(const QString &pathKey);

    Q_INVOKABLE bool saveNickname(const QString &nickname);
    Q_INVOKABLE bool pickAvatar();
    Q_INVOKABLE bool createNamedPlaylist(const QString &name);
    Q_INVOKABLE void deleteNamedPlaylist(int playlistId);
    Q_INVOKABLE bool renameNamedPlaylist(int playlistId, const QString &name);
    Q_INVOKABLE bool addTrackToNamedPlaylist(int playlistId, const QString &relPath, const QString &title,
                                             const QString &artist);
    Q_INVOKABLE bool removeTrackFromNamedPlaylist(int playlistId, const QString &relPath);

    Q_INVOKABLE void recordSearchQuery(const QString &query);
    Q_INVOKABLE void clearSearchHistory();
    Q_INVOKABLE void reloadSearchHistory();

    /// 与 favorites / recent_plays 表中的 path 键一致（供播放列表去重、点歌匹配）
    static QString storagePathKey(const QString &localPath);

signals:
    void loginStateChanged();
    void lastErrorChanged();
    void favoritesChanged();
    void trackListsChanged();
    void profileChanged();
    void prefersDarkThemeChanged();
    void playlistsModelChanged();
    void searchHistoryChanged();

private:
    void setLastError(const QString &e);
    bool createTables();
    void migrateSchema();
    /// 旧库若无 (user_id,path) 主键，INSERT OR REPLACE 会不断插入重复行，此处去重并重建表
    void migrateRecentPlaysIfNeeded();
    /// canonical 在部分盘符/工作目录下可能为空，需回退到 cleanPath(absolute)
    static QString normalizeStoragePath(const QString &localPath);
    static QByteArray randomSalt(int length = 16);
    static QByteArray hashPassword(const QString &password, const QByteArray &salt);

    void reloadTrackModels();

    QStringList m_searchHistory;

    QSqlDatabase m_db;
    QString m_connectionName;
    int m_userId = 0;
    QString m_username;
    QString m_lastError;

    RecentTracksModel *m_recentModel = nullptr;
    FavoriteTracksModel *m_favoriteModel = nullptr;
    NamedPlaylistsModel *m_namedPlaylists = nullptr;

    QString m_nickname;
    QString m_avatarPath;
    bool m_prefersDarkTheme = true;
};

#endif // USERSESSION_H
