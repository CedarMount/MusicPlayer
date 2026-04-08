#include "trackfiltermodel.h"
#include "tracklistmodel.h"

TrackFilterModel::TrackFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
}

void TrackFilterModel::setFilterText(const QString &text)
{
    if (m_filterText == text)
        return;
    m_filterText = text;
    invalidateFilter();
}

void TrackFilterModel::setCategoryFilter(const QString &artist, const QString &album, const QString &genre)
{
    if (m_catArtist == artist && m_catAlbum == album && m_catGenre == genre)
        return;
    m_catArtist = artist;
    m_catAlbum = album;
    m_catGenre = genre;
    invalidateFilter();
}

void TrackFilterModel::setTitleArtistOnly(bool on)
{
    if (m_titleArtistOnly == on)
        return;
    m_titleArtistOnly = on;
    invalidateFilter();
}

void TrackFilterModel::invalidateFiltering()
{
    invalidateFilter();
}

bool TrackFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (sourceParent.isValid())
        return false;
    const QAbstractItemModel *src = sourceModel();
    if (!src)
        return false;
    const QModelIndex idx = src->index(sourceRow, 0);
    const QString t = src->data(idx, TrackListModel::TitleRole).toString();
    const QString a = src->data(idx, TrackListModel::ArtistRole).toString();
    const QString al = src->data(idx, TrackListModel::AlbumRole).toString();
    const QString g = src->data(idx, TrackListModel::GenreRole).toString();

    const QString needle = m_filterText.trimmed();
    if (!needle.isEmpty()) {
        if (m_titleArtistOnly) {
            if (!t.contains(needle, Qt::CaseInsensitive) && !a.contains(needle, Qt::CaseInsensitive))
                return false;
        } else {
            if (!t.contains(needle, Qt::CaseInsensitive) && !a.contains(needle, Qt::CaseInsensitive)
                && !al.contains(needle, Qt::CaseInsensitive) && !g.contains(needle, Qt::CaseInsensitive))
                return false;
        }
    }

    if (!m_catArtist.isEmpty()
        && QString::compare(a, m_catArtist, Qt::CaseInsensitive) != 0)
        return false;
    if (!m_catAlbum.isEmpty()
        && QString::compare(al, m_catAlbum, Qt::CaseInsensitive) != 0)
        return false;
    if (!m_catGenre.isEmpty()
        && QString::compare(g, m_catGenre, Qt::CaseInsensitive) != 0)
        return false;
    return true;
}
