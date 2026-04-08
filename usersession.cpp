#include "usersession.h"

#include <QCoreApplication>
#include <QDebug>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QRandomGenerator>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUrl>
#include <QVariant>

int UserSession::recentTrackCount() const
{
    return m_recentModel ? m_recentModel->rowCount() : 0;
}

int UserSession::favoriteTrackCount() const
{
    return m_favoriteModel ? m_favoriteModel->rowCount() : 0;
}

UserSession::UserSession(QObject *parent)
    : QObject(parent)
    , m_connectionName(QStringLiteral("musicplayer_user_%1").arg(quintptr(this), 0, 16))
    , m_recentModel(new RecentTracksModel(this))
    , m_favoriteModel(new FavoriteTracksModel(this))
    , m_namedPlaylists(new NamedPlaylistsModel(this))
{
}

UserSession::~UserSession()
{
    if (QSqlDatabase::contains(m_connectionName)) {
        m_db.close();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool UserSession::openDatabase()
{
    if (m_db.isOpen())
        return true;

    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(base);
    const QString dbPath = base + QStringLiteral("/userdata.db");

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) {
        setLastError(tr("无法打开数据库：%1").arg(m_db.lastError().text()));
        return false;
    }

    QSqlQuery pragma(m_db);
    pragma.exec(QStringLiteral("PRAGMA foreign_keys = ON"));

    if (!createTables()) {
        m_db.close();
        QSqlDatabase::removeDatabase(m_connectionName);
        return false;
    }

    migrateSchema();

    reloadTrackModels();
    return true;
}

bool UserSession::createTables()
{
    QSqlQuery q(m_db);
    const QStringList statements = {
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS users ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "username TEXT NOT NULL UNIQUE,"
            "password_hash TEXT NOT NULL,"
            "salt BLOB NOT NULL,"
            "created_at INTEGER NOT NULL)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS favorites ("
            "user_id INTEGER NOT NULL,"
            "path TEXT NOT NULL,"
            "title TEXT,"
            "artist TEXT,"
            "added_at INTEGER NOT NULL,"
            "PRIMARY KEY (user_id, path),"
            "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS recent_plays ("
            "user_id INTEGER NOT NULL,"
            "path TEXT NOT NULL,"
            "title TEXT,"
            "artist TEXT,"
            "played_at INTEGER NOT NULL,"
            "PRIMARY KEY (user_id, path),"
            "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS library_tracks ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "rel_path TEXT NOT NULL UNIQUE,"
            "title TEXT,"
            "artist TEXT,"
            "added_at INTEGER NOT NULL)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS user_playlists ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "user_id INTEGER NOT NULL,"
            "name TEXT NOT NULL,"
            "created_at INTEGER NOT NULL,"
            "UNIQUE(user_id, name),"
            "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS playlist_items ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "playlist_id INTEGER NOT NULL,"
            "sort_order INTEGER NOT NULL,"
            "rel_path TEXT NOT NULL,"
            "title TEXT,"
            "artist TEXT,"
            "FOREIGN KEY (playlist_id) REFERENCES user_playlists(id) ON DELETE CASCADE,"
            "UNIQUE(playlist_id, rel_path))"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS search_history ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "user_id INTEGER NOT NULL,"
            "query TEXT NOT NULL,"
            "searched_at INTEGER NOT NULL)")};

    for (const QString &sql : statements) {
        if (!q.exec(sql)) {
            setLastError(tr("建表失败：%1").arg(q.lastError().text()));
            return false;
        }
    }
    return true;
}

void UserSession::migrateSchema()
{
    static const char *const kAlters[] = {
        "ALTER TABLE users ADD COLUMN nickname TEXT",
        "ALTER TABLE users ADD COLUMN avatar_path TEXT",
        "ALTER TABLE users ADD COLUMN theme_dark INTEGER NOT NULL DEFAULT 1",
        "ALTER TABLE library_tracks ADD COLUMN album TEXT",
        "ALTER TABLE library_tracks ADD COLUMN genre TEXT",
    };
    for (const char *sql : kAlters) {
        QSqlQuery q(m_db);
        q.exec(QString::fromUtf8(sql));
    }
    migrateRecentPlaysIfNeeded();
}

