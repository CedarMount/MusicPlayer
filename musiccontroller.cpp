#include "musiccontroller.h"
#include "tracklistmodel.h"
#include "usertracksmodels.h"
#include "usersession.h"

#include <QAbstractItemModel>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileDialog>
#include <QIODevice>
#include <QFileInfo>
#include <QMediaObject>
#include <QRegularExpression>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QSet>
#include <QSignalBlocker>
#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>
#include <QUrl>

#include <algorithm>

namespace {

/// 从本地音频读取内嵌标签（与播放时 applyMetaDataFromPlayer 使用的键一致）；不打扰主播放器
void readEmbeddedAudioMeta(const QString &localPath, QString &title, QString &artist, QString &album,
                           QString &genre)
{
    title.clear();
    artist.clear();
    album.clear();
    genre.clear();
    if (localPath.isEmpty() || !QFileInfo::exists(localPath))
        return;

    QMediaPlayer reader;
    QEventLoop loop;
    QTimer guard;
    guard.setSingleShot(true);
    QObject::connect(&guard, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(
        &reader,
        &QMediaPlayer::mediaStatusChanged,
        &loop,
        [&](QMediaPlayer::MediaStatus st) {
            if (st == QMediaPlayer::LoadedMedia || st == QMediaPlayer::BufferedMedia)
                QTimer::singleShot(80, &loop, &QEventLoop::quit);
            else if (st == QMediaPlayer::InvalidMedia)
                loop.quit();
        });

    guard.start(3000);
    reader.setMedia(QUrl::fromLocalFile(localPath));
    loop.exec();
    guard.stop();

    title = reader.metaData(QStringLiteral("Title")).toString().trimmed();
    artist = reader.metaData(QStringLiteral("ContributingArtist")).toString().trimmed();
    if (artist.isEmpty())
        artist = reader.metaData(QStringLiteral("AlbumArtist")).toString().trimmed();
    if (artist.isEmpty())
        artist = reader.metaData(QStringLiteral("Author")).toString().trimmed();
    if (artist.isEmpty())
        artist = reader.metaData(QStringLiteral("Artist")).toString().trimmed();
    album = reader.metaData(QStringLiteral("AlbumTitle")).toString().trimmed();
    if (album.isEmpty())
        album = reader.metaData(QStringLiteral("Album")).toString().trimmed();
    genre = reader.metaData(QStringLiteral("Genre")).toString().trimmed();
}

/// 上传前探测：Qt 多媒体后端能否加载（与播放一致）。超时则视为失败，避免把明显无法播放的文件写入曲库。
bool localFileLoadsWithQtMultimedia(const QString &localPath)
{
    if (localPath.isEmpty() || !QFileInfo::exists(localPath))
        return false;
    QMediaPlayer probe;
    QEventLoop loop;
    QTimer guard;
    guard.setSingleShot(true);
    QObject::connect(&guard, &QTimer::timeout, &loop, &QEventLoop::quit);
    bool ok = false;
    QObject::connect(
        &probe,
        &QMediaPlayer::mediaStatusChanged,
        &loop,
        [&](QMediaPlayer::MediaStatus st) {
            if (st == QMediaPlayer::LoadedMedia || st == QMediaPlayer::BufferedMedia) {
                ok = true;
                loop.quit();
            } else if (st == QMediaPlayer::InvalidMedia) {
                ok = false;
                loop.quit();
            }
        });
    guard.start(12000);
    probe.setMedia(QUrl::fromLocalFile(localPath));
    loop.exec();
    guard.stop();
    return ok;
}

static QVector<QPair<qint64, QString>> parseLrcContent(const QString &text)
{
    QVector<QPair<qint64, QString>> entries;
    const QStringList lines = text.split(QLatin1Char('\n'));
    static const QRegularExpression timeTag(
        QStringLiteral(R"(\[(\d{1,2}):(\d{2})(?:\.(\d{2,3}))?\])"));

    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty())
            continue;

        QVector<qint64> times;
        int pos = 0;
        for (QRegularExpressionMatchIterator it = timeTag.globalMatch(line); it.hasNext();) {
            const QRegularExpressionMatch m = it.next();
            const int mm = m.captured(1).toInt();
            const int ss = m.captured(2).toInt();
            const QString frac = m.captured(3);
            qint64 fracMs = 0;
            if (!frac.isEmpty()) {
                if (frac.size() == 2)
                    fracMs = frac.toInt() * 10;
                else if (frac.size() == 3)
                    fracMs = frac.toInt();
                else
                    fracMs = frac.toInt() * 100;
            }
            const qint64 tms = qint64(mm) * 60 * 1000 + qint64(ss) * 1000 + fracMs;
            times.append(tms);
            pos = m.capturedEnd();
        }

        if (times.isEmpty())
            continue;

        const QString lyric = line.mid(pos).trimmed();
        if (lyric.isEmpty())
            continue;
        for (qint64 t : times)
            entries.append(qMakePair(t, lyric));
    }

    std::sort(entries.begin(), entries.end(), [](const QPair<qint64, QString> &a,
                                                 const QPair<qint64, QString> &b) {
        return a.first < b.first;
    });
    return entries;
}

} // namespace

