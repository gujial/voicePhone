#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QUdpSocket>
#include <QMap>
#include <QSet>

class QTcpSocket;

struct ClientInfo {
    QTcpSocket *controlSocket = nullptr;
    QString username;
    QString currentChannel;
    QHostAddress udpAddress;
    quint16 udpPort = 0;
    bool isConnected = false;
};

class VoiceServer : public QObject
{
    Q_OBJECT
public:
    explicit VoiceServer(QObject *parent = nullptr);
    ~VoiceServer();

    bool startServer(quint16 controlPort, quint16 voicePort);
    void stopServer();

private slots:
    void onNewConnection();
    void onClientDisconnected();
    void onControlDataReceived();
    void onVoiceDataReceived();

private:
    void handleControlMessage(QTcpSocket *socket, const QByteArray &data);
    void sendToClient(QTcpSocket *socket, const QString &message);
    void broadcastToChannel(const QString &channel, const QByteArray &message);
    void broadcastToChannel(const QString &channel, const QByteArray &message, QTcpSocket *excludeSocket);
    void broadcastVoiceToChannel(const QString &channel, const QByteArray &audioData, QTcpSocket *sender);
    void sendChannelList(QTcpSocket *socket);
    void sendUserList(QTcpSocket *socket, const QString &channel);
    
    QTcpServer *m_controlServer;
    QUdpSocket *m_voiceSocket;
    QMap<QTcpSocket*, ClientInfo> m_clients;
    QMap<QString, QSet<QTcpSocket*>> m_channels; // channel -> set of clients
    quint16 m_voicePort;
};

#endif // SERVER_H
