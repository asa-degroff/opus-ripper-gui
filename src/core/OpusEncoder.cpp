#include "OpusEncoder.h"
#include <opus/opus.h>
#include <FLAC++/decoder.h>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <atomic>
#include <cstring>
#include <algorithm>

OpusEncoderImpl::OpusEncoderImpl(QObject *parent)
    : QObject(parent)
    , m_encoder(nullptr, opus_encoder_destroy)
{
}

OpusEncoderImpl::~OpusEncoderImpl() = default;

bool OpusEncoderImpl::initialize(int sampleRate, int channels, int bitrate)
{
    m_sampleRate = sampleRate;
    m_channels = channels;
    m_bitrate = bitrate;
    
    int error = 0;
    OpusEncoder *encoder = opus_encoder_create(sampleRate, channels, OPUS_APPLICATION_AUDIO, &error);
    
    if (error != OPUS_OK) {
        m_lastError = QString("Failed to create Opus encoder: %1").arg(opus_strerror(error));
        return false;
    }
    
    m_encoder.reset(encoder);
    
    // Set encoder parameters
    opus_encoder_ctl(m_encoder.get(), OPUS_SET_BITRATE(m_bitrate));
    opus_encoder_ctl(m_encoder.get(), OPUS_SET_COMPLEXITY(m_complexity));
    opus_encoder_ctl(m_encoder.get(), OPUS_SET_VBR(m_vbr ? 1 : 0));
    
    return true;
}

bool OpusEncoderImpl::encodeFlacToOpus(const QString &inputPath, const QString &outputPath)
{
    m_shouldStop = false;
    m_progress = 0;
    
    // Step 1: Decode FLAC data
    std::vector<float> samples;
    int sampleRate = 0;
    int channels = 0;
    
    emit progressUpdated(5);
    
    if (!decodeFlacData(inputPath, samples, sampleRate, channels)) {
        emit encodingError(m_lastError);
        return false;
    }
    
    emit progressUpdated(50);
    
    // Step 2: Initialize encoder with detected parameters
    if (!initialize(sampleRate, channels, m_bitrate)) {
        emit encodingError(m_lastError);
        return false;
    }
    
    // Step 3: Encode to Opus
    if (!encodeToOpusFile(samples, sampleRate, channels, outputPath)) {
        emit encodingError(m_lastError);
        return false;
    }
    
    emit progressUpdated(100);
    return true;
}

void OpusEncoderImpl::stop()
{
    m_shouldStop = true;
}

void OpusEncoderImpl::setBitrate(int bitrate)
{
    m_bitrate = bitrate;
    if (m_encoder) {
        opus_encoder_ctl(m_encoder.get(), OPUS_SET_BITRATE(bitrate));
    }
}

void OpusEncoderImpl::setComplexity(int complexity)
{
    m_complexity = complexity;
    if (m_encoder) {
        opus_encoder_ctl(m_encoder.get(), OPUS_SET_COMPLEXITY(complexity));
    }
}

void OpusEncoderImpl::setVbr(bool enabled)
{
    m_vbr = enabled;
    if (m_encoder) {
        opus_encoder_ctl(m_encoder.get(), OPUS_SET_VBR(enabled ? 1 : 0));
    }
}

void OpusEncoderImpl::setVbrConstraint(bool constrained)
{
    m_vbrConstrained = constrained;
    if (m_encoder) {
        opus_encoder_ctl(m_encoder.get(), OPUS_SET_VBR_CONSTRAINT(constrained ? 1 : 0));
    }
}

// FLAC decoder implementation
class FlacDecoder : public FLAC::Decoder::File {
public:
    FlacDecoder(std::vector<float> &samples) : m_samples(samples) {}
    
    int getSampleRate() const { return m_sampleRate; }
    int getChannels() const { return m_channels; }
    int getBitsPerSample() const { return m_bitsPerSample; }
    
protected:
    FLAC__StreamDecoderWriteStatus write_callback(const FLAC__Frame *frame, 
                                                  const FLAC__int32 * const buffer[]) override {
        if (!m_initialized) {
            m_sampleRate = frame->header.sample_rate;
            m_channels = frame->header.channels;
            m_bitsPerSample = frame->header.bits_per_sample;
            m_initialized = true;
        }
        
        size_t samples = frame->header.blocksize;
        
        // Convert to float and interleave channels
        for (size_t i = 0; i < samples; i++) {
            for (int ch = 0; ch < m_channels; ch++) {
                // Normalize to -1.0 to 1.0 range
                float sample = buffer[ch][i] / float(1 << (m_bitsPerSample - 1));
                m_samples.push_back(sample);
            }
        }
        
        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }
    