MusicController::MusicController(QObject *parent)
    : QObject(parent)
    , m_player(new QMediaPlayer(this))
    , m_playlist(new QMediaPlaylist(this))
    , m_queueModel(new TrackListModel(this))
    , m_libraryModel(new TrackListModel(this))
    , m_duration(0)
    , m_currentTitle(tr("未选择歌曲"))
    , m_currentArtist(tr("暂无艺术家"))
    , m_musicRoot(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + QStringLiteral("/Music"))
    , m_playbackMode(QMediaPlaylist::Sequential)
    , m_playlistBrowseModel(new PlaylistItemsModel(this))
{
    m_player->setPlaylist(m_playlist);
    m_playlist->setPlaybackMode(m_playbackMode);

    m_trackFilter = new TrackFilterModel(this);
    m_trackFilter->setSourceModel(m_libraryModel);
    connect(m_trackFilter, &QAbstractItemModel::modelReset,
            this, &MusicController::onProxyLayoutChanged);
    connect(m_trackFilter, &QAbstractItemModel::rowsInserted,
            this, &MusicController::onProxyLayoutChanged);
    connect(m_trackFilter, &QAbstractItemModel::rowsRemoved,
            this, &MusicController::onProxyLayoutChanged);
    connect(m_trackFilter, &QAbstractItemModel::layoutChanged,
            this, &MusicController::onProxyLayoutChanged);

    m_queueFilter = new TrackFilterModel(this);
    m_queueFilter->setSourceModel(m_queueModel);
    connect(m_queueFilter, &QAbstractItemModel::modelReset,
            this, &MusicController::onQueueProxyLayoutChanged);
    connect(m_queueFilter, &QAbstractItemModel::rowsInserted,
            this, &MusicController::onQueueProxyLayoutChanged);
    connect(m_queueFilter, &QAbstractItemModel::rowsRemoved,
            this, &MusicController::onQueueProxyLayoutChanged);
    connect(m_queueFilter, &QAbstractItemModel::layoutChanged,
            this, &MusicController::onQueueProxyLayoutChanged);

    m_player->setVolume(70);

    connect(m_player, &QMediaPlayer::stateChanged,
            this, &MusicController::onPlayerStateChanged);
    connect(m_player, &QMediaPlayer::positionChanged,
            this, &MusicController::positionChanged);
    connect(m_player, &QMediaPlayer::durationChanged,
            this, &MusicController::onDurationChanged);
    // 无参 metaDataChanged；QueuedConnection 避免与播放器内部回调重入；元数据读取延后防抖，减轻主线程阻塞（含 WNet 枚举）
    connect(m_player,
            static_cast<void (QMediaObject::*)()>(&QMediaObject::metaDataChanged),
            this,
            &MusicController::onMetaDataChanged,
            Qt::QueuedConnection);
    connect(m_playlist, &QMediaPlaylist::currentIndexChanged,
            this, &MusicController::onPlaylistIndexChanged);

    m_lyricTick = new QTimer(this);
    m_lyricTick->setSingleShot(true);
    m_lyricTick->setInterval(90);
    connect(m_lyricTick, &QTimer::timeout, this, [this]() {
        updateLyricForPosition(m_player->position());
    });
    connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64) {
        if (!m_lrcLines.isEmpty())
            m_lyricTick->start();
    });

    connect(m_queueModel, &QAbstractItemModel::rowsInserted,
            this, &MusicController::trackCountChanged);
    connect(m_queueModel, &QAbstractItemModel::rowsRemoved,
            this, &MusicController::trackCountChanged);
    connect(m_queueModel, &QAbstractItemModel::modelReset,
            this, &MusicController::trackCountChanged);

    connect(m_libraryModel, &QAbstractItemModel::rowsInserted,
            this, &MusicController::libraryTrackCountChanged);
    connect(m_libraryModel, &QAbstractItemModel::rowsRemoved,
            this, &MusicController::libraryTrackCountChanged);
    connect(m_libraryModel, &QAbstractItemModel::modelReset,
            this, &MusicController::libraryTrackCountChanged);
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &MusicController::onPlayerMediaStatusChanged);
    connect(m_player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error), this,
            [this](QMediaPlayer::Error e) {
                if (e == QMediaPlayer::NoError)
                    return;
                if (e == QMediaPlayer::ResourceError || e == QMediaPlayer::FormatError
                    || e == QMediaPlayer::ServiceMissingError)
                    onPlaybackLoadFailed();
            });

    syncQueueSearchFilterFromLibrary();
}

void MusicController::setUserSession(UserSession *session)
{
    if (m_userSession == session)
        return;
    if (m_userSession)
        disconnect(m_userSession, nullptr, this, nullptr);
    m_userSession = session;
    if (m_userSession) {
        connect(m_userSession, &UserSession::loginStateChanged,
                this, &MusicController::onUserLoginStateChanged);
        connect(m_userSession, &UserSession::favoritesChanged,
                this, &MusicController::refreshCurrentFavoriteFlag);
    }
    refreshCurrentFavoriteFlag();
}

QString MusicController::librarySearchFilter() const
{
    return m_trackFilter ? m_trackFilter->filterText() : QString();
}

void MusicController::setLibrarySearchFilter(const QString &text)
{
    if (!m_trackFilter)
        return;
    if (m_trackFilter->filterText() == text)
        return;
    m_trackFilter->setFilterText(text);
    syncQueueSearchFilterFromLibrary();
    emit librarySearchFilterChanged();
    emit filteredTrackCountChanged();
    emit currentDisplayRowChanged();
}

void MusicController::applySearchQuery(const QString &query)
{
    if (!m_trackFilter)
        return;
    const QString t = query.trimmed();
    m_trackFilter->setTitleArtistOnly(true);
    if (m_trackFilter->filterText() != t)
        m_trackFilter->setFilterText(t);
    else
        m_trackFilter->invalidateFiltering();
    syncQueueSearchFilterFromLibrary();
    emit librarySearchFilterChanged();
    emit filteredTrackCountChanged();
    emit currentDisplayRowChanged();
}

void MusicController::clearSearchQuery()
{
    if (!m_trackFilter)
        return;
    m_trackFilter->setTitleArtistOnly(false);
    if (!m_trackFilter->filterText().isEmpty())
        m_trackFilter->setFilterText(QString());
    else
        m_trackFilter->invalidateFiltering();
    syncQueueSearchFilterFromLibrary();
    emit librarySearchFilterChanged();
    emit filteredTrackCountChanged();
    emit currentDisplayRowChanged();
}

int MusicController::filteredTrackCount() const
{
    return m_trackFilter ? m_trackFilter->rowCount() : 0;
}

QString MusicController::playbackModeLabel() const
{
    if (!m_playlist)
        return tr("顺序播放");
    switch (m_playlist->playbackMode()) {
    case QMediaPlaylist::Sequential:
        return tr("顺序播放");
    case QMediaPlaylist::Loop:
        return tr("列表循环");
    case QMediaPlaylist::CurrentItemInLoop:
        return tr("单曲循环");
    case QMediaPlaylist::Random:
        return tr("随机播放");
    default:
        return tr("顺序播放");
    }
}

int MusicController::currentDisplayRow() const
{
    if (!m_trackFilter || !m_libraryModel || !m_queueModel)
        return -1;
    const int plIdx = m_playlist->currentIndex();
    if (plIdx < 0 || plIdx >= m_queueModel->rowCount())
        return -1;
    const QString key = UserSession::storagePathKey(m_queueModel->urlAt(plIdx).toLocalFile());
    if (key.isEmpty())
        return -1;
    const int n = m_libraryModel->rowCount();
    for (int r = 0; r < n; ++r) {
        if (UserSession::storagePathKey(m_libraryModel->urlAt(r).toLocalFile()) != key)
            continue;
        const QModelIndex src = m_libraryModel->index(r, 0);
        const QModelIndex px = m_trackFilter->mapFromSource(src);
        return px.isValid() ? px.row() : -1;
    }
    return -1;
}

int MusicController::currentQueueDisplayRow() const
{
    if (!m_queueFilter || !m_queueModel)
        return -1;
    const int plIdx = m_playlist->currentIndex();
    if (plIdx < 0 || plIdx >= m_queueModel->rowCount())
        return -1;
    const QModelIndex src = m_queueModel->index(plIdx, 0);
    if (!src.isValid())
        return -1;
    const QModelIndex px = m_queueFilter->mapFromSource(src);
    return px.isValid() ? px.row() : -1;
}

void MusicController::cyclePlaybackMode()
{
    using PM = QMediaPlaylist::PlaybackMode;
    PM pm = m_playlist->playbackMode();
    PM next;
    switch (pm) {
    case PM::Sequential:
        next = PM::Loop;
        break;
    case PM::Loop:
        next = PM::CurrentItemInLoop;
        break;
    case PM::CurrentItemInLoop:
        next = PM::Random;
        break;
    case PM::Random:
    default:
        next = PM::Sequential;
        break;
    }
    m_playbackMode = next;
    m_playlist->setPlaybackMode(next);
    emit playbackModeChanged();
}