void UserSession::migrateRecentPlaysIfNeeded()
{
    if (!m_db.isOpen())
        return;

    QSqlQuery exists(m_db);
    if (!exists.exec(QStringLiteral(
            "SELECT 1 FROM sqlite_master WHERE type='table' AND name='recent_plays' LIMIT 1"))
        || !exists.next())
        return;

    QSqlQuery sch(m_db);
    QString ddl;
    if (sch.exec(QStringLiteral("SELECT sql FROM sqlite_master WHERE type='table' AND name='recent_plays'"))
        && sch.next())
        ddl = sch.value(0).toString();

    const bool ddlLooksLikeCompositePk = ddl.contains(QLatin1String("PRIMARY KEY"), Qt::CaseInsensitive)
        && ddl.contains(QLatin1String("user_id"), Qt::CaseInsensitive)
        && ddl.contains(QLatin1String("path"), Qt::CaseInsensitive);

    int total = 0;
    int distinctPairs = 0;
    QSqlQuery cnt(m_db);
    if (cnt.exec(QStringLiteral("SELECT COUNT(*) FROM recent_plays")) && cnt.next())
        total = cnt.value(0).toInt();
    if (cnt.exec(QStringLiteral(
            "SELECT COUNT(DISTINCT user_id || char(31) || path) FROM recent_plays"))
        && cnt.next())
        distinctPairs = cnt.value(0).toInt();

    if (ddlLooksLikeCompositePk && total == distinctPairs)
        return;

    if (!m_db.transaction()) {
        qWarning() << "migrateRecentPlaysIfNeeded: transaction failed";
        return;
    }

    QSqlQuery q(m_db);
    q.exec(QStringLiteral("DROP TABLE IF EXISTS recent_plays__new"));

    if (!q.exec(QStringLiteral(
            "CREATE TABLE recent_plays__new ("
            "user_id INTEGER NOT NULL,"
            "path TEXT NOT NULL,"
            "title TEXT,"
            "artist TEXT,"
            "played_at INTEGER NOT NULL,"
            "PRIMARY KEY (user_id, path),"
            "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE)"))) {
        qWarning() << "migrateRecentPlaysIfNeeded CREATE:" << q.lastError().text();
        m_db.rollback();
        return;
    }

    if (!q.exec(QStringLiteral(
            "INSERT INTO recent_plays__new (user_id, path, title, artist, played_at) "
            "SELECT r.user_id, r.path, r.title, r.artist, r.played_at FROM recent_plays r "
            "INNER JOIN ("
            "  SELECT user_id, path, MAX(played_at) AS mx FROM recent_plays GROUP BY user_id, path"
            ") t ON r.user_id = t.user_id AND r.path = t.path AND r.played_at = t.mx "
            "GROUP BY r.user_id, r.path"))) {
        qWarning() << "migrateRecentPlaysIfNeeded INSERT:" << q.lastError().text();
        m_db.rollback();
        return;
    }

    if (!q.exec(QStringLiteral("DROP TABLE recent_plays"))) {
        qWarning() << "migrateRecentPlaysIfNeeded DROP:" << q.lastError().text();
        m_db.rollback();
        return;
    }
    if (!q.exec(QStringLiteral("ALTER TABLE recent_plays__new RENAME TO recent_plays"))) {
        qWarning() << "migrateRecentPlaysIfNeeded RENAME:" << q.lastError().text();
        m_db.rollback();
        return;
    }

    if (!m_db.commit())
        qWarning() << "migrateRecentPlaysIfNeeded commit:" << m_db.lastError().text();
}

void UserSession::setLastError(const QString &e)
{
    m_lastError = e;
    emit lastErrorChanged();
}

void UserSession::clearLastError()
{
    if (m_lastError.isEmpty())
        return;
    m_lastError.clear();
    emit lastErrorChanged();
}

