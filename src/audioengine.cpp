#include "audioengine.h"
#include "opuscodec.h"
#include "crypto.h"

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
    , m_audioCounter(nullptr)
{
    m_socket = new QUdpSocket(this);
    m_codec = new OpusCodec();
    
    // 初始化Opus编解码器（48kHz, 单声道, 24kbps）
    if (!m_codec->initialize(SAMPLE_RATE, CHANNELS, 24000)) {
        qWarning() << "Failed to initialize Opus codec:" << m_codec->lastError();
    }
}

AudioEngine::~AudioEngine()
{
    stop();
    delete m_codec;
}

void AudioEngine::setEncryptionKey(const QByteArray &key)
{
    m_encryptionKey = key;
    if (!key.isEmpty()) {
        qInfo() << "Encryption key set for audio engine";
    }
}

void AudioEngine::setAudioCounter(quint64 *counter)
{
    m_audioCounter = counter;
}

void AudioEngine::start(const QString &serverIp, quint16 voicePort, quint16 localPort)
{
    stop();

    QAudioFormat format;
    format.setSampleRate(SAMPLE_RATE);
    format.setChannelCount(CHANNELS);
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
    
    // 清空缓冲区
    m_captureBuffer.clear();

    // 绑定本地端口接收音频
    if (m_socket->bind(QHostAddress::AnyIPv4, localPort)) {
        connect(m_socket, &QUdpSocket::readyRead, this, &AudioEngine::handleSocketReadyRead);
        m_isRunning = true;
        qInfo() << "Audio engine started with Opus codec - Server:" << serverIp << ":" << voicePort << "Local:" << localPort;
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
    
    // 清空缓冲区
    m_captureBuffer.clear();
}

void AudioEngine::handleAudioReady()
{
    if (!m_inputDevice || !m_socket || !m_isRunning || !m_codec || !m_codec->isInitialized()) return;
    
    // 读取所有可用数据并添加到缓冲区
    QByteArray data = m_inputDevice->readAll();
    if (!data.isEmpty()) {
        m_captureBuffer.append(data);
    }
    
    // 当缓冲区有足够数据时，编码并发送
    while (m_captureBuffer.size() >= BYTES_PER_FRAME) {
        // 提取一帧数据
        QByteArray frame = m_captureBuffer.left(BYTES_PER_FRAME);
        m_captureBuffer.remove(0, BYTES_PER_FRAME);
        
        // 使用Opus编码
        QByteArray encoded = m_codec->encode(frame, FRAME_SIZE);
        
        if (!encoded.isEmpty() && m_serverPort != 0) {
            // 使用频道密钥加密音频数据（端到端加密）
            if (!m_encryptionKey.isEmpty() && m_audioCounter) {
                QByteArray encrypted = CryptoUtils::encryptAES_CTR(encoded, m_encryptionKey, *m_audioCounter);
                if (!encrypted.isEmpty()) {
                    // 将计数器添加到数据包前面（8字节）
                    QByteArray packet;
                    packet.resize(8);
                    for (int i = 0; i < 8; ++i) {
                        packet[i] = (*m_audioCounter >> (56 - i * 8)) & 0xFF;
                    }
                    packet.append(encrypted);
                    m_socket->writeDatagram(packet, m_serverAddress, m_serverPort);
                    (*m_audioCounter)++; // 递增计数器
                }
            } else {
                // 没有加密密钥，发送明文
                m_socket->writeDatagram(encoded, m_serverAddress, m_serverPort);
            }
        }
    }
}

void AudioEngine::handleSocketReadyRead()
{
    if (!m_codec || !m_codec->isInitialized()) return;
    
    while (m_socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(m_socket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        m_socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        
        if (!datagram.isEmpty()) {
            // 使用频道密钥解密音频数据（端到端加密）
            QByteArray toDecode = datagram;
            if (!m_encryptionKey.isEmpty() && datagram.size() > 8) {
                // 从数据包前8字节提取计数器
                quint64 counter = 0;
                for (int i = 0; i < 8; ++i) {
                    counter = (counter << 8) | (static_cast<unsigned char>(datagram[i]));
                }
                
                // 解密剩余数据
                QByteArray encrypted = datagram.mid(8);
                toDecode = CryptoUtils::decryptAES_CTR(encrypted, m_encryptionKey, counter);
            }
            
            // 使用Opus解码
            QByteArray decoded = m_codec->decode(toDecode, FRAME_SIZE);
            
            if (!decoded.isEmpty() && m_outputDevice) {
                // 播放解码后的PCM数据
                m_outputDevice->write(decoded);
            }
        }
    }
}
