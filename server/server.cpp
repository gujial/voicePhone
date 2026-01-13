#include "server.h"
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

VoiceServer::VoiceServer(QObject *parent)
    : QObject(parent)
    , m_controlServer(new QTcpServer(this))
    , m_voiceSocket(new QUdpSocket(this))
    , m_voicePort(0)
{
    connect(m_controlServer, &QTcpServer::newConnection, this, &VoiceServer::onNewConnection);
    connect(m_voiceSocket, &QUdpSocket::readyRead, this, &VoiceServer::onVoiceDataReceived);
}

VoiceServer::~VoiceServer()
{
    stopServer();
}

bool VoiceServer::startServer(quint16 controlPort, quint16 voicePort)
{
    if (!m_controlServer->listen(QHostAddress::Any, controlPort)) {
        qWarning() << "Failed to start control server:" << m_controlServer->errorString();
        return false;
    }

    if (!m_voiceSocket->bind(QHostAddress::Any, voicePort)) {
        qWarning() << "Failed to bind voice socket:" << m_voiceSocket->errorString();
        m_controlServer->close();
        return false;
    }

    m_voicePort = voicePort;
    qInfo() << "Server started - Control:" << controlPort << "Voice:" << voicePort;
    
    // 创建默认频道
    m_channels["General"] = QSet<QTcpSocket*>();
    m_channels["Gaming"] = QSet<QTcpSocket*>();
    
    return true;
}

void VoiceServer::stopServer()
{
    m_controlServer->close();
    m_voiceSocket->close();
    
    for (auto socket : m_clients.keys()) {
        socket->disconnectFromHost();
    }
    m_clients.clear();
    m_channels.clear();
}

void VoiceServer::onNewConnection()
{
    QTcpSocket *socket = m_controlServer->nextPendingConnection();
    if (!socket) return;

    qInfo() << "New client connected:" << socket->peerAddress().toString();
    
    ClientInfo info;
    info.controlSocket = socket;
    info.isConnected = true;
    m_clients[socket] = info;

    connect(socket, &QTcpSocket::disconnected, this, &VoiceServer::onClientDisconnected);
    connect(socket, &QTcpSocket::readyRead, this, &VoiceServer::onControlDataReceived);
    
    // 发送欢迎消息和频道列表
    sendChannelList(socket);
}

void VoiceServer::onClientDisconnected()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !m_clients.contains(socket)) return;

    ClientInfo &info = m_clients[socket];
    qInfo() << "Client disconnected:" << info.username;

    // 从频道中移除
    if (!info.currentChannel.isEmpty()) {
        m_channels[info.currentChannel].remove(socket);
        
        // 通知频道内其他用户
        QJsonObject msg;
        msg["type"] = "user_left";
        msg["username"] = info.username;
        msg["channel"] = info.currentChannel;
        broadcastToChannel(info.currentChannel, 
            QJsonDocument(msg).toJson(QJsonDocument::Compact));
    }

    m_clients.remove(socket);
    socket->deleteLater();
}

void VoiceServer::onControlDataReceived()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket || !m_clients.contains(socket)) return;

    QByteArray data = socket->readAll();
    handleControlMessage(socket, data);
}

void VoiceServer::onVoiceDataReceived()
{
    while (m_voiceSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(m_voiceSocket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        
        m_voiceSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        
        // 查找发送者并转发到同频道其他用户
        for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
            if (it->udpAddress == sender && it->udpPort == senderPort) {
                if (!it->currentChannel.isEmpty()) {
                    broadcastVoiceToChannel(it->currentChannel, datagram, it.key());
                }
                break;
            }
        }
    }
}