QString UserSession::storagePathKey(const QString &localPath)
{
    return normalizeStoragePath(localPath);
}

QString UserSession::normalizeStoragePath(const QString &localPath)
{
    if (localPath.isEmpty())
        return {};
    const QFileInfo fi(localPath);
    QString p = fi.canonicalFilePath();
    if (!p.isEmpty())
        return p;
    p = QDir::cleanPath(fi.absoluteFilePath());
    if (!p.isEmpty())
        return p;
    return QDir::cleanPath(localPath);
}

QByteArray UserSession::randomSalt(int length)
{
    QByteArray salt;
    salt.resize(length);
    for (int i = 0; i < length; ++i)
        salt[i] = char(QRandomGenerator::global()->bounded(256));
    return salt;
}

QByteArray UserSession::hashPassword(const QString &password, const QByteArray &salt)
{
    return QCryptographicHash::hash(salt + password.toUtf8(), QCryptographicHash::Sha256).toHex();
}

bool UserSession::registerAccount(const QString &username, const QString &password)
{
    clearLastError();
    if (!m_db.isOpen()) {
        setLastError(tr("数据库未就绪"));
        return false;
    }
    const QString u = username.trimmed();
    if (u.size() < 2) {
        setLastError(tr("账号至少 2 个字符"));
        return false;
    }
    if (password.size() < 4) {
        setLastError(tr("密码至少 4 位"));
        return false;
    }

    const QByteArray salt = randomSalt();
    const QByteArray hash = hashPassword(password, salt);

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO users (username, password_hash, salt, created_at) VALUES (?, ?, ?, ?)"));
    q.addBindValue(u);
    q.addBindValue(QString::fromLatin1(hash));
    q.addBindValue(salt);
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    if (!q.exec()) {
        const QString et = q.lastError().text();
        if (et.contains(QLatin1String("UNIQUE"), Qt::CaseInsensitive))
            setLastError(tr("该账号已存在"));
        else
            setLastError(tr("注册失败：%1").arg(et));
        return false;
    }
    return true;
}

bool UserSession::login(const QString &username, const QString &password)
{
    clearLastError();
    if (!m_db.isOpen()) {
        setLastError(tr("数据库未就绪"));
        return false;
    }
    const QString u = username.trimmed();
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT id, password_hash, salt, nickname, avatar_path, theme_dark "
        "FROM users WHERE username = ?"));
    q.addBindValue(u);
    if (!q.exec() || !q.next()) {
        setLastError(tr("账号或密码错误"));
        return false;
    }
    const int id = q.value(0).toInt();
    const QByteArray storedHex = q.value(1).toString().toLatin1();
    const QByteArray salt = q.value(2).toByteArray();
    const QByteArray h = hashPassword(password, salt);
    if (h != storedHex) {
        setLastError(tr("账号或密码错误"));
        return false;
    }

    m_userId = id;
    m_username = u;
    m_nickname = q.value(3).toString();
    m_avatarPath = q.value(4).toString();
    m_prefersDarkTheme = q.value(5).isNull() ? true : (q.value(5).toInt() != 0);
    emit loginStateChanged();
    emit profileChanged();
    emit prefersDarkThemeChanged();
    reloadTrackModels();
    return true;
}

void UserSession::logout()
{
    if (!isLoggedIn())
        return;
    m_userId = 0;
    m_username.clear();
    m_nickname.clear();
    m_avatarPath.clear();
    m_prefersDarkTheme = true;
    emit loginStateChanged();
    emit profileChanged();
    emit prefersDarkThemeChanged();
    reloadTrackModels();
}

void UserSession::reloadTrackModels()
{
    m_recentModel->reloadFromDb(m_db, m_userId);
    m_favoriteModel->reloadFromDb(m_db, m_userId);
    m_namedPlaylists->reloadFromDb(m_db, m_userId);
    emit trackListsChanged();
    emit playlistsModelChanged();
    reloadSearchHistory();
}

