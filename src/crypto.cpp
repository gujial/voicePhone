#include "crypto.h"
#include <QCryptographicHash>
#include <QRandomGenerator>
#include <QDebug>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/err.h>

QByteArray CryptoUtils::hashPassword(const QString &password)
{
    QByteArray data = password.toUtf8();
    return QCryptographicHash::hash(data, QCryptographicHash::Sha256);
}

QByteArray CryptoUtils::generateSessionToken()
{
    QByteArray token(32, Qt::Uninitialized);
    if (RAND_bytes(reinterpret_cast<unsigned char*>(token.data()), 32) != 1) {
        qWarning() << "Failed to generate random session token";
        // 回退到 Qt 随机数生成器
        for (int i = 0; i < 32; ++i) {
            token[i] = QRandomGenerator::global()->bounded(256);
        }
    }
    return token;
}

QByteArray CryptoUtils::generateAESKey()
{
    QByteArray key(32, Qt::Uninitialized); // 256 bits
    if (RAND_bytes(reinterpret_cast<unsigned char*>(key.data()), 32) != 1) {
        qWarning() << "Failed to generate random AES key";
        // 回退到 Qt 随机数生成器
        for (int i = 0; i < 32; ++i) {
            key[i] = QRandomGenerator::global()->bounded(256);
        }
    }
    return key;
}

QByteArray CryptoUtils::encryptAES_CBC(const QByteArray &plaintext, const QByteArray &key)
{
    if (key.size() != 32) {
        qWarning() << "Invalid key size for AES-256";
        return QByteArray();
    }

    // 生成随机IV
    unsigned char iv[16];
    if (RAND_bytes(iv, 16) != 1) {
        qWarning() << "Failed to generate IV";
        return QByteArray();
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        qWarning() << "Failed to create cipher context";
        return QByteArray();
    }

    QByteArray result;
    int len = 0;
    int ciphertext_len = 0;

    // 初始化加密
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, 
                          reinterpret_cast<const unsigned char*>(key.data()), iv) != 1) {
        qWarning() << "Failed to initialize encryption";
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    // 分配输出缓冲区 (明文长度 + 块大小)
    QByteArray ciphertext(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()), Qt::Uninitialized);

    // 加密数据
    if (EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()), &len,
                         reinterpret_cast<const unsigned char*>(plaintext.data()), 
                         plaintext.size()) != 1) {
        qWarning() << "Encryption failed";
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
    ciphertext_len = len;

    // 完成加密
    if (EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()) + len, &len) != 1) {
        qWarning() << "Encryption finalization failed";
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
    ciphertext_len += len;
    ciphertext.resize(ciphertext_len);

    EVP_CIPHER_CTX_free(ctx);

    // 将IV添加到密文前面
    result.append(reinterpret_cast<const char*>(iv), 16);
    result.append(ciphertext);

    return result;
}

QByteArray CryptoUtils::decryptAES_CBC(const QByteArray &ciphertext, const QByteArray &key)
{
    if (key.size() != 32) {
        qWarning() << "Invalid key size for AES-256";
        return QByteArray();
    }

    if (ciphertext.size() < 16) {
        qWarning() << "Ciphertext too short";
        return QByteArray();
    }

    // 提取IV
    const unsigned char *iv = reinterpret_cast<const unsigned char*>(ciphertext.data());
    QByteArray encrypted = ciphertext.mid(16);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        qWarning() << "Failed to create cipher context";
        return QByteArray();
    }

    QByteArray plaintext(encrypted.size(), Qt::Uninitialized);
    int len = 0;
    int plaintext_len = 0;

    // 初始化解密
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                          reinterpret_cast<const unsigned char*>(key.data()), iv) != 1) {
        qWarning() << "Failed to initialize decryption";
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    // 解密数据
    if (EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(plaintext.data()), &len,
                         reinterpret_cast<const unsigned char*>(encrypted.data()),
                         encrypted.size()) != 1) {
        qWarning() << "Decryption failed";
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
    plaintext_len = len;

    // 完成解密
    if (EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plaintext.data()) + len, &len) != 1) {
        qWarning() << "Decryption finalization failed";
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }
    plaintext_len += len;
    plaintext.resize(plaintext_len);

    EVP_CIPHER_CTX_free(ctx);

    return plaintext;
}

QByteArray CryptoUtils::encryptAES_CTR(const QByteArray &plaintext, const QByteArray &key, quint64 counter)
{
    if (key.size() != 32) {
        qWarning() << "Invalid key size for AES-256";
        return QByteArray();
    }

    // 构造CTR模式的IV (nonce + counter)
    unsigned char iv[16] = {0};
    // 使用counter作为nonce
    for (int i = 0; i < 8; ++i) {
        iv[i] = (counter >> (56 - i * 8)) & 0xFF;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        qWarning() << "Failed to create cipher context";
        return QByteArray();
    }

    QByteArray ciphertext(plaintext.size(), Qt::Uninitialized);
    int len = 0;

    // CTR模式不需要padding
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_ctr(), nullptr,
                          reinterpret_cast<const unsigned char*>(key.data()), iv) != 1) {
        qWarning() << "Failed to initialize CTR encryption";
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    if (EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()), &len,
                         reinterpret_cast<const unsigned char*>(plaintext.data()),
                         plaintext.size()) != 1) {
        qWarning() << "CTR encryption failed";
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    EVP_CIPHER_CTX_free(ctx);

    return ciphertext;
}

QByteArray CryptoUtils::decryptAES_CTR(const QByteArray &ciphertext, const QByteArray &key, quint64 counter)
{
    // CTR模式加密和解密是相同的操作
    return encryptAES_CTR(ciphertext, key, counter);
}

bool CryptoUtils::verifyPasswordHash(const QString &password, const QByteArray &hash)
{
    return hashPassword(password) == hash;
}