void VoiceServer::handleControlMessage(QTcpSocket *socket, const QByteArray &data)
{
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

    QJsonObject obj = doc.object();
    QString type = obj["type"].toString();
    
    ClientInfo &info = m_clients[socket];

    if (type == "login") {
        info.username = obj["username"].toString();
        info.udpAddress = QHostAddress(obj["udp_ip"].toString());
        info.udpPort = obj["udp_port"].toInt();
        
        QJsonObject response;
        response["type"] = "login_success";
        response["voice_port"] = m_voicePort;
        sendToClient(socket, QJsonDocument(response).toJson(QJsonDocument::Compact));
        
        qInfo() << "User logged in:" << info.username;
    }
    else if (type == "join_channel") {
        QString newChannel = obj["channel"].toString();
        
        // 离开旧频道
        if (!info.currentChannel.isEmpty()) {
            m_channels[info.currentChannel].remove(socket);
            
            QJsonObject leaveMsg;
            leaveMsg["type"] = "user_left";
            leaveMsg["username"] = info.username;
            broadcastToChannel(info.currentChannel, 
                QJsonDocument(leaveMsg).toJson(QJsonDocument::Compact));
        }
        
        // 加入新频道
        info.currentChannel = newChannel;
        if (!m_channels.contains(newChannel)) {
            m_channels[newChannel] = QSet<QTcpSocket*>();
        }
        m_channels[newChannel].insert(socket);
        
        // 通知成功加入
        QJsonObject response;
        response["type"] = "join_success";
        response["channel"] = newChannel;
        sendToClient(socket, QJsonDocument(response).toJson(QJsonDocument::Compact));
        
        // 发送频道用户列表
        sendUserList(socket, newChannel);
        
        // 通知频道内其他用户
        QJsonObject joinMsg;
        joinMsg["type"] = "user_joined";
        joinMsg["username"] = info.username;
        broadcastToChannel(newChannel, 
            QJsonDocument(joinMsg).toJson(QJsonDocument::Compact), socket);
        
        qInfo() << info.username << "joined channel:" << newChannel;
    }
    else if (type == "leave_channel") {
        if (!info.currentChannel.isEmpty()) {
            m_channels[info.currentChannel].remove(socket);
            
            QJsonObject msg;
            msg["type"] = "user_left";
            msg["username"] = info.username;
            broadcastToChannel(info.currentChannel, 
                QJsonDocument(msg).toJson(QJsonDocument::Compact));
            
            info.currentChannel.clear();
            
            QJsonObject response;
            response["type"] = "leave_success";
            sendToClient(socket, QJsonDocument(response).toJson(QJsonDocument::Compact));
        }
    }
    else if (type == "get_channels") {
        sendChannelList(socket);
    }
}

void VoiceServer::sendToClient(QTcpSocket *socket, const QString &message)
{
    if (!socket || !socket->isOpen()) return;
    
    QByteArray data = message.toUtf8();
    data.append('\n'); // 消息分隔符
    socket->write(data);
    socket->flush();
}

void VoiceServer::broadcastToChannel(const QString &channel, const QByteArray &message)
{
    broadcastToChannel(channel, message, nullptr);
}

void VoiceServer::broadcastToChannel(const QString &channel, const QByteArray &message, QTcpSocket *excludeSocket)
{
    if (!m_channels.contains(channel)) return;
    
    for (QTcpSocket *client : m_channels[channel]) {
        if (client != excludeSocket) {
            sendToClient(client, QString::fromUtf8(message));
        }
    }
}

void VoiceServer::broadcastVoiceToChannel(const QString &channel, const QByteArray &audioData, QTcpSocket *sender)
{
    if (!m_channels.contains(channel)) return;
    
    for (QTcpSocket *client : m_channels[channel]) {
        if (client != sender && m_clients.contains(client)) {
            const ClientInfo &info = m_clients[client];
            if (info.udpPort > 0) {
                m_voiceSocket->writeDatagram(audioData, info.udpAddress, info.udpPort);
            }
        }
    }
}

void VoiceServer::sendChannelList(QTcpSocket *socket)
{
    QJsonObject response;
    response["type"] = "channel_list";
    
    QJsonArray channels;
    for (const QString &channel : m_channels.keys()) {
        QJsonObject ch;
        ch["name"] = channel;
        ch["user_count"] = m_channels[channel].size();
        channels.append(ch);
    }
    response["channels"] = channels;
    
    sendToClient(socket, QJsonDocument(response).toJson(QJsonDocument::Compact));
}

void VoiceServer::sendUserList(QTcpSocket *socket, const QString &channel)
{
    if (!m_channels.contains(channel)) return;
    
    QJsonObject response;
    response["type"] = "user_list";
    response["channel"] = channel;
    
    QJsonArray users;
    for (QTcpSocket *client : m_channels[channel]) {
        if (m_clients.contains(client)) {
            users.append(m_clients[client].username);
        }
    }
    response["users"] = users;
    
    sendToClient(socket, QJsonDocument(response).toJson(QJsonDocument::Compact));
}
