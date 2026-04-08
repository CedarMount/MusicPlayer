#ifndef MUSICCONTROLLER_H
#define MUSICCONTROLLER_H

#include <QObject>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QPair>
#include <QSqlDatabase>
#include <QVector>

#include "trackfiltermodel.h"
#include "tracklistmodel.h"

class QTimer;
class UserSession;
class PlaylistItemsModel;

class MusicController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(TrackListModel *trackModel READ trackModel CONSTANT)
    Q_PROPERTY(bool playing READ playing NOTIFY playingChanged)
    Q_PROPERTY(qint64 position READ position NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(QString currentTitle READ currentTitle NOTIFY currentTrackChanged)
    Q_PROPERTY(QString currentArtist READ currentArtist NOTIFY currentTrackChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(qreal volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(bool hasCurrentTrack READ hasCurrentTrack NOTIFY currentTrackChanged)
    /// 当前播放队列曲目数（与曲库总曲目数 libraryTrackCount 不同）
    Q_PROPERTY(int trackCount READ trackCount NOTIFY trackCountChanged)
    Q_PROPERTY(bool currentIsFavorite READ currentIsFavorite NOTIFY currentIsFavoriteChanged)
    Q_PROPERTY(QString musicStorageFolder READ musicStorageFolder CONSTANT)
    Q_PROPERTY(TrackFilterModel *displayTrackModel READ displayTrackModel CONSTANT)
    /// 播放队列专用过滤模型（与曲库列表数据源分离）
    Q_PROPERTY(TrackFilterModel *queueDisplayModel READ queueDisplayModel CONSTANT)
    Q_PROPERTY(int libraryTrackCount READ libraryTrackCount NOTIFY libraryTrackCountChanged)
    Q_PROPERTY(int queueFilteredTrackCount READ queueFilteredTrackCount NOTIFY queueFilteredTrackCountChanged)
    Q_PROPERTY(QString librarySearchFilter READ librarySearchFilter WRITE setLibrarySearchFilter NOTIFY librarySearchFilterChanged)
    Q_PROPERTY(int filteredTrackCount READ filteredTrackCount NOTIFY filteredTrackCountChanged)
    Q_PROPERTY(QString playbackModeLabel READ playbackModeLabel NOTIFY playbackModeChanged)
    Q_PROPERTY(int currentDisplayRow READ currentDisplayRow NOTIFY currentDisplayRowChanged)
    /// 当前播放在「播放队列」列表（queueDisplayModel）中的行；-1 表示不可见（如被搜索过滤掉）
    Q_PROPERTY(int currentQueueDisplayRow READ currentQueueDisplayRow NOTIFY currentDisplayRowChanged)
    Q_PROPERTY(bool hasLyrics READ hasLyrics NOTIFY lyricsChanged)
    Q_PROPERTY(QString lyricLinePrev READ lyricLinePrev NOTIFY lyricsChanged)
    Q_PROPERTY(QString lyricLineCurrent READ lyricLineCurrent NOTIFY lyricsChanged)
    Q_PROPERTY(QString lyricLineNext READ lyricLineNext NOTIFY lyricsChanged)
    Q_PROPERTY(QStringList lyricLines READ lyricLines NOTIFY lyricsChanged)
    Q_PROPERTY(int lyricHighlightIndex READ lyricHighlightIndex NOTIFY lyricsChanged)
    Q_PROPERTY(bool showingFullLibrary READ showingFullLibrary NOTIFY libraryViewModeChanged)
    Q_PROPERTY(int viewingPlaylistId READ viewingPlaylistId NOTIFY libraryViewModeChanged)
    Q_PROPERTY(QString viewingPlaylistTitle READ viewingPlaylistTitle NOTIFY libraryViewModeChanged)
    Q_PROPERTY(PlaylistItemsModel *playlistBrowseModel READ playlistBrowseModel CONSTANT)
    Q_PROPERTY(QString playbackError READ playbackError NOTIFY playbackErrorChanged)

public:
    explicit MusicController(QObject *parent = nullptr);
    ~MusicController() override;

    void setUserSession(UserSession *session);
    void setLibraryDatabase(const QSqlDatabase &db);
    void loadMusicLibraryFromDatabase();

    TrackListModel *trackModel() const { return m_queueModel; }
    TrackFilterModel *displayTrackModel() const { return m_trackFilter; }
    TrackFilterModel *queueDisplayModel() const { return m_queueFilter; }
    int libraryTrackCount() const { return m_libraryModel ? m_libraryModel->rowCount() : 0; }
    int queueFilteredTrackCount() const { return m_queueFilter ? m_queueFilter->rowCount() : 0; }
    QString musicStorageFolder() const { return m_musicRoot; }
    QString librarySearchFilter() const;
    void setLibrarySearchFilter(const QString &text);
    int filteredTrackCount() const;
    QString playbackModeLabel() const;
    int currentDisplayRow() const;
    int currentQueueDisplayRow() const;
    bool hasLyrics() const { return !m_lrcLines.isEmpty(); }
    QString lyricLinePrev() const { return m_lyricPrev; }
    QString lyricLineCurrent() const { return m_lyricCur; }
    QString lyricLineNext() const { return m_lyricNext; }
    QStringList lyricLines() const;
    int lyricHighlightIndex() const { return m_lyricHighlightIndex; }
    bool showingFullLibrary() const { return m_showingFullLibrary; }
    int viewingPlaylistId() const { return m_viewingPlaylistId; }
    QString viewingPlaylistTitle() const { return m_viewingPlaylistTitle; }
    PlaylistItemsModel *playlistBrowseModel() const { return m_playlistBrowseModel; }
    bool playing() const;
    qint64 position() const { return m_player->position(); }
    qint64 duration() const { return m_duration; }
    QString currentTitle() const { return m_currentTitle; }
    QString currentArtist() const { return m_currentArtist; }
    int currentIndex() const { return m_playlist->currentIndex(); }
    qreal volume() const;
    void setVolume(qreal v);
    bool hasCurrentTrack() const;
    int trackCount() const { return m_queueModel ? m_queueModel->rowCount() : 0; }
    bool currentIsFavorite() const;
    QString playbackError() const { return m_playbackError; }
    Q_INVOKABLE void clearPlaybackError();

    Q_INVOKABLE void uploadSongs();
    Q_INVOKABLE void removeSongAt(int row);
    Q_INVOKABLE void playIndex(int index);
    Q_INVOKABLE void playPause();
    Q_INVOKABLE void playNext();
    Q_INVOKABLE void playPrevious();
    Q_INVOKABLE void seek(qreal ratio);
    Q_INVOKABLE QString formatMs(qint64 ms) const;
    Q_INVOKABLE void playLocalPath(const QString &localPath);
    Q_INVOKABLE void toggleFavoriteCurrent();
    Q_INVOKABLE void cyclePlaybackMode();
    /// 在「音乐曲库 / 搜索 / 分类」等曲库视图中按过滤行播放（与当前播放队列无关时仍会定位或加入队列）
    Q_INVOKABLE void playIndexFromFilterRow(int filterRow);
    Q_INVOKABLE void playIndexFromQueueFilterRow(int filterRow);
    Q_INVOKABLE void removeSongAtLibraryFilterRow(int filterRow);
    Q_INVOKABLE void removeSongAtQueueFilterRow(int filterRow);
    Q_INVOKABLE void setCategoryBrowse(const QString &artist, const QString &album, const QString &genre);
    Q_INVOKABLE void clearCategoryBrowse();
    Q_INVOKABLE QStringList distinctLibraryArtists() const;
    Q_INVOKABLE QStringList distinctLibraryAlbums() const;
    Q_INVOKABLE QStringList distinctLibraryGenres() const;
    Q_INVOKABLE bool openUserPlaylistPlayback(int playlistId);
    /// 用当前账号「我的喜欢」顺序替换主播放队列（与 openUserPlaylistPlayback 类似）
    Q_INVOKABLE bool openFavoritesPlayback();
    Q_INVOKABLE void restoreFullMusicLibrary();
    /// 仅填充歌单浏览模型，不停止播放、不替换主曲库（用于栈内「索引」页）
    Q_INVOKABLE bool loadPlaylistBrowse(int playlistId);
    Q_INVOKABLE void clearPlaylistBrowse();
    Q_INVOKABLE void playFirstInPlaylistBrowse();
    /// 从当前栈内浏览的歌单中移除一条（仅删 playlist_items，不删曲库文件）
    Q_INVOKABLE bool removeTrackFromBrowsePlaylist(const QString &relPath);
    /// 顶部「搜索」：仅按歌名、歌手过滤并刷新列表
    Q_INVOKABLE void applySearchQuery(const QString &query);
    /// 回到曲库主页时恢复全量列表（含专辑/风格匹配模式）
    Q_INVOKABLE void clearSearchQuery();

signals:
    void playingChanged();
    void positionChanged();
    void durationChanged();
    void currentTrackChanged();
    void currentIndexChanged();
    void volumeChanged();
    void trackCountChanged();
    void libraryTrackCountChanged();
    void queueFilteredTrackCountChanged();
    void currentIsFavoriteChanged();
    void librarySearchFilterChanged();
    void filteredTrackCountChanged();
    void playbackModeChanged();
    void currentDisplayRowChanged();
    void lyricsChanged();
    void libraryViewModeChanged();
    void playbackErrorChanged();

private slots:
    void onPlayerStateChanged(QMediaPlayer::State state);
    void onDurationChanged(qint64 duration);
    void onPlaylistIndexChanged();
    void onPlayerMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onMetaDataChanged();
    void refreshCurrentFavoriteFlag();
    void onUserLoginStateChanged();
    void onProxyLayoutChanged();
    void onQueueProxyLayoutChanged();

private:
    void submitCurrentTrackAsRecent();
    void maybeRecordRecentPlay(const QString &localPath, const QString &title, const QString &artist);
    void ensureMusicRoot();
    static QStringList audioFilenameFilters();
    void purgeMissingLibraryRows();
    void importLooseFilesIntoLibrary();
    void updateCurrentMetadata();
    void scheduleApplyMetaFromPlayer();
    void applyMetaDataFromPlayer();
    static QString unknownArtist();
    QString currentLocalPath() const;
    int findRowForStoragePath(const QString &anyLocalPath) const;
    int findLibraryRowForLocalPath(const QString &anyLocalPath) const;
    void reloadLrcForCurrentTrack();
    void updateLyricForPosition(qint64 posMs);
    void onPlaybackLoadFailed();
    void markPlayCommandTime();
    void syncQueueSearchFilterFromLibrary();
    void removeFirstLibraryRowMatchingRel(const QString &rel);
    int findQueueRowForLibraryRel(const QString &rel) const;

    QMediaPlayer *m_player;
    QMediaPlaylist *m_playlist;
    /// 当前播放队列（与 QMediaPlaylist 一一对应）
    TrackListModel *m_queueModel = nullptr;
    /// 本地曲库完整列表（不受歌单/喜欢播放影响）
    TrackListModel *m_libraryModel = nullptr;
    qint64 m_duration;
    QString m_currentTitle;
    QString m_currentArtist;
    QTimer *m_metaDebounce = nullptr;
    UserSession *m_userSession = nullptr;
    QSqlDatabase m_libraryDb;
    QString m_musicRoot;
    TrackFilterModel *m_trackFilter = nullptr;
    TrackFilterModel *m_queueFilter = nullptr;
    QMediaPlaylist::PlaybackMode m_playbackMode = QMediaPlaylist::Sequential;

    QVector<QPair<qint64, QString>> m_lrcLines;
    QString m_lyricPrev;
    QString m_lyricCur;
    QString m_lyricNext;
    QTimer *m_lyricTick = nullptr;
    int m_lyricHighlightIndex = -1;

    bool m_showingFullLibrary = true;
    int m_viewingPlaylistId = 0;
    QString m_viewingPlaylistTitle;
    PlaylistItemsModel *m_playlistBrowseModel = nullptr;
    int m_playlistBrowsePlaylistId = 0;

    QString m_lastRecentDedupePath;
    qint64 m_lastRecentDedupeMs = 0;

    QString m_playbackError;
    int m_lastKnownPlaylistIndex = -1;
    int m_indexBeforePlaylistChange = -1;
    bool m_inPlaybackFailureRecovery = false;
    qint64 m_lastPlayCommandMs = 0;
};

#endif // MUSICCONTROLLER_H
