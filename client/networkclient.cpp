#include "networkclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

NetworkClient::NetworkClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected, this, &NetworkClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &NetworkClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &NetworkClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &NetworkClient::onError);
}

NetworkClient::~NetworkClient()
{
    disconnect();
}

void NetworkClient::connectToServer(const QString &host, quint16 port)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        return;
    }
    
    qInfo() << "Connecting to server:" << host << ":" << port;
    m_socket->connectToHost(host, port);
}

void NetworkClient::disconnect()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

bool NetworkClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkClient::login(const QString &username, const QString &udpIp, quint16 udpPort)
{
    QJsonObject msg;
    msg["type"] = "login";
    msg["username"] = username;
    msg["udp_ip"] = udpIp;
    msg["udp_port"] = udpPort;
    sendMessage(msg);
}

void NetworkClient::joinChannel(const QString &channel)
{
    QJsonObject msg;
    msg["type"] = "join_channel";
    msg["channel"] = channel;
    sendMessage(msg);
}

void NetworkClient::leaveChannel()
{
    QJsonObject msg;
    msg["type"] = "leave_channel";
    sendMessage(msg);
}

void NetworkClient::requestChannelList()
{
    QJsonObject msg;
    msg["type"] = "get_channels";
    sendMessage(msg);
}

void NetworkClient::onConnected()
{
    qInfo() << "Connected to server";
    emit connected();
}

void NetworkClient::onDisconnected()
{
    qInfo() << "Disconnected from server";
    m_buffer.clear();
    emit disconnected();
}

void NetworkClient::onReadyRead()
{
    m_buffer.append(m_socket->readAll());
    
    // 处理完整的消息（以换行符分隔）
    int pos;
    while ((pos = m_buffer.indexOf('\n')) != -1) {
        QByteArray message = m_buffer.left(pos);
        m_buffer.remove(0, pos + 1);
        
        QJsonDocument doc = QJsonDocument::fromJson(message);
        if (doc.isObject()) {
            handleMessage(doc.object());
        }
    }
}

void NetworkClient::onError(QAbstractSocket::SocketError socketError)
{
    Q_UNUSED(socketError);
    QString error = m_socket->errorString();
    qWarning() << "Socket error:" << error;
    emit errorOccurred(error);
}

void NetworkClient::handleMessage(const QJsonObject &obj)
{
    QString type = obj["type"].toString();
    
    if (type == "login_success") {
        quint16 voicePort = obj["voice_port"].toInt();
        emit loginSuccess(voicePort);
    }
    else if (type == "channel_list") {
        emit channelListReceived(obj);
    }
    else if (type == "user_list") {
        emit userListReceived(obj);
    }
    else if (type == "user_joined") {
        emit userJoined(obj["username"].toString());
    }
    else if (type == "user_left") {
        emit userLeft(obj["username"].toString());
    }
    else if (type == "join_success") {
        emit joinedChannel(obj["channel"].toString());
    }
    else if (type == "leave_success") {
        emit leftChannel();
    }
}

void NetworkClient::sendMessage(const QJsonObject &obj)
{
    if (!isConnected()) {
        qWarning() << "Not connected to server";
        return;
    }
    
    QByteArray data = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    data.append('\n');
    m_socket->write(data);
    m_socket->flush();
}