void MusicController::playIndexFromFilterRow(int filterRow)
{
    if (!m_trackFilter || !m_libraryModel || !m_queueModel || filterRow < 0
        || filterRow >= m_trackFilter->rowCount())
        return;
    const QModelIndex pidx = m_trackFilter->index(filterRow, 0);
    if (!pidx.isValid())
        return;
    const QModelIndex sidx = m_trackFilter->mapToSource(pidx);
    if (!sidx.isValid())
        return;
    const int libRow = sidx.row();
    if (libRow < 0 || libRow >= m_libraryModel->rowCount())
        return;

    const QString selPath = m_libraryModel->urlAt(libRow).toLocalFile();
    const QString selRel = m_libraryModel->libraryRelPathAt(libRow);

    if (!m_showingFullLibrary)
        restoreFullMusicLibrary();

    int playRow = findRowForStoragePath(selPath);
    if (playRow < 0 && !selRel.isEmpty())
        playRow = findQueueRowForLibraryRel(selRel);
    if (playRow >= 0)
        playIndex(playRow);
    else
        playLocalPath(selPath);
}

void MusicController::playIndexFromQueueFilterRow(int filterRow)
{
    if (!m_queueFilter || filterRow < 0 || filterRow >= m_queueFilter->rowCount())
        return;
    const QModelIndex pidx = m_queueFilter->index(filterRow, 0);
    if (!pidx.isValid())
        return;
    const QModelIndex sidx = m_queueFilter->mapToSource(pidx);
    if (!sidx.isValid())
        return;
    playIndex(sidx.row());
}

void MusicController::removeSongAtLibraryFilterRow(int filterRow)
{
    if (!m_trackFilter || !m_libraryModel || filterRow < 0 || filterRow >= m_trackFilter->rowCount())
        return;
    const QModelIndex pidx = m_trackFilter->index(filterRow, 0);
    if (!pidx.isValid())
        return;
    const QModelIndex sidx = m_trackFilter->mapToSource(pidx);
    if (!sidx.isValid())
        return;
    const int libRow = sidx.row();
    if (libRow < 0 || libRow >= m_libraryModel->rowCount())
        return;

    const QString rel = m_libraryModel->libraryRelPathAt(libRow);
    const QString abs = m_libraryModel->urlAt(libRow).toLocalFile();
    const QString userListKey = (m_userSession && !abs.isEmpty()) ? UserSession::storagePathKey(abs) : QString();

    if (!rel.isEmpty() && m_libraryDb.isOpen()) {
        QSqlQuery dq(m_libraryDb);
        dq.prepare(QStringLiteral("DELETE FROM library_tracks WHERE rel_path = ?"));
        dq.addBindValue(rel);
        if (!dq.exec())
            qWarning() << "removeSongAtLibraryFilterRow DELETE:" << dq.lastError().text();
        QFile::remove(abs);
        if (m_userSession && !userListKey.isEmpty())
            m_userSession->removeFavoritesRecentForStorageKey(userListKey);
        QSqlQuery dpi(m_libraryDb);
        dpi.prepare(QStringLiteral("DELETE FROM playlist_items WHERE rel_path = ?"));
        dpi.addBindValue(rel);
        dpi.exec();
    }

    m_libraryModel->removeRows(libRow, 1);

    const int qRow = findQueueRowForLibraryRel(rel);
    if (qRow >= 0) {
        const int wasPlaying = m_playlist->currentIndex();
        m_playlist->removeMedia(qRow);
        m_queueModel->removeRows(qRow, 1);
        if (m_playlist->mediaCount() == 0) {
            m_player->stop();
            m_duration = 0;
            emit durationChanged();
            emit playingChanged();
        } else if (wasPlaying == qRow) {
            const int next = qMin(qRow, m_playlist->mediaCount() - 1);
            m_playlist->setCurrentIndex(next);
            m_player->play();
        }
        emit trackCountChanged();
        emit currentIndexChanged();
        updateCurrentMetadata();
        refreshCurrentFavoriteFlag();
    }

    emit filteredTrackCountChanged();
    emit queueFilteredTrackCountChanged();
    emit currentDisplayRowChanged();
}

void MusicController::removeSongAtQueueFilterRow(int filterRow)
{
    if (!m_queueFilter || filterRow < 0 || filterRow >= m_queueFilter->rowCount())
        return;
    const QModelIndex pidx = m_queueFilter->index(filterRow, 0);
    if (!pidx.isValid())
        return;
    const QModelIndex sidx = m_queueFilter->mapToSource(pidx);
    if (!sidx.isValid())
        return;
    removeSongAt(sidx.row());
}

void MusicController::onProxyLayoutChanged()
{
    emit filteredTrackCountChanged();
    emit currentDisplayRowChanged();
}

void MusicController::onQueueProxyLayoutChanged()
{
    emit queueFilteredTrackCountChanged();
    emit currentDisplayRowChanged();
}

bool MusicController::currentIsFavorite() const
{
    if (!m_userSession || !m_userSession->isLoggedIn())
        return false;
    const QString p = currentLocalPath();
    if (p.isEmpty())
        return false;
    return m_userSession->isFavoritePath(p);
}

QString MusicController::currentLocalPath() const
{
    const int i = m_playlist->currentIndex();
    if (i < 0 || i >= m_queueModel->rowCount())
        return {};
    return m_queueModel->urlAt(i).toLocalFile();
}

int MusicController::findRowForStoragePath(const QString &anyLocalPath) const
{
    const QString target = UserSession::storagePathKey(anyLocalPath);
    if (target.isEmpty())
        return -1;
    const int n = m_queueModel->rowCount();
    for (int r = 0; r < n; ++r) {
        const QString p = UserSession::storagePathKey(m_queueModel->urlAt(r).toLocalFile());
        if (p == target)
            return r;
    }
    return -1;
}

int MusicController::findLibraryRowForLocalPath(const QString &anyLocalPath) const
{
    if (!m_libraryModel)
        return -1;
    const QString target = UserSession::storagePathKey(anyLocalPath);
    if (target.isEmpty())
        return -1;
    const int n = m_libraryModel->rowCount();
    for (int r = 0; r < n; ++r) {
        if (UserSession::storagePathKey(m_libraryModel->urlAt(r).toLocalFile()) == target)
            return r;
    }
    return -1;
}