void UserSession::reloadSearchHistory()
{
    m_searchHistory.clear();
    if (!m_db.isOpen()) {
        emit searchHistoryChanged();
        return;
    }
    const int uid = m_userId > 0 ? m_userId : 0;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "SELECT query FROM search_history WHERE user_id = ? ORDER BY searched_at DESC LIMIT 80"));
    q.addBindValue(uid);
    if (!q.exec()) {
        qWarning() << "reloadSearchHistory:" << q.lastError().text();
        emit searchHistoryChanged();
        return;
    }
    QStringList seen;
    while (q.next()) {
        const QString s = q.value(0).toString().trimmed();
        if (s.isEmpty() || seen.contains(s))
            continue;
        seen.append(s);
        m_searchHistory.append(s);
        if (m_searchHistory.size() >= 24)
            break;
    }
    emit searchHistoryChanged();
}

void UserSession::recordSearchQuery(const QString &query)
{
    const QString t = query.trimmed();
    if (t.isEmpty() || !m_db.isOpen())
        return;
    const int uid = m_userId > 0 ? m_userId : 0;
    QSqlQuery del(m_db);
    del.prepare(QStringLiteral("DELETE FROM search_history WHERE user_id = ? AND query = ?"));
    del.addBindValue(uid);
    del.addBindValue(t);
    if (!del.exec())
        qWarning() << "recordSearchQuery DELETE:" << del.lastError().text();
    QSqlQuery ins(m_db);
    ins.prepare(QStringLiteral(
        "INSERT INTO search_history (user_id, query, searched_at) VALUES (?,?,?)"));
    ins.addBindValue(uid);
    ins.addBindValue(t);
    ins.addBindValue(QDateTime::currentMSecsSinceEpoch());
    if (!ins.exec())
        qWarning() << "recordSearchQuery INSERT:" << ins.lastError().text();
    reloadSearchHistory();
}

void UserSession::clearSearchHistory()
{
    if (!m_db.isOpen())
        return;
    const int uid = m_userId > 0 ? m_userId : 0;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM search_history WHERE user_id = ?"));
    q.addBindValue(uid);
    if (!q.exec())
        qWarning() << "clearSearchHistory:" << q.lastError().text();
    reloadSearchHistory();
}

bool UserSession::isFavoritePath(const QString &localPath) const
{
    if (!isLoggedIn() || !m_db.isOpen())
        return false;
    const QString key = normalizeStoragePath(localPath);
    if (key.isEmpty())
        return false;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("SELECT 1 FROM favorites WHERE user_id = ? AND path = ? LIMIT 1"));
    q.addBindValue(m_userId);
    q.addBindValue(key);
    return q.exec() && q.next();
}

void UserSession::setFavorite(const QString &localPath, bool favorite, const QString &title,
                              const QString &artist)
{
    if (!isLoggedIn() || !m_db.isOpen())
        return;
    const QString key = normalizeStoragePath(localPath);
    if (key.isEmpty())
        return;

    QSqlQuery q(m_db);
    if (favorite) {
        q.prepare(QStringLiteral(
            "INSERT OR REPLACE INTO favorites (user_id, path, title, artist, added_at) "
            "VALUES (?, ?, ?, ?, ?)"));
        q.addBindValue(m_userId);
        q.addBindValue(key);
        q.addBindValue(title);
        q.addBindValue(artist);
        q.addBindValue(QDateTime::currentMSecsSinceEpoch());
        if (!q.exec())
            qWarning() << "setFavorite INSERT:" << q.lastError().text();
    } else {
        q.prepare(QStringLiteral("DELETE FROM favorites WHERE user_id = ? AND path = ?"));
        q.addBindValue(m_userId);
        q.addBindValue(key);
        if (!q.exec())
            qWarning() << "setFavorite DELETE:" << q.lastError().text();
    }
    m_favoriteModel->reloadFromDb(m_db, m_userId);
    emit favoritesChanged();
    emit trackListsChanged();
}

void UserSession::toggleFavorite(const QString &localPath, const QString &title,
                                 const QString &artist)
{
    if (!isLoggedIn())
        return;
    const bool on = !isFavoritePath(localPath);
    setFavorite(localPath, on, title, artist);
}

