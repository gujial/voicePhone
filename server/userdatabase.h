#ifndef USERDATABASE_H
#define USERDATABASE_H

#include <QObject>
#include <QMap>
#include <QByteArray>
#include <QString>
#include <QSqlDatabase>

enum class UserType {
    User = 0,
    Administrator = 1
};

struct UserCredentials {
    QString username;
    QByteArray passwordHash;
    UserType userType = UserType::User;
    QString createdAt;
};

class UserDatabase : public QObject
{
    Q_OBJECT
public:
    explicit UserDatabase(QObject *parent = nullptr);
    ~UserDatabase();
    
    // 初始化数据库
    bool initialize(const QString &dbPath = "voicephone.db");
    
    // 用户注册 (添加新用户)
    bool registerUser(const QString &username, const QByteArray &passwordHash, UserType userType = UserType::User);
    
    // 验证用户凭据
    bool authenticate(const QString &username, const QByteArray &passwordHash);
    
    // 检查用户是否存在
    bool userExists(const QString &username) const;
    
    // 获取用户信息
    UserCredentials getUserInfo(const QString &username) const;
    
    // 获取用户类型
    UserType getUserType(const QString &username) const;
    bool setUserType(const QString &username, UserType userType);

    // 获取所有用户列表
    QStringList getAllUsers() const;
    
    // 管理会话令牌
    QString createSession(const QString &username, const QByteArray &token);
    bool validateSession(const QString &sessionId);
    QString getUsernameFromSession(const QString &sessionId);
    void removeSession(const QString &sessionId);
    
    // 管理会话密钥
    void setSessionKey(const QString &sessionId, const QByteArray &key);
    QByteArray getSessionKey(const QString &sessionId) const;

private:
    bool createTables();
    bool loadUsersFromDatabase();
    bool saveUserToDatabase(const UserCredentials &user);
    
    QSqlDatabase m_database;
    QMap<QString, UserCredentials> m_users; // username -> credentials (缓存)
    QMap<QString, QString> m_sessions; // sessionId -> username
    QMap<QString, QByteArray> m_sessionKeys; // sessionId -> encryption key
};

#endif // USERDATABASE_H
