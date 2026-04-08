#include "tracklistmodel.h"

TrackListModel::TrackListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int TrackListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_tracks.size();
}

QVariant TrackListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tracks.size())
        return QVariant();

    const TrackEntry &t = m_tracks.at(index.row());
    switch (role) {
    case TitleRole:
        return t.title;
    case ArtistRole:
        return t.artist;
    case AlbumRole:
        return t.album;
    case GenreRole:
        return t.genre;
    case PathRole:
        return t.url.toLocalFile();
    case UrlRole:
        return t.url;
    case LibraryRelPathRole:
        return t.libraryRelPath;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> TrackListModel::roleNames() const
{
    return {
        {TitleRole, "title"},
        {ArtistRole, "artist"},
        {AlbumRole, "album"},
        {GenreRole, "genre"},
        {PathRole, "path"},
        {UrlRole, "url"},
        {LibraryRelPathRole, "libraryRelPath"}
    };
}

bool TrackListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (parent.isValid() || count < 1 || row < 0 || row + count > m_tracks.size())
        return false;
    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for (int i = 0; i < count; ++i)
        m_tracks.removeAt(row);
    endRemoveRows();
    return true;
}

void TrackListModel::appendTracks(const QVector<TrackEntry> &tracks)
{
    if (tracks.isEmpty())
        return;

    const int first = m_tracks.size();
    const int count = tracks.size();
    beginInsertRows(QModelIndex(), first, first + count - 1);
    m_tracks += tracks;
    endInsertRows();
}

void TrackListModel::clear()
{
    if (m_tracks.isEmpty())
        return;
    beginRemoveRows(QModelIndex(), 0, m_tracks.size() - 1);
    m_tracks.clear();
    endRemoveRows();
}

QUrl TrackListModel::urlAt(int row) const
{
    if (row < 0 || row >= m_tracks.size())
        return {};
    return m_tracks.at(row).url;
}

QString TrackListModel::titleAt(int row) const
{
    if (row < 0 || row >= m_tracks.size())
        return {};
    return m_tracks.at(row).title;
}

QString TrackListModel::artistAt(int row) const
{
    if (row < 0 || row >= m_tracks.size())
        return {};
    return m_tracks.at(row).artist;
}

QString TrackListModel::albumAt(int row) const
{
    if (row < 0 || row >= m_tracks.size())
        return {};
    return m_tracks.at(row).album;
}

QString TrackListModel::genreAt(int row) const
{
    if (row < 0 || row >= m_tracks.size())
        return {};
    return m_tracks.at(row).genre;
}

QString TrackListModel::libraryRelPathAt(int row) const
{
    if (row < 0 || row >= m_tracks.size())
        return {};
    return m_tracks.at(row).libraryRelPath;
}