void UserSession::recordRecentPlay(const QString &localPath, const QString &title,
                                   const QString &artist)
{
    if (!isLoggedIn() || !m_db.isOpen())
        return;
    const QString key = normalizeStoragePath(localPath);
    if (key.isEmpty())
        return;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT OR REPLACE INTO recent_plays (user_id, path, title, artist, played_at) "
        "VALUES (?, ?, ?, ?, ?)"));
    q.addBindValue(m_userId);
    q.addBindValue(key);
    q.addBindValue(title);
    q.addBindValue(artist);
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    if (!q.exec()) {
        qWarning() << "recordRecentPlay SQL:" << q.lastError().text();
        return;
    }
    m_recentModel->reloadFromDb(m_db, m_userId);
    emit trackListsChanged();
}

void UserSession::removeRecentPlay(const QString &localPath)
{
    if (!isLoggedIn() || !m_db.isOpen())
        return;
    const QString key = normalizeStoragePath(localPath);
    if (key.isEmpty())
        return;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM recent_plays WHERE user_id = ? AND path = ?"));
    q.addBindValue(m_userId);
    q.addBindValue(key);
    if (!q.exec()) {
        qWarning() << "removeRecentPlay:" << q.lastError().text();
        return;
    }
    m_recentModel->reloadFromDb(m_db, m_userId);
    emit trackListsChanged();
}

void UserSession::clearRecentPlays()
{
    if (!isLoggedIn() || !m_db.isOpen())
        return;

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM recent_plays WHERE user_id = ?"));
    q.addBindValue(m_userId);
    if (!q.exec()) {
        qWarning() << "clearRecentPlays:" << q.lastError().text();
        return;
    }
    m_recentModel->reloadFromDb(m_db, m_userId);
    emit trackListsChanged();
}

void UserSession::removeFavoritesRecentForStorageKey(const QString &pathKey)
{
    if (!m_db.isOpen() || pathKey.isEmpty())
        return;
    QSqlQuery d1(m_db);
    d1.prepare(QStringLiteral("DELETE FROM favorites WHERE path = ?"));
    d1.addBindValue(pathKey);
    d1.exec();
    QSqlQuery d2(m_db);
    d2.prepare(QStringLiteral("DELETE FROM recent_plays WHERE path = ?"));
    d2.addBindValue(pathKey);
    d2.exec();
    if (m_userId > 0) {
        m_recentModel->reloadFromDb(m_db, m_userId);
        m_favoriteModel->reloadFromDb(m_db, m_userId);
        emit trackListsChanged();
        emit favoritesChanged();
    }
}

void UserSession::pruneStaleFavoritesAndRecent()
{
    if (!m_db.isOpen())
        return;
    QSet<QString> stale;
    QSqlQuery qf(m_db);
    if (!qf.exec(QStringLiteral("SELECT DISTINCT path FROM favorites"))) {
        qWarning() << "pruneStaleFavoritesAndRecent favorites:" << qf.lastError().text();
        return;
    }
    while (qf.next()) {
        const QString p = qf.value(0).toString();
        if (p.isEmpty() || !QFileInfo::exists(p))
            stale.insert(p);
    }
    QSqlQuery qr(m_db);
    if (!qr.exec(QStringLiteral("SELECT DISTINCT path FROM recent_plays"))) {
        qWarning() << "pruneStaleFavoritesAndRecent recent_plays:" << qr.lastError().text();
        return;
    }
    while (qr.next()) {
        const QString p = qr.value(0).toString();
        if (p.isEmpty() || !QFileInfo::exists(p))
            stale.insert(p);
    }
    if (stale.isEmpty())
        return;
    for (const QString &p : stale) {
        QSqlQuery d1(m_db);
        d1.prepare(QStringLiteral("DELETE FROM favorites WHERE path = ?"));
        d1.addBindValue(p);
        d1.exec();
        QSqlQuery d2(m_db);
        d2.prepare(QStringLiteral("DELETE FROM recent_plays WHERE path = ?"));
        d2.addBindValue(p);
        d2.exec();
    }
    if (m_userId > 0) {
        m_recentModel->reloadFromDb(m_db, m_userId);
        m_favoriteModel->reloadFromDb(m_db, m_userId);
        emit trackListsChanged();
        emit favoritesChanged();
    }
}

