#ifndef OPUSENCODER_H
#define OPUSENCODER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>
#include <vector>
#include <cstdint>
#include <atomic>
#include <samplerate.h>

// Forward declarations for opus types
struct OpusEncoder;

class QIODevice;

class OpusEncoderImpl : public QObject
{
    Q_OBJECT
    
public:
    explicit OpusEncoderImpl(QObject *parent = nullptr);
    ~OpusEncoderImpl();
    
    bool initialize(int sampleRate, int channels, int bitrate);
    bool encodeFlacToOpus(const QString &inputPath, const QString &outputPath);
    void stop();
    
    // Encoder settings
    void setBitrate(int bitrate);
    void setComplexity(int complexity);
    void setVbr(bool enabled);
    void setVbrConstraint(bool constrained);
    
    // Get encoder info
    QString getLastError() const { return m_lastError; }
    int getProgress() const { return m_progress; }
    
signals:
    void progressUpdated(int percentage);
    void encodingError(const QString &error);
    
private:
    std::unique_ptr<OpusEncoder, void(*)(OpusEncoder*)> m_encoder;
    
    int m_sampleRate = 48000;
    int m_channels = 2;
    int m_bitrate = 128000;
    int m_complexity = 10;
    bool m_vbr = true;
    bool m_vbrConstrained = false;
    
    QString m_lastError;
    int m_progress = 0;
    std::atomic<bool> m_shouldStop{false};
    
    // FLAC decoding
    bool decodeFlacData(const QString &inputPath, std::vector<float> &samples, int &sampleRate, int &channels);
    
    // Opus encoding
    bool encodeToOpusFile(const std::vector<float> &samples, int sampleRate, int channels, const QString &outputPath);
    
    // Helper functions
    std::vector<uint8_t> encodeFrame(const float *pcm, int frameSize);
    bool writeOpusHeader(QIODevice *device, int sampleRate, int channels);
    bool writeOpusComment(QIODevice *device);
    uint32_t calculateChecksum(const QByteArray &data);
    
    // Ogg Opus specific
    bool writeOggOpusFile(const std::vector<float> &samples, int sampleRate, int channels, const QString &outputPath);
    void createOpusHeader(unsigned char *header, int &headerSize, int channels, int preskip, int inputSampleRate);
    void createOpusComment(unsigned char *comment, int &commentSize);
    
    // Resampling
    std::vector<float> resampleAudio(const std::vector<float> &input, int inputSampleRate, int outputSampleRate, int channels);
};

#endif // OPUSENCODER_H