void MusicController::playLocalPath(const QString &localPath)
{
    QFileInfo fi(localPath);
    if (!fi.exists() || !fi.isFile())
        return;
    const QString key = UserSession::storagePathKey(fi.absoluteFilePath());
    const QString urlPath = key.isEmpty() ? fi.absoluteFilePath() : key;
    int row = findRowForStoragePath(fi.absoluteFilePath());
    if (row < 0) {
        TrackEntry e;
        e.url = QUrl::fromLocalFile(urlPath);
        const int lr = findLibraryRowForLocalPath(fi.absoluteFilePath());
        if (lr >= 0) {
            e.title = m_libraryModel->titleAt(lr).trimmed();
            if (e.title.isEmpty())
                e.title = fi.completeBaseName();
            e.artist = m_libraryModel->artistAt(lr).trimmed();
            if (e.artist.isEmpty())
                e.artist = unknownArtist();
            e.album = m_libraryModel->albumAt(lr);
            e.genre = m_libraryModel->genreAt(lr);
            e.libraryRelPath = m_libraryModel->libraryRelPathAt(lr);
        } else {
            e.title = fi.completeBaseName();
            e.artist = unknownArtist();
            e.album.clear();
            e.genre.clear();
            e.libraryRelPath.clear();
        }
        m_queueModel->appendTracks(QVector<TrackEntry>{e});
        m_playlist->addMedia(e.url);
        row = m_queueModel->rowCount() - 1;
    }
    markPlayCommandTime();
    if (m_playlist->currentIndex() == row) {
        m_player->play();
        return;
    }
    m_playlist->setCurrentIndex(row);
    m_player->play();
}

void MusicController::toggleFavoriteCurrent()
{
    if (!m_userSession || !m_userSession->isLoggedIn())
        return;
    const QString p = currentLocalPath();
    if (p.isEmpty())
        return;
    m_userSession->toggleFavorite(p, m_currentTitle, m_currentArtist);
}

void MusicController::refreshCurrentFavoriteFlag()
{
    emit currentIsFavoriteChanged();
}

void MusicController::onUserLoginStateChanged()
{
    if (m_userSession && m_userSession->isLoggedIn())
        submitCurrentTrackAsRecent();
    refreshCurrentFavoriteFlag();
}

void MusicController::submitCurrentTrackAsRecent()
{
    if (!m_userSession || !m_userSession->isLoggedIn())
        return;
    const int i = m_playlist->currentIndex();
    if (i < 0 || i >= m_queueModel->rowCount())
        return;
    const QString path = m_queueModel->urlAt(i).toLocalFile();
    maybeRecordRecentPlay(path, m_currentTitle, m_currentArtist);
}

void MusicController::maybeRecordRecentPlay(const QString &localPath, const QString &title,
                                            const QString &artist)
{
    if (!m_userSession || !m_userSession->isLoggedIn() || localPath.isEmpty())
        return;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    // 同一曲目在短时间内多次触发 currentIndexChanged / 登录补写，避免重复写入
    if (localPath == m_lastRecentDedupePath && (now - m_lastRecentDedupeMs) < 300)
        return;
    m_lastRecentDedupePath = localPath;
    m_lastRecentDedupeMs = now;
    m_userSession->recordRecentPlay(localPath, title, artist);
}

void MusicController::setLibraryDatabase(const QSqlDatabase &db)
{
    m_libraryDb = db;
}

void MusicController::ensureMusicRoot()
{
    QDir().mkpath(m_musicRoot);
}

QStringList MusicController::audioFilenameFilters()
{
    return {QStringLiteral("*.mp3"), QStringLiteral("*.wav"), QStringLiteral("*.flac"),
            QStringLiteral("*.m4a"),  QStringLiteral("*.ogg"), QStringLiteral("*.wma"),
            QStringLiteral("*.aac"),  QStringLiteral("*.opus")};
}

void MusicController::purgeMissingLibraryRows()
{
    if (!m_libraryDb.isOpen())
        return;
    QSqlQuery qAll(m_libraryDb);
    if (!qAll.exec(QStringLiteral("SELECT rel_path FROM library_tracks"))) {
        qWarning() << "purgeMissingLibraryRows:" << qAll.lastError().text();
        return;
    }
    QStringList missing;
    while (qAll.next()) {
        const QString rel = qAll.value(0).toString();
        const QString abs = QDir(m_musicRoot).filePath(rel);
        if (!QFileInfo::exists(abs))
            missing.append(rel);
    }
    for (const QString &rel : missing) {
        QSqlQuery dq(m_libraryDb);
        dq.prepare(QStringLiteral("DELETE FROM library_tracks WHERE rel_path = ?"));
        dq.addBindValue(rel);
        dq.exec();
        QSqlQuery dpi(m_libraryDb);
        dpi.prepare(QStringLiteral("DELETE FROM playlist_items WHERE rel_path = ?"));
        dpi.addBindValue(rel);
        dpi.exec();
    }
    if (m_userSession)
        m_userSession->pruneStaleFavoritesAndRecent();
}

void MusicController::importLooseFilesIntoLibrary()
{
    if (!m_libraryDb.isOpen())
        return;
    QDir dir(m_musicRoot);
    if (!dir.exists())
        return;
    const QFileInfoList files = dir.entryInfoList(audioFilenameFilters(),
                                                   QDir::Files | QDir::Readable,
                                                   QDir::Name);
    for (const QFileInfo &fi : files) {
        const QString rel = fi.fileName();
        QSqlQuery ck(m_libraryDb);
        ck.prepare(QStringLiteral("SELECT 1 FROM library_tracks WHERE rel_path = ? LIMIT 1"));
        ck.addBindValue(rel);
        if (ck.exec() && ck.next())
            continue;
        QString metaTitle;
        QString metaArtist;
        QString metaAlbum;
        QString metaGenre;
        readEmbeddedAudioMeta(fi.absoluteFilePath(), metaTitle, metaArtist, metaAlbum, metaGenre);
        if (metaTitle.isEmpty())
            metaTitle = fi.completeBaseName();
        if (metaArtist.isEmpty())
            metaArtist = unknownArtist();

        QSqlQuery ins(m_libraryDb);
        ins.prepare(QStringLiteral(
            "INSERT INTO library_tracks (rel_path, title, artist, album, genre, added_at) "
            "VALUES (?,?,?,?,?,?)"));
        ins.addBindValue(rel);
        ins.addBindValue(metaTitle);
        ins.addBindValue(metaArtist);
        ins.addBindValue(metaAlbum);
        ins.addBindValue(metaGenre);
        ins.addBindValue(QDateTime::currentMSecsSinceEpoch());
        if (!ins.exec())
            qWarning() << "importLooseFilesIntoLibrary:" << ins.lastError().text();
    }
}

