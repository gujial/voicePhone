#include "audioengine.h"

#include <QAudioFormat>
#include <QAudioSink>
#include <QAudioSource>
#include <QMediaDevices>
#include <QUdpSocket>
#include <QHostAddress>
#include <QByteArray>
#include <QDebug>

AudioEngine::AudioEngine(QObject *parent)
    : QObject(parent)
{
    m_socket = new QUdpSocket(this);
}

AudioEngine::~AudioEngine()
{
    stop();
}

void AudioEngine::start(const QString &serverIp, quint16 voicePort, quint16 localPort)
{
    stop();

    QAudioFormat format;
    format.setSampleRate(48000);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    m_audioSource = new QAudioSource(format, this);
    m_audioSink = new QAudioSink(format, this);

    m_inputDevice = m_audioSource->start();
    m_outputDevice = m_audioSink->start();

    if (m_inputDevice) {
        connect(m_inputDevice, &QIODevice::readyRead, this, &AudioEngine::handleAudioReady);
    }

    m_serverAddress = QHostAddress(serverIp);
    m_serverPort = voicePort;

    // 绑定本地端口接收音频
    if (m_socket->bind(QHostAddress::AnyIPv4, localPort)) {
        connect(m_socket, &QUdpSocket::readyRead, this, &AudioEngine::handleSocketReadyRead);
        m_isRunning = true;
        qInfo() << "Audio engine started - Server:" << serverIp << ":" << voicePort << "Local:" << localPort;
    } else {
        qWarning() << "Failed to bind UDP socket:" << m_socket->errorString();
    }
}

void AudioEngine::stop()
{
    m_isRunning = false;
    
    if (m_audioSource) {
        m_audioSource->stop();
        delete m_audioSource;
        m_audioSource = nullptr;
        m_inputDevice = nullptr;
    }
    if (m_audioSink) {
        m_audioSink->stop();
        delete m_audioSink;
        m_audioSink = nullptr;
        m_outputDevice = nullptr;
    }
    if (m_socket) {
        m_socket->close();
    }
}

void AudioEngine::handleAudioReady()
{
    if (!m_inputDevice || !m_socket || !m_isRunning) return;
    
    QByteArray data = m_inputDevice->readAll();
    if (!data.isEmpty() && m_serverPort != 0) {
        m_socket->writeDatagram(data, m_serverAddress, m_serverPort);
    }
}

void AudioEngine::handleSocketReadyRead()
{
    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(m_socket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        m_socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        
        if (m_outputDevice && !datagram.isEmpty()) {
            m_outputDevice->write(datagram);
        }
    }
}