    void metadata_callback(const FLAC__StreamMetadata *metadata) override {
        if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
            // Reserve space for samples
            size_t totalSamples = metadata->data.stream_info.total_samples;
            m_samples.reserve(totalSamples * metadata->data.stream_info.channels);
        }
    }
    
    void error_callback(FLAC__StreamDecoderErrorStatus status) override {
        qDebug() << "FLAC decoder error:" << FLAC__StreamDecoderErrorStatusString[status];
    }
    
private:
    std::vector<float> &m_samples;
    int m_sampleRate = 0;
    int m_channels = 0;
    int m_bitsPerSample = 0;
    bool m_initialized = false;
};

bool OpusEncoderImpl::decodeFlacData(const QString &inputPath, std::vector<float> &samples, int &sampleRate, int &channels)
{
    QFileInfo fileInfo(inputPath);
    if (!fileInfo.exists()) {
        m_lastError = "Input file does not exist";
        return false;
    }
    
    FlacDecoder decoder(samples);
    
    FLAC__StreamDecoderInitStatus init_status = decoder.init(inputPath.toStdString());
    if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        m_lastError = QString("Failed to initialize FLAC decoder: %1")
            .arg(FLAC__StreamDecoderInitStatusString[init_status]);
        return false;
    }
    
    if (!decoder.process_until_end_of_stream()) {
        m_lastError = "Failed to decode FLAC file";
        return false;
    }
    
    sampleRate = decoder.getSampleRate();
    channels = decoder.getChannels();
    
    if (sampleRate == 0 || channels == 0) {
        m_lastError = "Invalid FLAC file format";
        return false;
    }
    
    return true;
}

bool OpusEncoderImpl::encodeToOpusFile(const std::vector<float> &samples, int sampleRate, int channels, const QString &outputPath)
{
    // Ensure output directory exists
    QFileInfo outputInfo(outputPath);
    QDir outputDir = outputInfo.dir();
    if (!outputDir.mkpath(".")) {
        m_lastError = "Failed to create output directory";
        return false;
    }
    
    QFile outputFile(outputPath);
    if (!outputFile.open(QIODevice::WriteOnly)) {
        m_lastError = "Failed to open output file: " + outputFile.errorString();
        return false;
    }
    
    // Write Ogg Opus header
    if (!writeOpusHeader(&outputFile, sampleRate, channels)) {
        return false;
    }
    
    // Write comment header
    if (!writeOpusComment(&outputFile)) {
        return false;
    }
    
    // Opus requires specific frame sizes
    const int frameSize = 960; // 20ms at 48kHz
    const int maxPacketSize = 4000;
    std::vector<unsigned char> packet(maxPacketSize);
    
    // Process samples in frames
    size_t samplesProcessed = 0;
    int packetCount = 0;
    
    while (samplesProcessed < samples.size() / channels) {
        if (m_shouldStop) {
            outputFile.close();
            outputFile.remove();
            return false;
        }
        
        // Prepare frame
        std::vector<float> frame(frameSize * channels);
        size_t frameSamples = std::min(size_t(frameSize), (samples.size() / channels) - samplesProcessed);
        
        // Copy samples to frame
        for (size_t i = 0; i < frameSamples * channels; i++) {
            frame[i] = samples[samplesProcessed * channels + i];
        }
        
        // Pad with silence if needed
        for (size_t i = frameSamples * channels; i < frame.size(); i++) {
            frame[i] = 0.0f;
        }
        
        // Encode frame
        opus_int32 len = opus_encode_float(m_encoder.get(), frame.data(), frameSize, 
                                          packet.data(), maxPacketSize);
        
        if (len < 0) {
            m_lastError = QString("Opus encoding error: %1").arg(opus_strerror(len));
            outputFile.close();
            outputFile.remove();
            return false;
        }
        
        // Write packet to Ogg stream
        // TODO: Implement proper Ogg encapsulation
        // For now, we'll write a simple format
        outputFile.write(reinterpret_cast<char*>(&len), sizeof(len));
        outputFile.write(reinterpret_cast<char*>(packet.data()), len);
        
        samplesProcessed += frameSamples;
        packetCount++;
        
        // Update progress
        int progress = 50 + (samplesProcessed * 50) / (samples.size() / channels);
        if (progress != m_progress) {
            m_progress = progress;
            emit progressUpdated(progress);
        }
    }
    
    outputFile.close();
    return true;
}

std::vector<uint8_t> OpusEncoderImpl::encodeFrame(const float *pcm, int frameSize)
{
    // TODO: Implement frame encoding
    return std::vector<uint8_t>();
}

bool OpusEncoderImpl::writeOpusHeader(QIODevice *device, int sampleRate, int channels)
{
    // TODO: Implement Opus header writing
    return true;
}

bool OpusEncoderImpl::writeOpusComment(QIODevice *device)
{
    // TODO: Implement Opus comment writing
    return true;
}

uint32_t OpusEncoderImpl::calculateChecksum(const QByteArray &data)
{
    // TODO: Implement CRC32 checksum
    return 0;
}