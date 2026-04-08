#ifndef TRACKLISTMODEL_H
#define TRACKLISTMODEL_H

#include <QAbstractListModel>
#include <QUrl>
#include <QVector>

struct TrackEntry {
    QUrl url;
    QString title;
    QString artist;
    QString album;
    QString genre;
    /// 曲库相对 Music 目录的文件名；空表示未入库（如从最近播放临时加入）
    QString libraryRelPath;
};

class TrackListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        ArtistRole,
        AlbumRole,
        GenreRole,
        PathRole,
        UrlRole,
        LibraryRelPathRole
    };

    explicit TrackListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex()) override;

    void appendTracks(const QVector<TrackEntry> &tracks);
    void clear();

    QUrl urlAt(int row) const;
    QString titleAt(int row) const;
    QString artistAt(int row) const;
    QString albumAt(int row) const;
    QString genreAt(int row) const;
    QString libraryRelPathAt(int row) const;

private:
    QVector<TrackEntry> m_tracks;
};

#endif // TRACKLISTMODEL_H