void MusicController::loadMusicLibraryFromDatabase()
{
    ensureMusicRoot();
    if (!m_libraryDb.isOpen()) {
        qWarning("Music library database is not open.");
        return;
    }

    m_player->stop();
    m_playlist->clear();
    m_queueModel->clear();
    m_libraryModel->clear();

    purgeMissingLibraryRows();
    importLooseFilesIntoLibrary();

    QSqlQuery q(m_libraryDb);
    if (!q.exec(QStringLiteral(
            "SELECT rel_path, title, artist, album, genre FROM library_tracks ORDER BY added_at ASC"))) {
        qWarning() << "loadMusicLibraryFromDatabase:" << q.lastError().text();
        m_showingFullLibrary = true;
        m_viewingPlaylistId = 0;
        m_viewingPlaylistTitle.clear();
        emit libraryViewModeChanged();
        m_playlist->setPlaybackMode(m_playbackMode);
        emit trackCountChanged();
        emit libraryTrackCountChanged();
        emit filteredTrackCountChanged();
        emit queueFilteredTrackCountChanged();
        emit currentIndexChanged();
        updateCurrentMetadata();
        refreshCurrentFavoriteFlag();
        m_lastKnownPlaylistIndex = m_playlist->currentIndex();
        m_indexBeforePlaylistChange = -1;
        return;
    }

    QVector<TrackEntry> vec;
    while (q.next()) {
        const QString rel = q.value(0).toString();
        const QString abs = QDir(m_musicRoot).filePath(rel);
        if (!QFileInfo::exists(abs))
            continue;
        TrackEntry e;
        e.url = QUrl::fromLocalFile(abs);
        e.title = q.value(1).toString();
        e.artist = q.value(2).toString();
        e.album = q.value(3).toString();
        e.genre = q.value(4).toString();
        if (e.title.isEmpty())
            e.title = QFileInfo(abs).completeBaseName();
        if (e.artist.isEmpty())
            e.artist = unknownArtist();
        e.libraryRelPath = rel;
        vec.append(e);
    }

    if (!vec.isEmpty()) {
        m_libraryModel->appendTracks(vec);
        m_queueModel->appendTracks(vec);
        for (const TrackEntry &e : vec)
            m_playlist->addMedia(e.url);
    }

    m_showingFullLibrary = true;
    m_viewingPlaylistId = 0;
    m_viewingPlaylistTitle.clear();
    emit libraryViewModeChanged();

    m_playlist->setPlaybackMode(m_playbackMode);
    emit trackCountChanged();
    emit libraryTrackCountChanged();
    emit filteredTrackCountChanged();
    emit queueFilteredTrackCountChanged();
    emit currentIndexChanged();
    m_duration = 0;
    emit durationChanged();
    updateCurrentMetadata();
    refreshCurrentFavoriteFlag();
    m_lastKnownPlaylistIndex = m_playlist->currentIndex();
    m_indexBeforePlaylistChange = -1;
}

void MusicController::uploadSongs()
{
    ensureMusicRoot();
    if (!m_libraryDb.isOpen())
        return;

    QString filter;
    for (const QString &p : audioFilenameFilters()) {
        if (!filter.isEmpty())
            filter += QLatin1Char(' ');
        filter += p;
    }
    const QStringList paths = QFileDialog::getOpenFileNames(
        nullptr,
        tr("选择要上传的音乐文件"),
        QDir::homePath(),
        tr("音频文件 (%1)").arg(filter));

    for (const QString &path : paths) {
        QFileInfo src(path);
        if (!src.isFile())
            continue;
        QString suf = src.suffix();
        if (suf.isEmpty())
            suf = QStringLiteral("mp3");
        const QString rel = QUuid::createUuid().toString(QUuid::WithoutBraces) + QLatin1Char('.') + suf;
        const QString dst = QDir(m_musicRoot).filePath(rel);
        if (!QFile::copy(path, dst)) {
            qWarning() << "upload copy failed:" << path << "->" << dst;
            continue;
        }

        if (!localFileLoadsWithQtMultimedia(dst)) {
            QFile::remove(dst);
            QMessageBox::warning(nullptr,
                                 tr("无法导入"),
                                 tr("本机 Qt 多媒体无法加载该文件，未加入曲库（常见原因：格式不受系统解码器支持、文件损坏或非音频）。"
                                    "可尝试转为 MP3，或安装解码组件后重试。\n\n%1")
                                     .arg(src.fileName()));
            continue;
        }

        // 与音频同目录、同主文件名的 .lrc 一并复制到曲库（曲库内音频为 UUID 名，歌词需同名 .lrc 才能被加载）
        QString lrcDst;
        const QString lrcSrc = src.dir().filePath(src.completeBaseName() + QStringLiteral(".lrc"));
        if (QFileInfo::exists(lrcSrc)) {
            const QFileInfo dstFi(dst);
            lrcDst = dstFi.dir().filePath(dstFi.completeBaseName() + QStringLiteral(".lrc"));
            if (!QFile::copy(lrcSrc, lrcDst))
                qWarning() << "upload lrc copy failed:" << lrcSrc << "->" << lrcDst;
        }

        QString metaTitle;
        QString metaArtist;
        QString metaAlbum;
        QString metaGenre;
        readEmbeddedAudioMeta(dst, metaTitle, metaArtist, metaAlbum, metaGenre);
        if (metaTitle.isEmpty())
            metaTitle = src.completeBaseName();
        if (metaArtist.isEmpty())
            metaArtist = unknownArtist();

        QSqlQuery ins(m_libraryDb);
        ins.prepare(QStringLiteral(
            "INSERT INTO library_tracks (rel_path, title, artist, album, genre, added_at) "
            "VALUES (?,?,?,?,?,?)"));
        ins.addBindValue(rel);
        ins.addBindValue(metaTitle);
        ins.addBindValue(metaArtist);
        ins.addBindValue(metaAlbum);
        ins.addBindValue(metaGenre);
        ins.addBindValue(QDateTime::currentMSecsSinceEpoch());
        if (!ins.exec()) {
            qWarning() << "upload INSERT:" << ins.lastError().text();
            QFile::remove(dst);
            if (!lrcDst.isEmpty())
                QFile::remove(lrcDst);
            continue;
        }

        TrackEntry e;
        e.url = QUrl::fromLocalFile(dst);
        e.title = metaTitle;
        e.artist = metaArtist;
        e.album = metaAlbum;
        e.genre = metaGenre;
        e.libraryRelPath = rel;
        m_libraryModel->appendTracks(QVector<TrackEntry>{e});
        if (m_showingFullLibrary) {
            m_queueModel->appendTracks(QVector<TrackEntry>{e});
            m_playlist->addMedia(e.url);
        }
    }
}

void MusicController::removeSongAt(int row)
{
    if (row < 0 || row >= m_queueModel->rowCount())
        return;

    const QString rel = m_queueModel->libraryRelPathAt(row);
    const QString abs = m_queueModel->urlAt(row).toLocalFile();
    const int wasPlaying = m_playlist->currentIndex();
    const QString userListKey = (m_userSession && !abs.isEmpty()) ? UserSession::storagePathKey(abs) : QString();

    bool removedLibraryFile = false;
    if (!m_showingFullLibrary && m_viewingPlaylistId > 0 && m_libraryDb.isOpen() && !rel.isEmpty()) {
        QSqlQuery dq(m_libraryDb);
        dq.prepare(QStringLiteral("DELETE FROM playlist_items WHERE playlist_id = ? AND rel_path = ?"));
        dq.addBindValue(m_viewingPlaylistId);
        dq.addBindValue(rel);
        if (!dq.exec())
            qWarning() << "removeSongAt playlist_items:" << dq.lastError().text();
    } else if (m_showingFullLibrary && !rel.isEmpty() && m_libraryDb.isOpen()) {
        QSqlQuery dq(m_libraryDb);
        dq.prepare(QStringLiteral("DELETE FROM library_tracks WHERE rel_path = ?"));
        dq.addBindValue(rel);
        if (!dq.exec())
            qWarning() << "removeSongAt DELETE:" << dq.lastError().text();
        QFile::remove(abs);
        if (m_userSession && !userListKey.isEmpty())
            m_userSession->removeFavoritesRecentForStorageKey(userListKey);
        removedLibraryFile = true;
    }

    m_playlist->removeMedia(row);
    m_queueModel->removeRows(row, 1);

    if (removedLibraryFile)
        removeFirstLibraryRowMatchingRel(rel);

    if (m_playlist->mediaCount() == 0) {
        m_player->stop();
        m_duration = 0;
        emit durationChanged();
        emit playingChanged();
    } else if (wasPlaying == row) {
        const int next = qMin(row, m_playlist->mediaCount() - 1);
        m_playlist->setCurrentIndex(next);
        m_player->play();
    }

    emit trackCountChanged();
    emit queueFilteredTrackCountChanged();
    emit currentIndexChanged();
    updateCurrentMetadata();
    refreshCurrentFavoriteFlag();
}

