#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QObject>
#include <QHostAddress>
#include <QByteArray>

class QAudioInput;
class QAudioOutput;
class QAudioSink;
class QAudioSource;
class QUdpSocket;
class QIODevice;
class OpusCodec;

class AudioEngine : public QObject
{
    Q_OBJECT
public:
    explicit AudioEngine(QObject *parent = nullptr);
    ~AudioEngine();

    void start(const QString &serverIp, quint16 voicePort, quint16 localPort);
    void stop();
    bool isRunning() const { return m_isRunning; }

private slots:
    void handleAudioReady();
    void handleSocketReadyRead();

private:
    QAudioSource *m_audioSource = nullptr;
    QAudioSink *m_audioSink = nullptr;
    QUdpSocket *m_socket = nullptr;
    QIODevice *m_inputDevice = nullptr;
    QIODevice *m_outputDevice = nullptr;
    QHostAddress m_serverAddress;
    quint16 m_serverPort = 0;
    bool m_isRunning = false;
    
    // Opus编解码器
    OpusCodec *m_codec = nullptr;
    
    // 音频缓冲区
    QByteArray m_captureBuffer;  // 捕获的PCM数据缓冲
    static constexpr int FRAME_SIZE = 960;  // 20ms @ 48kHz
    static constexpr int SAMPLE_RATE = 48000;
    static constexpr int CHANNELS = 1;
    static constexpr int BYTES_PER_FRAME = FRAME_SIZE * CHANNELS * 2; // int16 = 2 bytes
};

#endif // AUDIOENGINE_H
