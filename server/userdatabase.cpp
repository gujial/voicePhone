#include "userdatabase.h"
#include "../src/crypto.h"
#include <QDebug>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QVariant>

UserDatabase::UserDatabase(QObject *parent)
    : QObject(parent)
{
}

UserDatabase::~UserDatabase()
{
    if (m_database.isOpen()) {
        m_database.close();
    }
}

bool UserDatabase::initialize(const QString &dbPath)
{
    // 创建SQLite数据库连接
    m_database = QSqlDatabase::addDatabase("QSQLITE");
    m_database.setDatabaseName(dbPath);
    
    if (!m_database.open()) {
        qCritical() << "Failed to open database:" << m_database.lastError().text();
        return false;
    }
    
    qInfo() << "Database opened:" << dbPath;
    
    // 创建表
    if (!createTables()) {
        qCritical() << "Failed to create tables";
        return false;
    }
    
    // 从数据库加载用户
    if (!loadUsersFromDatabase()) {
        qCritical() << "Failed to load users from database";
        return false;
    }
    
    // 如果没有用户，创建默认管理员账号
    if (m_users.isEmpty()) {
        qInfo() << "No users found, creating default admin account";
        QByteArray adminPasswordHash = CryptoUtils::hashPassword("admin_pass");
        registerUser("admin", adminPasswordHash, UserType::Administrator);
        
        // // 创建测试用户
        // QByteArray testPasswordHash = CryptoUtils::hashPassword("password");
        // registerUser("user1", testPasswordHash, UserType::User);
        // registerUser("user2", testPasswordHash, UserType::User);
    }
    
    return true;
}

bool UserDatabase::createTables()
{
    QSqlQuery query(m_database);
    
    // 创建用户表
    QString createUsersTable = R"(
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            user_type INTEGER NOT NULL DEFAULT 0,
            created_at TEXT NOT NULL,
            last_login TEXT
        )
    )";
    
    if (!query.exec(createUsersTable)) {
        qCritical() << "Failed to create users table:" << query.lastError().text();
        return false;
    }
    
    qInfo() << "Database tables created successfully";
    return true;
}

bool UserDatabase::loadUsersFromDatabase()
{
    QSqlQuery query(m_database);
    
    if (!query.exec("SELECT username, password_hash, user_type, created_at FROM users")) {
        qCritical() << "Failed to load users:" << query.lastError().text();
        return false;
    }
    
    m_users.clear();
    int count = 0;
    
    while (query.next()) {
        UserCredentials cred;
        cred.username = query.value(0).toString();
        cred.passwordHash = QByteArray::fromHex(query.value(1).toByteArray());
        cred.userType = static_cast<UserType>(query.value(2).toInt());
        cred.createdAt = query.value(3).toString();
        
        m_users[cred.username] = cred;
        count++;
    }
    
    qInfo() << "Loaded" << count << "users from database";
    return true;
}

bool UserDatabase::saveUserToDatabase(const UserCredentials &user)
{
    QSqlQuery query(m_database);
    
    query.prepare("INSERT INTO users (username, password_hash, user_type, created_at) "
                  "VALUES (:username, :password_hash, :user_type, :created_at)");
    query.bindValue(":username", user.username);
    query.bindValue(":password_hash", QString::fromUtf8(user.passwordHash.toHex()));
    query.bindValue(":user_type", static_cast<int>(user.userType));
    query.bindValue(":created_at", user.createdAt);
    
    if (!query.exec()) {
        qCritical() << "Failed to save user to database:" << query.lastError().text();
        return false;
    }
    
    return true;
}