bool MusicController::playing() const
{
    return m_player->state() == QMediaPlayer::PlayingState;
}

qreal MusicController::volume() const
{
    return m_player->volume() / 100.0;
}

void MusicController::setVolume(qreal v)
{
    v = qBound(0.0, v, 1.0);
    const int vi = qRound(v * 100.0);
    if (m_player->volume() == vi)
        return;
    m_player->setVolume(vi);
    emit volumeChanged();
}

bool MusicController::hasCurrentTrack() const
{
    const int idx = m_playlist->currentIndex();
    return idx >= 0 && idx < m_playlist->mediaCount();
}

void MusicController::playIndex(int index)
{
    if (index < 0 || index >= m_playlist->mediaCount())
        return;
    markPlayCommandTime();
    if (m_playlist->currentIndex() == index) {
        m_player->play();
        return;
    }
    m_playlist->setCurrentIndex(index);
    m_player->play();
}

void MusicController::playPause()
{
    if (m_playlist->mediaCount() == 0)
        return;
    if (m_playlist->currentIndex() < 0)
        m_playlist->setCurrentIndex(0);
    if (playing())
        m_player->pause();
    else {
        markPlayCommandTime();
        m_player->play();
    }
}

void MusicController::playNext()
{
    if (m_playlist->mediaCount() == 0)
        return;
    if (m_playlist->currentIndex() < 0)
        m_playlist->setCurrentIndex(0);
    else
        m_playlist->next();
    markPlayCommandTime();
    m_player->play();
}

void MusicController::playPrevious()
{
    if (m_playlist->mediaCount() == 0)
        return;
    if (m_playlist->currentIndex() < 0)
        m_playlist->setCurrentIndex(0);
    else
        m_playlist->previous();
    markPlayCommandTime();
    m_player->play();
}

void MusicController::seek(qreal ratio)
{
    if (m_duration <= 0)
        return;
    ratio = qBound(0.0, ratio, 1.0);
    m_player->setPosition(qint64(ratio * qreal(m_duration)));
}

QString MusicController::formatMs(qint64 ms) const
{
    if (ms < 0)
        ms = 0;
    const qint64 totalSec = ms / 1000;
    const int m = int(totalSec / 60);
    const int s = int(totalSec % 60);
    return QStringLiteral("%1:%2")
        .arg(m)
        .arg(s, 2, 10, QChar('0'));
}

void MusicController::onPlayerStateChanged(QMediaPlayer::State)
{
    emit playingChanged();
}

void MusicController::onDurationChanged(qint64 duration)
{
    m_duration = duration;
    emit durationChanged();
}

void MusicController::markPlayCommandTime()
{
    m_lastPlayCommandMs = QDateTime::currentMSecsSinceEpoch();
}

void MusicController::clearPlaybackError()
{
    if (m_playbackError.isEmpty())
        return;
    m_playbackError.clear();
    emit playbackErrorChanged();
}

void MusicController::onPlayerMediaStatusChanged(QMediaPlayer::MediaStatus st)
{
    if (st == QMediaPlayer::LoadedMedia || st == QMediaPlayer::BufferedMedia) {
        if (!m_playbackError.isEmpty()) {
            m_playbackError.clear();
            emit playbackErrorChanged();
        }
        return;
    }
    if (st == QMediaPlayer::InvalidMedia)
        onPlaybackLoadFailed();
}

void MusicController::onPlaybackLoadFailed()
{
    if (m_inPlaybackFailureRecovery)
        return;
    m_inPlaybackFailureRecovery = true;

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const bool recentUserPlay = (nowMs - m_lastPlayCommandMs) < 1600;
    const int prevIdx = m_indexBeforePlaylistChange;
    const int curIdx = m_playlist->currentIndex();

    QTimer::singleShot(0, this, [this, recentUserPlay, prevIdx, curIdx]() {
        if (recentUserPlay && prevIdx >= 0 && curIdx != prevIdx
            && m_playlist->currentIndex() == curIdx) {
            const QSignalBlocker blocker(m_playlist);
            m_playlist->setCurrentIndex(prevIdx);
        }

        m_player->stop();

        QString err = m_player->errorString().trimmed();
        if (err.isEmpty()) {
            err = tr("当前文件无法解码播放。常见原因：格式不受本机解码器支持、文件已损坏或非音频文件。"
                     "可尝试转为 MP3，或安装系统音视频解码组件（如 Windows 上的 K-Lite 等）。");
        }
        if (m_playbackError != err) {
            m_playbackError = err;
            emit playbackErrorChanged();
        }

        const int row = m_playlist->currentIndex();
        if (row >= 0 && row < m_queueModel->rowCount()) {
            m_currentTitle = m_queueModel->titleAt(row);
            m_currentArtist = m_queueModel->artistAt(row);
        } else {
            m_currentTitle = tr("无法播放");
            m_currentArtist = tr("暂无艺术家");
        }
        emit currentTrackChanged();

        m_inPlaybackFailureRecovery = false;
    });
}

void MusicController::onPlaylistIndexChanged()
{
    const int now = m_playlist->currentIndex();
    m_indexBeforePlaylistChange = m_lastKnownPlaylistIndex;
    m_lastKnownPlaylistIndex = now;

    updateCurrentMetadata();
    emit currentIndexChanged();
    emit currentDisplayRowChanged();
    if (m_userSession && m_userSession->isLoggedIn()) {
        const int i = m_playlist->currentIndex();
        if (i >= 0 && i < m_queueModel->rowCount()) {
            const QString path = m_queueModel->urlAt(i).toLocalFile();
            maybeRecordRecentPlay(path, m_currentTitle, m_currentArtist);
        }
    }
    refreshCurrentFavoriteFlag();
}

void MusicController::onMetaDataChanged()
{
    scheduleApplyMetaFromPlayer();
}

void MusicController::scheduleApplyMetaFromPlayer()
{
    if (!m_metaDebounce) {
        m_metaDebounce = new QTimer(this);
        m_metaDebounce->setSingleShot(true);
        connect(m_metaDebounce, &QTimer::timeout, this, [this]() {
            applyMetaDataFromPlayer();
        });
    }
    m_metaDebounce->stop();
    m_metaDebounce->start(120);
}

