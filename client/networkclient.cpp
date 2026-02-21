#include "networkclient.h"
#include "../src/crypto.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent), m_socket(new QTcpSocket(this)),
      m_isAuthenticated(false) {
  connect(m_socket, &QTcpSocket::connected, this, &NetworkClient::onConnected);
  connect(m_socket, &QTcpSocket::disconnected, this,
          &NetworkClient::onDisconnected);
  connect(m_socket, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
  connect(m_socket, &QTcpSocket::errorOccurred, this, &NetworkClient::onError);
}

NetworkClient::~NetworkClient() { disconnect(); }

void NetworkClient::connectToServer(const QString &host, quint16 port) {
  if (m_socket->state() == QAbstractSocket::ConnectedState) {
    return;
  }

  qInfo() << "Connecting to server:" << host << ":" << port;
  m_socket->connectToHost(host, port);
}

void NetworkClient::disconnect() {
  if (m_socket->state() == QAbstractSocket::ConnectedState) {
    m_socket->disconnectFromHost();
  }
  m_isAuthenticated = false;
  m_sessionId.clear();
  m_sessionKey.clear();
  m_channelKey.clear();
}

bool NetworkClient::isConnected() const {
  return m_socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkClient::registerUser(const QString &username,
                                 const QString &password) {
  QByteArray passwordHash = CryptoUtils::hashPassword(password);

  QJsonObject msg;
  msg["type"] = "register";
  msg["username"] = username;
  msg["password_hash"] = QString::fromUtf8(passwordHash.toHex());
  sendMessage(msg);
}

void NetworkClient::login(const QString &username, const QString &password,
                          const QString &udpIp, quint16 udpPort) {
  QByteArray passwordHash = CryptoUtils::hashPassword(password);

  QJsonObject msg;
  msg["type"] = "login";
  msg["username"] = username;
  msg["password_hash"] = QString::fromUtf8(passwordHash.toHex());
  msg["udp_ip"] = udpIp;
  msg["udp_port"] = udpPort;
  sendMessage(msg);
  m_username = username;
  m_udpPort = udpPort;
}

void NetworkClient::joinChannel(const QString &channel) {
  QJsonObject msg;
  msg["type"] = "join_channel";
  msg["channel"] = channel;
  sendEncryptedMessage(msg);
}

void NetworkClient::leaveChannel() {
  QJsonObject msg;
  msg["type"] = "leave_channel";
  sendEncryptedMessage(msg);
}

void NetworkClient::requestChannelList() {
  QJsonObject msg;
  msg["type"] = "get_channels";
  sendEncryptedMessage(msg);
}

void NetworkClient::onConnected() {
  qInfo() << "Connected to server";
  emit connected();
}

void NetworkClient::onDisconnected() {
  qInfo() << "Disconnected from server";
  m_buffer.clear();
  m_isAuthenticated = false;
  m_sessionId.clear();
  m_sessionKey.clear();
  m_channelKey.clear();
  m_voicePort = 0;
  m_udpPort = 0;
  emit disconnected();
}

void NetworkClient::onReadyRead() {
  m_buffer.append(m_socket->readAll());

  // 处理完整的消息（以换行符分隔）
  int pos;
  while ((pos = m_buffer.indexOf('\n')) != -1) {
    QByteArray message = m_buffer.left(pos);
    m_buffer.remove(0, pos + 1);

    // 先尝试作为JSON明文解析
    QByteArray decryptedMessage = message;
    QJsonDocument doc = QJsonDocument::fromJson(message);

    // 如果不是有效的JSON且已认证，尝试解密
    if (doc.isNull() && m_isAuthenticated && !m_sessionKey.isEmpty()) {
      // 尝试从Base64解码并解密
      QByteArray decoded = QByteArray::fromBase64(message);
      if (!decoded.isEmpty() && decoded.size() > 16) {
        QByteArray decrypted =
            CryptoUtils::decryptAES_CBC(decoded, m_sessionKey);
        if (!decrypted.isEmpty()) {
          decryptedMessage = decrypted;
          doc = QJsonDocument::fromJson(decryptedMessage);
        }
      }
    }

    if (doc.isObject()) {
      handleMessage(doc.object());
    } else {
      qWarning() << "Received invalid JSON message";
    }
  }
}

void NetworkClient::onError(QAbstractSocket::SocketError socketError) {
  Q_UNUSED(socketError);
  QString error = m_socket->errorString();
  qWarning() << "Socket error:" << error;
  emit errorOccurred(error);
}

void NetworkClient::handleMessage(const QJsonObject &obj) {
  QString type = obj["type"].toString();

  if (type == "register_success") {
    emit registrationSuccess();
  } else if (type == "login_success") {
    m_sessionId = obj["session_id"].toString();
    m_sessionKey = QByteArray::fromHex(obj["session_key"].toString().toUtf8());
    m_isAuthenticated = true;

    quint16 voicePort = obj["voice_port"].toInt();
    m_voicePort = voicePort;
    emit loginSuccess(voicePort);
  } else if (type == "channel_list") {
    emit channelListReceived(obj);
  } else if (type == "user_list") {
    emit userListReceived(obj);
  } else if (type == "user_joined") {
    emit userJoined(obj["username"].toString());
  } else if (type == "user_left") {
    emit userLeft(obj["username"].toString());
  } else if (type == "join_success") {
    // 保存频道加密密钥
    if (obj.contains("channel_key")) {
      m_channelKey =
          QByteArray::fromHex(obj["channel_key"].toString().toUtf8());
      if (!m_channelKey.isEmpty()) {
        qInfo() << "Received channel encryption key";
      }
    }
    emit joinedChannel(obj["channel"].toString());
  } else if (type == "leave_success") {
    m_channelKey.clear();
    emit leftChannel();
  } else if (type == "error") {
    QString errorMsg = obj["message"].toString();
    emit errorOccurred(errorMsg);
  }
}

void NetworkClient::sendMessage(const QJsonObject &obj) {
  if (!isConnected()) {
    qWarning() << "Not connected to server";
    return;
  }

  QByteArray data = QJsonDocument(obj).toJson(QJsonDocument::Compact);
  data.append('\n');
  m_socket->write(data);
  m_socket->flush();
}

void NetworkClient::sendEncryptedMessage(const QJsonObject &obj) {
  if (!isConnected()) {
    qWarning() << "Not connected to server";
    return;
  }

  // 如果未认证或没有会话密钥，发送明文
  if (!m_isAuthenticated || m_sessionKey.isEmpty()) {
    sendMessage(obj);
    return;
  }

  QByteArray plaintext = QJsonDocument(obj).toJson(QJsonDocument::Compact);
  QByteArray encrypted = CryptoUtils::encryptAES_CBC(plaintext, m_sessionKey);

  if (!encrypted.isEmpty()) {
    // 使用Base64编码加密数据以安全传输
    QByteArray encoded = encrypted.toBase64();
    encoded.append('\n');
    m_socket->write(encoded);
    m_socket->flush();
  } else {
    qWarning() << "Failed to encrypt message";
  }
}
