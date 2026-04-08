#ifndef TRACKFILTERMODEL_H
#define TRACKFILTERMODEL_H

#include <QSortFilterProxyModel>

class TrackFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit TrackFilterModel(QObject *parent = nullptr);

    void setFilterText(const QString &text);
    QString filterText() const { return m_filterText; }

    /// 与搜索条件同时生效；空字符串表示不按该维度筛选
    void setCategoryFilter(const QString &artist, const QString &album, const QString &genre);

    /// true：关键词仅匹配歌名、歌手；false：同时匹配专辑、风格（曲库浏览）
    void setTitleArtistOnly(bool on);
    bool titleArtistOnly() const { return m_titleArtistOnly; }

    /// 供外部在无法通过 setFilterText 触发刷新时重新跑过滤（invalidateFilter 在基类中为 protected）
    void invalidateFiltering();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QString m_filterText;
    QString m_catArtist;
    QString m_catAlbum;
    QString m_catGenre;
    bool m_titleArtistOnly = false;
};

#endif // TRACKFILTERMODEL_H
