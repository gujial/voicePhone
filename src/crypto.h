#ifndef CRYPTO_H
#define CRYPTO_H

#include <QString>
#include <QByteArray>

class CryptoUtils
{
public:
    // 密码哈希 (SHA-256)
    static QByteArray hashPassword(const QString &password);
    
    // 生成随机会话令牌
    static QByteArray generateSessionToken();
    
    // 生成随机AES密钥 (256-bit)
    static QByteArray generateAESKey();
    
    // AES-256-CBC 加密/解密 (用于TCP消息)
    static QByteArray encryptAES_CBC(const QByteArray &plaintext, const QByteArray &key);
    static QByteArray decryptAES_CBC(const QByteArray &ciphertext, const QByteArray &key);
    
    // AES-256-CTR 加密/解密 (用于UDP音频)
    static QByteArray encryptAES_CTR(const QByteArray &plaintext, const QByteArray &key, quint64 counter);
    static QByteArray decryptAES_CTR(const QByteArray &ciphertext, const QByteArray &key, quint64 counter);
    
    // 验证哈希
    static bool verifyPasswordHash(const QString &password, const QByteArray &hash);

private:
    CryptoUtils() = delete;
};

#endif // CRYPTO_H
