#include "opuscodec.h"
#include <QDebug>

OpusCodec::OpusCodec()
{
}

OpusCodec::~OpusCodec()
{
    cleanup();
}

bool OpusCodec::initialize(int sampleRate, int channels, int bitrate)
{
    cleanup();
    
    m_sampleRate = sampleRate;
    m_channels = channels;
    
    // 创建编码器
    int error;
    m_encoder = opus_encoder_create(sampleRate, channels, OPUS_APPLICATION_VOIP, &error);
    if (error != OPUS_OK) {
        m_lastError = QString("Failed to create Opus encoder: %1").arg(opus_strerror(error));
        qWarning() << m_lastError;
        return false;
    }
    
    // 设置比特率
    opus_encoder_ctl(m_encoder, OPUS_SET_BITRATE(bitrate));
    
    // 创建解码器
    m_decoder = opus_decoder_create(sampleRate, channels, &error);
    if (error != OPUS_OK) {
        m_lastError = QString("Failed to create Opus decoder: %1").arg(opus_strerror(error));
        qWarning() << m_lastError;
        opus_encoder_destroy(m_encoder);
        m_encoder = nullptr;
        return false;
    }
    
    m_initialized = true;
    qInfo() << "Opus codec initialized - Sample rate:" << sampleRate 
            << "Channels:" << channels << "Bitrate:" << bitrate;
    return true;
}

QByteArray OpusCodec::encode(const QByteArray &pcmData, int frameSize)
{
    if (!m_initialized || !m_encoder) {
        m_lastError = "Encoder not initialized";
        return QByteArray();
    }
    
    // 检查输入数据大小是否匹配帧大小
    int expectedBytes = frameSize * m_channels * sizeof(opus_int16);
    if (pcmData.size() < expectedBytes) {
        m_lastError = QString("Insufficient PCM data: expected %1 bytes, got %2")
                          .arg(expectedBytes).arg(pcmData.size());
        return QByteArray();
    }
    
    // 准备输出缓冲区 (最大4000字节对于大多数情况足够)
    QByteArray encoded(4000, 0);
    
    const opus_int16 *pcm = reinterpret_cast<const opus_int16*>(pcmData.constData());
    unsigned char *output = reinterpret_cast<unsigned char*>(encoded.data());
    
    int encodedBytes = opus_encode(m_encoder, pcm, frameSize, output, encoded.size());
    
    if (encodedBytes < 0) {
        m_lastError = QString("Encoding failed: %1").arg(opus_strerror(encodedBytes));
        qWarning() << m_lastError;
        return QByteArray();
    }
    
    encoded.resize(encodedBytes);
    return encoded;
}

QByteArray OpusCodec::decode(const QByteArray &opusData, int frameSize)
{
    if (!m_initialized || !m_decoder) {
        m_lastError = "Decoder not initialized";
        return QByteArray();
    }
    
    if (opusData.isEmpty()) {
        m_lastError = "Empty Opus data";
        return QByteArray();
    }
    
    // 准备输出缓冲区
    int maxSamples = frameSize * m_channels;
    QByteArray decoded(maxSamples * sizeof(opus_int16), 0);
    
    const unsigned char *input = reinterpret_cast<const unsigned char*>(opusData.constData());
    opus_int16 *output = reinterpret_cast<opus_int16*>(decoded.data());
    
    int decodedSamples = opus_decode(m_decoder, input, opusData.size(), output, frameSize, 0);
    
    if (decodedSamples < 0) {
        m_lastError = QString("Decoding failed: %1").arg(opus_strerror(decodedSamples));
        qWarning() << m_lastError;
        return QByteArray();
    }
    
    // 调整大小为实际解码的字节数
    decoded.resize(decodedSamples * m_channels * sizeof(opus_int16));
    return decoded;
}

int OpusCodec::getFrameSize(double durationMs, int sampleRate)
{
    return static_cast<int>(sampleRate * durationMs / 1000.0);
}

void OpusCodec::cleanup()
{
    if (m_encoder) {
        opus_encoder_destroy(m_encoder);
        m_encoder = nullptr;
    }
    if (m_decoder) {
        opus_decoder_destroy(m_decoder);
        m_decoder = nullptr;
    }
    m_initialized = false;
}