QString UserSession::displayName() const
{
    const QString n = m_nickname.trimmed();
    return n.isEmpty() ? m_username : n;
}

QString UserSession::avatarUrl() const
{
    if (m_avatarPath.isEmpty() || !QFileInfo::exists(m_avatarPath))
        return {};
    return QUrl::fromLocalFile(m_avatarPath).toString();
}

bool UserSession::prefersDarkTheme() const
{
    if (m_userId > 0)
        return m_prefersDarkTheme;
    // 未登录：始终默认深色，不沿用上次已登录用户或 QSettings 中的浅色
    return true;
}

void UserSession::setPrefersDarkTheme(bool dark)
{
    if (m_userId <= 0)
        return;
    m_prefersDarkTheme = dark;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE users SET theme_dark = ? WHERE id = ?"));
    q.addBindValue(dark ? 1 : 0);
    q.addBindValue(m_userId);
    if (!q.exec())
        qWarning() << "setPrefersDarkTheme:" << q.lastError().text();
    emit prefersDarkThemeChanged();
}

int UserSession::namedPlaylistCount() const
{
    return m_namedPlaylists ? m_namedPlaylists->rowCount() : 0;
}

bool UserSession::saveNickname(const QString &nickname)
{
    if (!isLoggedIn() || !m_db.isOpen())
        return false;
    const QString n = nickname.trimmed();
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE users SET nickname = ? WHERE id = ?"));
    q.addBindValue(n.isEmpty() ? QVariant() : QVariant(n));
    q.addBindValue(m_userId);
    if (!q.exec()) {
        qWarning() << "saveNickname:" << q.lastError().text();
        return false;
    }
    m_nickname = n;
    emit profileChanged();
    return true;
}

bool UserSession::pickAvatar()
{
    if (!isLoggedIn() || !m_db.isOpen())
        return false;
    const QString path = QFileDialog::getOpenFileName(
        nullptr,
        tr("选择头像图片"),
        QDir::homePath(),
        tr("图片 (*.png *.jpg *.jpeg *.bmp)"));
    if (path.isEmpty())
        return false;

    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString dir = base + QStringLiteral("/avatars");
    QDir().mkpath(dir);
    const QString suf = QFileInfo(path).suffix().toLower();
    const QString dst = dir + QLatin1Char('/') + QString::number(m_userId) + QLatin1Char('.') + suf;
    if (QFile::exists(dst))
        QFile::remove(dst);
    if (!QFile::copy(path, dst)) {
        qWarning() << "pickAvatar copy failed";
        return false;
    }

    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE users SET avatar_path = ? WHERE id = ?"));
    q.addBindValue(dst);
    q.addBindValue(m_userId);
    if (!q.exec()) {
        qWarning() << "pickAvatar UPDATE:" << q.lastError().text();
        return false;
    }
    m_avatarPath = dst;
    emit profileChanged();
    return true;
}

bool UserSession::createNamedPlaylist(const QString &name)
{
    if (!isLoggedIn() || !m_db.isOpen())
        return false;
    const QString n = name.trimmed();
    if (n.size() < 1) {
        setLastError(tr("歌单名称不能为空"));
        return false;
    }
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral(
        "INSERT INTO user_playlists (user_id, name, created_at) VALUES (?, ?, ?)"));
    q.addBindValue(m_userId);
    q.addBindValue(n);
    q.addBindValue(QDateTime::currentMSecsSinceEpoch());
    if (!q.exec()) {
        if (q.lastError().text().contains(QLatin1String("UNIQUE"), Qt::CaseInsensitive))
            setLastError(tr("已有同名歌单"));
        else
            setLastError(tr("创建失败：%1").arg(q.lastError().text()));
        return false;
    }
    clearLastError();
    m_namedPlaylists->reloadFromDb(m_db, m_userId);
    emit playlistsModelChanged();
    return true;
}