void MusicController::updateCurrentMetadata()
{
    const int idx = m_playlist->currentIndex();
    if (idx < 0 || idx >= m_queueModel->rowCount()) {
        m_currentTitle = tr("未选择歌曲");
        m_currentArtist = tr("暂无艺术家");
        emit currentTrackChanged();
        reloadLrcForCurrentTrack();
        return;
    }
    m_currentTitle = m_queueModel->titleAt(idx);
    m_currentArtist = m_queueModel->artistAt(idx);
    emit currentTrackChanged();
    scheduleApplyMetaFromPlayer();
    reloadLrcForCurrentTrack();
}

void MusicController::applyMetaDataFromPlayer()
{
    if (m_playlist->currentIndex() < 0)
        return;

    // 与 QMediaMetaData 中约定的键名一致；多次调用可能触发底层阻塞，故仅在此防抖回调中执行
    const QString t = m_player->metaData(QStringLiteral("Title")).toString().trimmed();
    if (!t.isEmpty())
        m_currentTitle = t;

    QString a = m_player->metaData(QStringLiteral("AlbumArtist")).toString().trimmed();
    if (a.isEmpty())
        a = m_player->metaData(QStringLiteral("ContributingArtist")).toString().trimmed();
    if (a.isEmpty())
        a = m_player->metaData(QStringLiteral("Author")).toString().trimmed();
    if (!a.isEmpty())
        m_currentArtist = a;

    emit currentTrackChanged();
}

QString MusicController::unknownArtist()
{
    return tr("未知艺术家");
}

void MusicController::reloadLrcForCurrentTrack()
{
    m_lrcLines.clear();
    m_lyricPrev.clear();
    m_lyricCur.clear();
    m_lyricNext.clear();
    m_lyricHighlightIndex = -1;

    const QString audio = currentLocalPath();
    if (audio.isEmpty()) {
        emit lyricsChanged();
        return;
    }

    const QFileInfo fi(audio);
    const QString lrcPath = fi.dir().filePath(fi.completeBaseName() + QStringLiteral(".lrc"));
    QFile f(lrcPath);
    if (!f.open(QIODevice::ReadOnly)) {
        emit lyricsChanged();
        return;
    }

    const QByteArray raw = f.readAll();
    QString text = QString::fromUtf8(raw);
    if (text.contains(QChar::ReplacementCharacter))
        text = QString::fromLocal8Bit(raw);

    m_lrcLines = parseLrcContent(text);
    if (m_lrcLines.isEmpty()) {
        emit lyricsChanged();
        return;
    }
    updateLyricForPosition(m_player->position());
}

void MusicController::updateLyricForPosition(qint64 posMs)
{
    if (m_lrcLines.isEmpty())
        return;

    int idx = -1;
    for (int i = m_lrcLines.size() - 1; i >= 0; --i) {
        if (m_lrcLines.at(i).first <= posMs) {
            idx = i;
            break;
        }
    }

    QString prev;
    QString cur;
    QString next;
    if (idx < 0) {
        next = m_lrcLines.first().second;
    } else {
        cur = m_lrcLines.at(idx).second;
        if (idx > 0)
            prev = m_lrcLines.at(idx - 1).second;
        if (idx + 1 < m_lrcLines.size())
            next = m_lrcLines.at(idx + 1).second;
    }

    const int newHi = idx;
    if (prev == m_lyricPrev && cur == m_lyricCur && next == m_lyricNext && newHi == m_lyricHighlightIndex)
        return;
    m_lyricPrev = prev;
    m_lyricCur = cur;
    m_lyricNext = next;
    m_lyricHighlightIndex = newHi;
    emit lyricsChanged();
}

QStringList MusicController::lyricLines() const
{
    QStringList ls;
    ls.reserve(m_lrcLines.size());
    for (const auto &p : m_lrcLines)
        ls.append(p.second);
    return ls;
}

void MusicController::setCategoryBrowse(const QString &artist, const QString &album, const QString &genre)
{
    if (!m_trackFilter)
        return;
    m_trackFilter->setCategoryFilter(artist, album, genre);
    emit filteredTrackCountChanged();
    emit currentDisplayRowChanged();
}

void MusicController::clearCategoryBrowse()
{
    setCategoryBrowse(QString(), QString(), QString());
}

QStringList MusicController::distinctLibraryArtists() const
{
    QSet<QString> set;
    const int n = m_libraryModel->rowCount();
    for (int i = 0; i < n; ++i) {
        const QString a = m_libraryModel->artistAt(i).trimmed();
        if (!a.isEmpty())
            set.insert(a);
    }
    QStringList out;
    for (const QString &s : set)
        out.append(s);
    std::sort(out.begin(), out.end(), [](const QString &a, const QString &b) {
        return QString::localeAwareCompare(a, b) < 0;
    });
    return out;
}

QStringList MusicController::distinctLibraryAlbums() const
{
    QSet<QString> set;
    const int n = m_libraryModel->rowCount();
    for (int i = 0; i < n; ++i) {
        const QString a = m_libraryModel->albumAt(i).trimmed();
        if (!a.isEmpty())
            set.insert(a);
    }
    QStringList out;
    for (const QString &s : set)
        out.append(s);
    std::sort(out.begin(), out.end(), [](const QString &a, const QString &b) {
        return QString::localeAwareCompare(a, b) < 0;
    });
    return out;
}

QStringList MusicController::distinctLibraryGenres() const
{
    QSet<QString> set;
    const int n = m_libraryModel->rowCount();
    for (int i = 0; i < n; ++i) {
        const QString a = m_libraryModel->genreAt(i).trimmed();
        if (!a.isEmpty())
            set.insert(a);
    }
    QStringList out;
    for (const QString &s : set)
        out.append(s);
    std::sort(out.begin(), out.end(), [](const QString &a, const QString &b) {
        return QString::localeAwareCompare(a, b) < 0;
    });
    return out;
}

