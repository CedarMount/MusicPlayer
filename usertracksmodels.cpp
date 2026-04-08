#include "usertracksmodels.h"
#include "usersession.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>

RecentTracksModel::RecentTracksModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int RecentTracksModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

QVariant RecentTracksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return QVariant();
    const UserTrackRow &r = m_rows.at(index.row());
    switch (role) {
    case PathRole:
        return r.path;
    case TitleRole:
        return r.title;
    case ArtistRole:
        return r.artist;
    case TimeRole:
        return r.sortTime;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> RecentTracksModel::roleNames() const
{
    return {
        {PathRole, "path"},
        {TitleRole, "title"},
        {ArtistRole, "artist"},
        {TimeRole, "time"}
    };
}

void RecentTracksModel::reloadFromDb(QSqlDatabase &db, int userId)
{
    beginResetModel();
    m_rows.clear();
    if (userId <= 0 || !db.isOpen()) {
        endResetModel();
        return;
    }
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT path, title, artist, played_at FROM recent_plays WHERE user_id = ? "
        "ORDER BY played_at DESC LIMIT 200"));
    q.addBindValue(userId);
    if (q.exec()) {
        QSet<QString> seenPath;
        while (q.next()) {
            const QString path = q.value(0).toString();
            if (path.isEmpty() || !QFileInfo::exists(path)) {
                QSqlQuery dq(db);
                dq.prepare(QStringLiteral("DELETE FROM recent_plays WHERE user_id = ? AND path = ?"));
                dq.addBindValue(userId);
                dq.addBindValue(path);
                dq.exec();
                continue;
            }
            if (seenPath.contains(path))
                continue;
            seenPath.insert(path);
            UserTrackRow row;
            row.path = path;
            row.title = q.value(1).toString();
            row.artist = q.value(2).toString();
            row.sortTime = q.value(3).toLongLong();
            m_rows.append(row);
        }
    }
    endResetModel();
}

FavoriteTracksModel::FavoriteTracksModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int FavoriteTracksModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

QVariant FavoriteTracksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return QVariant();
    const UserTrackRow &r = m_rows.at(index.row());
    switch (role) {
    case PathRole:
        return r.path;
    case TitleRole:
        return r.title;
    case ArtistRole:
        return r.artist;
    case TimeRole:
        return r.sortTime;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> FavoriteTracksModel::roleNames() const
{
    return {
        {PathRole, "path"},
        {TitleRole, "title"},
        {ArtistRole, "artist"},
        {TimeRole, "time"}
    };
}

void FavoriteTracksModel::reloadFromDb(QSqlDatabase &db, int userId)
{
    beginResetModel();
    m_rows.clear();
    if (userId <= 0 || !db.isOpen()) {
        endResetModel();
        return;
    }
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT path, title, artist, added_at FROM favorites WHERE user_id = ? "
        "ORDER BY added_at DESC"));
    q.addBindValue(userId);
    if (q.exec()) {
        while (q.next()) {
            const QString path = q.value(0).toString();
            if (path.isEmpty() || !QFileInfo::exists(path)) {
                QSqlQuery dq(db);
                dq.prepare(QStringLiteral("DELETE FROM favorites WHERE user_id = ? AND path = ?"));
                dq.addBindValue(userId);
                dq.addBindValue(path);
                dq.exec();
                continue;
            }
            UserTrackRow row;
            row.path = path;
            row.title = q.value(1).toString();
            row.artist = q.value(2).toString();
            row.sortTime = q.value(3).toLongLong();
            m_rows.append(row);
        }
    }
    endResetModel();
}

NamedPlaylistsModel::NamedPlaylistsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int NamedPlaylistsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_rows.size();
}

QVariant NamedPlaylistsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return QVariant();
    const auto &r = m_rows.at(index.row());
    switch (role) {
    case PlaylistIdRole:
        return r.first;
    case NameRole:
        return r.second;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> NamedPlaylistsModel::roleNames() const
{
    return {{PlaylistIdRole, "playlistId"}, {NameRole, "name"}};
}

void NamedPlaylistsModel::reloadFromDb(QSqlDatabase &db, int userId)
{
    beginResetModel();
    m_rows.clear();
    if (userId <= 0 || !db.isOpen()) {
        endResetModel();
        return;
    }
    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT id, name FROM user_playlists WHERE user_id = ? ORDER BY created_at DESC, id DESC"));
    q.addBindValue(userId);
    if (q.exec()) {
        while (q.next())
            m_rows.append(qMakePair(q.value(0).toInt(), q.value(1).toString()));
    }
    endResetModel();
}

PlaylistItemsModel::PlaylistItemsModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PlaylistItemsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

QVariant PlaylistItemsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();
    const Item &r = m_items.at(index.row());
    switch (role) {
    case PathRole:
        return r.pathKey;
    case TitleRole:
        return r.title;
    case ArtistRole:
        return r.artist;
    case SortOrderRole:
        return r.sortOrder;
    case RelPathRole:
        return r.relPath;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> PlaylistItemsModel::roleNames() const
{
    return {
        {PathRole, "path"},
        {TitleRole, "title"},
        {ArtistRole, "artist"},
        {SortOrderRole, "sortOrder"},
        {RelPathRole, "relPath"},
    };
}

void PlaylistItemsModel::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}

bool PlaylistItemsModel::loadFromPlaylist(QSqlDatabase &db, const QString &musicRoot, int userId,
                                            int playlistId)
{
    beginResetModel();
    m_items.clear();
    if (playlistId <= 0 || userId <= 0 || !db.isOpen() || musicRoot.isEmpty()) {
        endResetModel();
        return false;
    }

    QSqlQuery ck(db);
    ck.prepare(QStringLiteral("SELECT 1 FROM user_playlists WHERE id = ? AND user_id = ? LIMIT 1"));
    ck.addBindValue(playlistId);
    ck.addBindValue(userId);
    if (!ck.exec() || !ck.next()) {
        endResetModel();
        return false;
    }

    QSqlQuery q(db);
    q.prepare(QStringLiteral(
        "SELECT rel_path, title, artist FROM playlist_items WHERE playlist_id = ? "
        "ORDER BY sort_order ASC, id ASC"));
    q.addBindValue(playlistId);
    if (!q.exec()) {
        endResetModel();
        return false;
    }

    int ord = 0;
    while (q.next()) {
        const QString rel = q.value(0).toString();
        if (rel.isEmpty())
            continue;
        const QString abs = QDir(musicRoot).filePath(rel);
        Item it;
        it.relPath = rel;
        it.title = q.value(1).toString();
        it.artist = q.value(2).toString();
        it.sortOrder = ord++;

        if (QFileInfo::exists(abs)) {
            it.pathKey = UserSession::storagePathKey(abs);
            if (it.title.isEmpty())
                it.title = QFileInfo(abs).completeBaseName();
            if (it.artist.isEmpty())
                it.artist = QStringLiteral("未知艺术家");
        } else {
            it.pathKey.clear();
            if (it.title.isEmpty())
                it.title = QFileInfo(rel).completeBaseName();
            if (it.artist.isEmpty())
                it.artist = QStringLiteral("文件不在曲库目录");
        }
        m_items.append(it);
    }
    endResetModel();
    return true;
}
