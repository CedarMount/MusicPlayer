#ifndef USERTRACKSMODELS_H
#define USERTRACKSMODELS_H

#include <QAbstractListModel>
#include <QString>
#include <QVector>

class QSqlDatabase;

struct UserTrackRow {
    QString path;
    QString title;
    QString artist;
    qint64 sortTime = 0;
};

class RecentTracksModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles { PathRole = Qt::UserRole + 1, TitleRole, ArtistRole, TimeRole };

    explicit RecentTracksModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void reloadFromDb(QSqlDatabase &db, int userId);

private:
    QVector<UserTrackRow> m_rows;
};

class FavoriteTracksModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles { PathRole = Qt::UserRole + 1, TitleRole, ArtistRole, TimeRole };

    explicit FavoriteTracksModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void reloadFromDb(QSqlDatabase &db, int userId);

private:
    QVector<UserTrackRow> m_rows;
};

class NamedPlaylistsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles { PlaylistIdRole = Qt::UserRole + 1, NameRole };

    explicit NamedPlaylistsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void reloadFromDb(QSqlDatabase &db, int userId);

private:
    QVector<QPair<int, QString>> m_rows;
};

/// 歌单浏览用：仅索引曲目路径，不改动主曲库/播放队列（与「我的喜欢」列表类似）
class PlaylistItemsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        TitleRole,
        ArtistRole,
        SortOrderRole,
        RelPathRole,
    };

    explicit PlaylistItemsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// 歌单不存在或无权访问返回 false；无曲目也返回 true
    bool loadFromPlaylist(QSqlDatabase &db, const QString &musicRoot, int userId, int playlistId);
    void clear();

private:
    struct Item {
        QString relPath;
        QString pathKey;
        QString title;
        QString artist;
        int sortOrder = 0;
    };
    QVector<Item> m_items;
};

Q_DECLARE_METATYPE(RecentTracksModel *)
Q_DECLARE_METATYPE(FavoriteTracksModel *)
Q_DECLARE_METATYPE(NamedPlaylistsModel *)
Q_DECLARE_METATYPE(PlaylistItemsModel *)

#endif // USERTRACKSMODELS_H