bool MusicController::openUserPlaylistPlayback(int playlistId)
{
    if (playlistId <= 0 || !m_libraryDb.isOpen() || !m_userSession || !m_userSession->isLoggedIn())
        return false;

    clearSearchQuery();
    clearCategoryBrowse();

    QSqlQuery qname(m_libraryDb);
    qname.prepare(QStringLiteral(
        "SELECT name FROM user_playlists WHERE id = ? AND user_id = ?"));
    qname.addBindValue(playlistId);
    qname.addBindValue(m_userSession->currentUserId());
    if (!qname.exec() || !qname.next())
        return false;
    const QString pname = qname.value(0).toString();

    QSqlQuery q(m_libraryDb);
    q.prepare(QStringLiteral(
        "SELECT rel_path, title, artist FROM playlist_items WHERE playlist_id = ? "
        "ORDER BY sort_order ASC, id ASC"));
    q.addBindValue(playlistId);
    if (!q.exec()) {
        qWarning() << "openUserPlaylistPlayback:" << q.lastError().text();
        return false;
    }

    QVector<TrackEntry> vec;
    while (q.next()) {
        const QString rel = q.value(0).toString();
        const QString abs = QDir(m_musicRoot).filePath(rel);
        if (!QFileInfo::exists(abs))
            continue;
        TrackEntry e;
        e.url = QUrl::fromLocalFile(abs);
        e.title = q.value(1).toString();
        e.artist = q.value(2).toString();
        if (e.title.isEmpty())
            e.title = QFileInfo(abs).completeBaseName();
        if (e.artist.isEmpty())
            e.artist = unknownArtist();
        e.album.clear();
        e.genre.clear();
        e.libraryRelPath = rel;
        vec.append(e);
    }

    if (vec.isEmpty())
        return false;

    m_player->stop();
    m_playlist->clear();
    m_queueModel->clear();

    m_queueModel->appendTracks(vec);
    for (const TrackEntry &e : vec)
        m_playlist->addMedia(e.url);

    m_showingFullLibrary = false;
    m_viewingPlaylistId = playlistId;
    m_viewingPlaylistTitle = pname;
    m_playlist->setPlaybackMode(m_playbackMode);
    emit libraryViewModeChanged();
    emit trackCountChanged();
    emit filteredTrackCountChanged();
    emit queueFilteredTrackCountChanged();
    emit currentIndexChanged();
    m_duration = 0;
    emit durationChanged();
    updateCurrentMetadata();
    refreshCurrentFavoriteFlag();
    m_lastKnownPlaylistIndex = m_playlist->currentIndex();
    m_indexBeforePlaylistChange = -1;
    return true;
}

bool MusicController::openFavoritesPlayback()
{
    if (!m_libraryDb.isOpen() || !m_userSession || !m_userSession->isLoggedIn())
        return false;

    clearSearchQuery();
    clearCategoryBrowse();

    QSqlQuery q(m_libraryDb);
    q.prepare(QStringLiteral(
        "SELECT path, title, artist FROM favorites WHERE user_id = ? ORDER BY added_at DESC"));
    q.addBindValue(m_userSession->currentUserId());
    if (!q.exec()) {
        qWarning() << "openFavoritesPlayback:" << q.lastError().text();
        return false;
    }

    const QDir root(m_musicRoot);
    QVector<TrackEntry> vec;
    while (q.next()) {
        const QString pathKey = q.value(0).toString().trimmed();
        if (pathKey.isEmpty() || !QFileInfo::exists(pathKey))
            continue;

        TrackEntry e;
        e.url = QUrl::fromLocalFile(pathKey);
        e.title = q.value(1).toString().trimmed();
        e.artist = q.value(2).toString().trimmed();
        if (e.title.isEmpty())
            e.title = QFileInfo(pathKey).completeBaseName();
        if (e.artist.isEmpty())
            e.artist = unknownArtist();
        e.album.clear();
        e.genre.clear();
        const QString rel = root.relativeFilePath(pathKey);
        if (!rel.startsWith(QLatin1String("..")) && !QDir::isAbsolutePath(rel))
            e.libraryRelPath = rel;
        else
            e.libraryRelPath.clear();
        vec.append(e);
    }

    if (vec.isEmpty())
        return false;

    m_player->stop();
    m_playlist->clear();
    m_queueModel->clear();

    m_queueModel->appendTracks(vec);
    for (const TrackEntry &e : vec)
        m_playlist->addMedia(e.url);

    m_showingFullLibrary = false;
    m_viewingPlaylistId = -1;
    m_viewingPlaylistTitle = tr("我的喜欢");
    m_playlist->setPlaybackMode(m_playbackMode);
    emit libraryViewModeChanged();
    emit trackCountChanged();
    emit filteredTrackCountChanged();
    emit queueFilteredTrackCountChanged();
    emit currentIndexChanged();
    m_duration = 0;
    emit durationChanged();
    updateCurrentMetadata();
    refreshCurrentFavoriteFlag();
    m_lastKnownPlaylistIndex = m_playlist->currentIndex();
    m_indexBeforePlaylistChange = -1;
    return true;
}

void MusicController::restoreFullMusicLibrary()
{
    loadMusicLibraryFromDatabase();
}

bool MusicController::loadPlaylistBrowse(int playlistId)
{
    m_playlistBrowsePlaylistId = 0;
    if (!m_playlistBrowseModel || playlistId <= 0 || !m_libraryDb.isOpen() || !m_userSession
        || !m_userSession->isLoggedIn())
        return false;
    if (!m_playlistBrowseModel->loadFromPlaylist(m_libraryDb, m_musicRoot, m_userSession->currentUserId(),
                                                 playlistId))
        return false;
    m_playlistBrowsePlaylistId = playlistId;
    return true;
}

void MusicController::clearPlaylistBrowse()
{
    m_playlistBrowsePlaylistId = 0;
    if (m_playlistBrowseModel)
        m_playlistBrowseModel->clear();
}

bool MusicController::removeTrackFromBrowsePlaylist(const QString &relPath)
{
    if (m_playlistBrowsePlaylistId <= 0 || relPath.isEmpty() || !m_userSession
        || !m_userSession->isLoggedIn())
        return false;
    if (!m_userSession->removeTrackFromNamedPlaylist(m_playlistBrowsePlaylistId, relPath))
        return false;
    return loadPlaylistBrowse(m_playlistBrowsePlaylistId);
}

void MusicController::playFirstInPlaylistBrowse()
{
    if (!m_playlistBrowseModel || m_playlistBrowseModel->rowCount() <= 0)
        return;
    const int n = m_playlistBrowseModel->rowCount();
    for (int r = 0; r < n; ++r) {
        const QModelIndex ix = m_playlistBrowseModel->index(r, 0);
        const QString p = m_playlistBrowseModel->data(ix, PlaylistItemsModel::PathRole).toString();
        if (!p.isEmpty()) {
            playLocalPath(p);
            return;
        }
    }
}

void MusicController::syncQueueSearchFilterFromLibrary()
{
    if (!m_trackFilter || !m_queueFilter)
        return;
    m_queueFilter->setTitleArtistOnly(m_trackFilter->titleArtistOnly());
    m_queueFilter->setCategoryFilter(QString(), QString(), QString());
    const QString t = m_trackFilter->filterText();
    if (m_queueFilter->filterText() != t)
        m_queueFilter->setFilterText(t);
    else
        m_queueFilter->invalidateFiltering();
}

void MusicController::removeFirstLibraryRowMatchingRel(const QString &rel)
{
    if (!m_libraryModel || rel.isEmpty())
        return;
    const int n = m_libraryModel->rowCount();
    for (int r = 0; r < n; ++r) {
        if (m_libraryModel->libraryRelPathAt(r) == rel) {
            m_libraryModel->removeRows(r, 1);
            break;
        }
    }
}

int MusicController::findQueueRowForLibraryRel(const QString &rel) const
{
    if (!m_queueModel || rel.isEmpty())
        return -1;
    for (int r = 0; r < m_queueModel->rowCount(); ++r) {
        if (m_queueModel->libraryRelPathAt(r) == rel)
            return r;
    }
    return -1;
}

MusicController::~MusicController() = default;
