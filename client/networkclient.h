#ifndef NETWORKCLIENT_H
#define NETWORKCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>

class NetworkClient : public QObject
{
    Q_OBJECT
public:
    explicit NetworkClient(QObject *parent = nullptr);
    ~NetworkClient();

    void connectToServer(const QString &host, quint16 port);
    void disconnect();
    bool isConnected() const;

    void login(const QString &username, const QString &udpIp, quint16 udpPort);
    void joinChannel(const QString &channel);
    void leaveChannel();
    void requestChannelList();

signals:
    void connected();
    void disconnected();
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

    QTcpSocket *m_socket;
    QByteArray m_buffer;
};

#endif // NETWORKCLIENT_H