bool UserDatabase::registerUser(const QString &username, const QByteArray &passwordHash, UserType userType)
{
    if (username.isEmpty() || passwordHash.isEmpty()) {
        qWarning() << "Invalid username or password hash";
        return false;
    }
    
    if (m_users.contains(username)) {
        qWarning() << "User already exists:" << username;
        return false;
    }
    
    UserCredentials cred;
    cred.username = username;
    cred.passwordHash = passwordHash;
    cred.userType = userType;
    cred.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    // 保存到数据库
    if (!saveUserToDatabase(cred)) {
        return false;
    }
    
    // 添加到内存缓存
    m_users[username] = cred;
    
    QString userTypeStr = (userType == UserType::Administrator) ? "Administrator" : "User";
    qInfo() << "User registered:" << username << "Type:" << userTypeStr;
    
    return true;
}

bool UserDatabase::authenticate(const QString &username, const QByteArray &passwordHash)
{
    if (!m_users.contains(username)) {
        qWarning() << "User not found:" << username;
        return false;
    }
    
    const UserCredentials &cred = m_users[username];
    bool valid = (cred.passwordHash == passwordHash);
    
    if (valid) {
        // 更新最后登录时间
        QSqlQuery query(m_database);
        query.prepare("UPDATE users SET last_login = :last_login WHERE username = :username");
        query.bindValue(":last_login", QDateTime::currentDateTime().toString(Qt::ISODate));
        query.bindValue(":username", username);
        query.exec();
        
        QString userTypeStr = (cred.userType == UserType::Administrator) ? "Admin" : "User";
        qInfo() << "Authentication successful for:" << username << "(" << userTypeStr << ")";
    } else {
        qWarning() << "Authentication failed for:" << username;
    }
    
    return valid;
}

bool UserDatabase::userExists(const QString &username) const
{
    return m_users.contains(username);
}

UserCredentials UserDatabase::getUserInfo(const QString &username) const
{
    return m_users.value(username, UserCredentials());
}

UserType UserDatabase::getUserType(const QString &username) const
{
    if (!m_users.contains(username)) {
        return UserType::User;
    }
    return m_users[username].userType;
}

bool UserDatabase::setUserType(const QString &username, UserType userType)
{
    if (!m_users.contains(username)) {
        qWarning() << "User not found:" << username;
        return false;
    }
    
    // 更新数据库
    QSqlQuery query(m_database);
    query.prepare("UPDATE users SET user_type = :user_type WHERE username = :username");
    query.bindValue(":user_type", static_cast<int>(userType));
    query.bindValue(":username", username);
    
    if (!query.exec()) {
        qCritical() << "Failed to update user type:" << query.lastError().text();
        return false;
    }
    
    // 更新缓存
    m_users[username].userType = userType;
    
    QString userTypeStr = (userType == UserType::Administrator) ? "Administrator" : "User";
    qInfo() << "User type updated:" << username << "→" << userTypeStr;
    
    return true;
}

QStringList UserDatabase::getAllUsers() const
{
    return m_users.keys();
}

QString UserDatabase::createSession(const QString &username, const QByteArray &token)
{
    QString sessionId = QString::fromUtf8(token.toHex());
    m_sessions[sessionId] = username;
    qInfo() << "Session created for:" << username << "ID:" << sessionId.left(16) + "...";
    return sessionId;
}

bool UserDatabase::validateSession(const QString &sessionId)
{
    return m_sessions.contains(sessionId);
}

QString UserDatabase::getUsernameFromSession(const QString &sessionId)
{
    return m_sessions.value(sessionId, QString());
}

void UserDatabase::removeSession(const QString &sessionId)
{
    QString username = m_sessions.value(sessionId);
    m_sessions.remove(sessionId);
    m_sessionKeys.remove(sessionId);
    
    if (!username.isEmpty()) {
        qInfo() << "Session removed for:" << username;
    }
}

void UserDatabase::setSessionKey(const QString &sessionId, const QByteArray &key)
{
    m_sessionKeys[sessionId] = key;
}

QByteArray UserDatabase::getSessionKey(const QString &sessionId) const
{
    return m_sessionKeys.value(sessionId, QByteArray());
}