void UserSession::deleteNamedPlaylist(int playlistId)
{
    if (!isLoggedIn() || !m_db.isOpen() || playlistId <= 0)
        return;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("DELETE FROM user_playlists WHERE id = ? AND user_id = ?"));
    q.addBindValue(playlistId);
    q.addBindValue(m_userId);
    q.exec();
    m_namedPlaylists->reloadFromDb(m_db, m_userId);
    emit playlistsModelChanged();
}

bool UserSession::renameNamedPlaylist(int playlistId, const QString &name)
{
    if (!isLoggedIn() || !m_db.isOpen() || playlistId <= 0)
        return false;
    const QString n = name.trimmed();
    if (n.isEmpty())
        return false;
    QSqlQuery q(m_db);
    q.prepare(QStringLiteral("UPDATE user_playlists SET name = ? WHERE id = ? AND user_id = ?"));
    q.addBindValue(n);
    q.addBindValue(playlistId);
    q.addBindValue(m_userId);
    if (!q.exec()) {
        setLastError(tr("重命名失败：%1").arg(q.lastError().text()));
        return false;
    }
    clearLastError();
    m_namedPlaylists->reloadFromDb(m_db, m_userId);
    emit playlistsModelChanged();
    return true;
}

bool UserSession::addTrackToNamedPlaylist(int playlistId, const QString &relPath, const QString &title,
                                          const QString &artist)
{
    if (!isLoggedIn() || !m_db.isOpen() || playlistId <= 0 || relPath.isEmpty())
        return false;

    QSqlQuery ck(m_db);
    ck.prepare(QStringLiteral("SELECT 1 FROM user_playlists WHERE id = ? AND user_id = ?"));
    ck.addBindValue(playlistId);
    ck.addBindValue(m_userId);
    if (!ck.exec() || !ck.next())
        return false;

    QSqlQuery qmax(m_db);
    qmax.prepare(QStringLiteral(
        "SELECT COALESCE(MAX(sort_order), -1) + 1 FROM playlist_items WHERE playlist_id = ?"));
    qmax.addBindValue(playlistId);
    int nextOrder = 0;
    if (qmax.exec() && qmax.next())
        nextOrder = qmax.value(0).toInt();

    QSqlQuery ins(m_db);
    ins.prepare(QStringLiteral(
        "INSERT OR IGNORE INTO playlist_items (playlist_id, sort_order, rel_path, title, artist) "
        "VALUES (?, ?, ?, ?, ?)"));
    ins.addBindValue(playlistId);
    ins.addBindValue(nextOrder);
    ins.addBindValue(relPath);
    ins.addBindValue(title);
    ins.addBindValue(artist);
    if (!ins.exec()) {
        qWarning() << "addTrackToNamedPlaylist:" << ins.lastError().text();
        return false;
    }
    return ins.numRowsAffected() > 0;
}

bool UserSession::removeTrackFromNamedPlaylist(int playlistId, const QString &relPath)
{
    if (!isLoggedIn() || !m_db.isOpen() || playlistId <= 0 || relPath.isEmpty())
        return false;

    QSqlQuery ck(m_db);
    ck.prepare(QStringLiteral("SELECT 1 FROM user_playlists WHERE id = ? AND user_id = ?"));
    ck.addBindValue(playlistId);
    ck.addBindValue(m_userId);
    if (!ck.exec() || !ck.next())
        return false;

    QSqlQuery dq(m_db);
    dq.prepare(QStringLiteral("DELETE FROM playlist_items WHERE playlist_id = ? AND rel_path = ?"));
    dq.addBindValue(playlistId);
    dq.addBindValue(relPath);
    if (!dq.exec()) {
        qWarning() << "removeTrackFromNamedPlaylist:" << dq.lastError().text();
        return false;
    }
    return dq.numRowsAffected() > 0;
}
