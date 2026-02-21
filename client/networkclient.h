#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QJsonObject>
#include <QObject>
#include <QTcpSocket>
#include <qobject.h>

class NetworkClient : public QObject {
  Q_OBJECT
public:
  explicit NetworkClient(QObject *parent = nullptr);
  ~NetworkClient();

  void connectToServer(const QString &host, quint16 port);
  void disconnect();
  bool isConnected() const;
  bool isAuthenticated() const { return m_isAuthenticated; }
  QString getServerIP() const { return m_socket->peerAddress().toString(); }
  QString getUsername() const { return m_username; }
  int getVoicePort() const { return m_voicePort; }
  int getUdpPort() const { return m_udpPort; }

  void registerUser(const QString &username, const QString &password);
  void login(const QString &username, const QString &password,
             const QString &udpIp, quint16 udpPort);
  void joinChannel(const QString &channel);
  void leaveChannel();
  void requestChannelList();

  // 获取会话密钥（用于音频加密）
  QByteArray getSessionKey() const { return m_sessionKey; }

  // 获取频道密钥（用于UDP音频加密）
  QByteArray getChannelKey() const { return m_channelKey; }

signals:
  void connected();
  void disconnected();
  void registrationSuccess();
  void loginSuccess(quint16 voicePort);
  void channelListReceived(const QJsonObject &data);
  void userListReceived(const QJsonObject &data);
  void userJoined(const QString &username);
  void userLeft(const QString &username);
  void joinedChannel(const QString &channel);
  void leftChannel();
  void errorOccurred(const QString &error);

private slots:
  void onConnected();
  void onDisconnected();
  void onReadyRead();
  void onError(QAbstractSocket::SocketError socketError);

private:
  void handleMessage(const QJsonObject &obj);
  void sendMessage(const QJsonObject &obj);
  void sendEncryptedMessage(const QJsonObject &obj);

  QTcpSocket *m_socket;
  QByteArray m_buffer;
  QString m_sessionId;
  QByteArray m_sessionKey;
  QByteArray m_channelKey;
  bool m_isAuthenticated;
  QString m_username;
  int m_voicePort;
  int m_udpPort;
};

#endif // NETWORKCLIENT_H
