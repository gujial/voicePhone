#ifndef OPUSCODEC_H
#define OPUSCODEC_H

#include <QByteArray>
#include <QString>
#include <opus/opus.h>

/**
 * @brief Opus音频编解码器
 * 
 * 提供Opus编码和解码功能，用于压缩和解压缩音频数据
 * - 采样率: 48kHz
 * - 声道数: 1 (单声道)
 * - 帧大小: 960 samples (20ms @ 48kHz)
 * - 比特率: 24kbps (可调)
 */
class OpusCodec
{
public:
    OpusCodec();
    ~OpusCodec();

    /**
     * @brief 初始化编解码器
     * @param sampleRate 采样率 (8000, 12000, 16000, 24000, 48000)
     * @param channels 声道数 (1 或 2)
     * @param bitrate 比特率 (bps), 推荐: 24000-64000
     * @return 成功返回true
     */
    bool initialize(int sampleRate = 48000, int channels = 1, int bitrate = 24000);

    /**
     * @brief 编码PCM音频数据
     * @param pcmData 原始PCM数据 (int16格式)
     * @param frameSize 帧大小(采样数), 必须是以下之一: 120, 240, 480, 960, 1920, 2880
     * @return 编码后的Opus数据包，失败返回空
     */
    QByteArray encode(const QByteArray &pcmData, int frameSize = 960);

    /**
     * @brief 解码Opus数据包
     * @param opusData Opus编码数据
     * @param frameSize 期望输出的帧大小(采样数)
     * @return 解码后的PCM数据 (int16格式)，失败返回空
     */
    QByteArray decode(const QByteArray &opusData, int frameSize = 960);

    /**
     * @brief 检查编解码器是否已初始化
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief 获取最后的错误信息
     */
    QString lastError() const { return m_lastError; }

    /**
     * @brief 计算指定时长的帧大小
     * @param durationMs 时长(毫秒): 2.5, 5, 10, 20, 40, 60
     * @param sampleRate 采样率
     * @return 帧大小(采样数)
     */
    static int getFrameSize(double durationMs, int sampleRate);

private:
    OpusEncoder *m_encoder = nullptr;
    OpusDecoder *m_decoder = nullptr;
    bool m_initialized = false;
    int m_sampleRate = 48000;
    int m_channels = 1;
    QString m_lastError;

    void cleanup();
};

#endif // OPUSCODEC_H
