#include "OpusEncoder.h"
#include <opus/opus.h>
#include <ogg/ogg.h>
#include <FLAC++/decoder.h>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <atomic>
#include <cstring>
#include <algorithm>
#include <random>

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
    
    // Step 2: Resample if necessary - Opus only supports specific sample rates
    int opusSampleRate = 48000; // Target sample rate for Opus
    if (sampleRate != 8000 && sampleRate != 12000 && sampleRate != 16000 && 
        sampleRate != 24000 && sampleRate != 48000) {
        // For now, we'll use 48kHz as the target
        // TODO: Implement proper resampling
        // For this initial version, we'll just use 48kHz and let Opus handle it
    } else {
        opusSampleRate = sampleRate;
    }
    
    // Step 3: Initialize encoder with Opus-compatible sample rate
    if (!initialize(opusSampleRate, channels, m_bitrate)) {
        emit encodingError(m_lastError);
        return false;
    }
    
    // Step 4: Resample if necessary
    std::vector<float> resampledSamples;
    if (sampleRate != opusSampleRate) {
        resampledSamples = resampleAudio(samples, sampleRate, opusSampleRate, channels);
    } else {
        resampledSamples = samples;
    }
    
    // Step 5: Encode to Ogg Opus
    if (!writeOggOpusFile(resampledSamples, opusSampleRate, channels, outputPath)) {
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
    // For 48kHz: 120, 240, 480, 960, 1920, 2880 samples
    // We'll use 960 samples (20ms) for good balance
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

bool OpusEncoderImpl::writeOggOpusFile(const std::vector<float> &samples, int sampleRate, int channels, const QString &outputPath)
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
    
    // Initialize Ogg stream
    ogg_stream_state os;
    ogg_page og;
    ogg_packet op;
    
    // Generate random serial number
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 0x7FFFFFFF);
    int serialno = dis(gen);
    
    if (ogg_stream_init(&os, serialno) != 0) {
        m_lastError = "Failed to initialize Ogg stream";
        return false;
    }
    
    // Pre-skip for Opus (recommended 3840 samples at 48kHz = 80ms)
    const int preskip = 3840;
    int64_t granulepos = 0;
    int64_t packetno = 0;
    
    // Create Opus header packet
    unsigned char header_data[276];
    int header_size = 0;
    createOpusHeader(header_data, header_size, channels, preskip, sampleRate);
    
    op.packet = header_data;
    op.bytes = header_size;
    op.b_o_s = 1;
    op.e_o_s = 0;
    op.granulepos = 0;
    op.packetno = packetno++;
    
    ogg_stream_packetin(&os, &op);
    
    // Write header page
    while (ogg_stream_flush(&os, &og) != 0) {
        outputFile.write(reinterpret_cast<char*>(og.header), og.header_len);
        outputFile.write(reinterpret_cast<char*>(og.body), og.body_len);
    }
    
    // Create comment header packet
    unsigned char comment_data[1024];
    int comment_size = 0;
    createOpusComment(comment_data, comment_size);
    
    op.packet = comment_data;
    op.bytes = comment_size;
    op.b_o_s = 0;
    op.e_o_s = 0;
    op.granulepos = 0;
    op.packetno = packetno++;
    
    ogg_stream_packetin(&os, &op);
    
    // Write comment page
    while (ogg_stream_flush(&os, &og) != 0) {
        outputFile.write(reinterpret_cast<char*>(og.header), og.header_len);
        outputFile.write(reinterpret_cast<char*>(og.body), og.body_len);
    }
    
    // Encode audio data
    const int frameSize = 960; // 20ms at 48kHz
    const int maxPacketSize = 4000;
    std::vector<unsigned char> packet(maxPacketSize);
    
    size_t samplesProcessed = 0;
    int totalFrames = (samples.size() / channels + frameSize - 1) / frameSize;
    
    while (samplesProcessed < samples.size() / channels) {
        if (m_shouldStop) {
            ogg_stream_clear(&os);
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
        if (!m_encoder) {
            m_lastError = "Encoder not initialized";
            ogg_stream_clear(&os);
            outputFile.close();
            outputFile.remove();
            return false;
        }
        
        opus_int32 len = opus_encode_float(m_encoder.get(), frame.data(), frameSize, 
                                          packet.data(), maxPacketSize);
        
        if (len < 0) {
            m_lastError = QString("Opus encoding error: %1").arg(opus_strerror(len));
            ogg_stream_clear(&os);
            outputFile.close();
            outputFile.remove();
            return false;
        }
        
        // Calculate granulepos (number of samples at 48kHz)
        granulepos += frameSize;
        
        // Create Ogg packet
        op.packet = packet.data();
        op.bytes = len;
        op.b_o_s = 0;
        op.e_o_s = (samplesProcessed + frameSamples >= samples.size() / channels) ? 1 : 0;
        op.granulepos = granulepos - preskip;
        op.packetno = packetno++;
        
        ogg_stream_packetin(&os, &op);
        
        // Write pages
        while (ogg_stream_pageout(&os, &og) != 0) {
            outputFile.write(reinterpret_cast<char*>(og.header), og.header_len);
            outputFile.write(reinterpret_cast<char*>(og.body), og.body_len);
        }
        
        samplesProcessed += frameSamples;
        
        // Update progress
        int progress = 50 + (samplesProcessed * 50) / (samples.size() / channels);
        if (progress != m_progress) {
            m_progress = progress;
            emit progressUpdated(progress);
        }
    }
    
    // Flush remaining pages
    while (ogg_stream_flush(&os, &og) != 0) {
        outputFile.write(reinterpret_cast<char*>(og.header), og.header_len);
        outputFile.write(reinterpret_cast<char*>(og.body), og.body_len);
    }
    
    ogg_stream_clear(&os);
    outputFile.close();
    return true;
}

void OpusEncoderImpl::createOpusHeader(unsigned char *header, int &headerSize, int channels, int preskip, int inputSampleRate)
{
    // OpusHead structure
    memcpy(header, "OpusHead", 8);  // Magic signature
    header[8] = 1;  // Version
    header[9] = channels;  // Channel count
    
    // Pre-skip (16-bit LE)
    header[10] = preskip & 0xFF;
    header[11] = (preskip >> 8) & 0xFF;
    
    // Input sample rate (32-bit LE)
    header[12] = inputSampleRate & 0xFF;
    header[13] = (inputSampleRate >> 8) & 0xFF;
    header[14] = (inputSampleRate >> 16) & 0xFF;
    header[15] = (inputSampleRate >> 24) & 0xFF;
    
    // Output gain (16-bit LE) - 0 dB
    header[16] = 0;
    header[17] = 0;
    
    // Channel mapping family
    header[18] = 0;  // RTP mapping family
    
    headerSize = 19;
}

void OpusEncoderImpl::createOpusComment(unsigned char *comment, int &commentSize)
{
    // OpusTags structure
    memcpy(comment, "OpusTags", 8);  // Magic signature
    
    // Vendor string length (32-bit LE)
    const char *vendor = "OpusRipperGUI";
    int vendorLen = strlen(vendor);
    comment[8] = vendorLen & 0xFF;
    comment[9] = (vendorLen >> 8) & 0xFF;
    comment[10] = (vendorLen >> 16) & 0xFF;
    comment[11] = (vendorLen >> 24) & 0xFF;
    
    // Vendor string
    memcpy(comment + 12, vendor, vendorLen);
    
    // User comment count (32-bit LE)
    int pos = 12 + vendorLen;
    comment[pos] = 0;
    comment[pos + 1] = 0;
    comment[pos + 2] = 0;
    comment[pos + 3] = 0;
    
    commentSize = pos + 4;
}

std::vector<float> OpusEncoderImpl::resampleAudio(const std::vector<float> &input, int inputSampleRate, int outputSampleRate, int channels)
{
    // Simple linear interpolation resampler
    // TODO: Implement a proper resampler (e.g., using libsamplerate or speex resampler)
    
    double ratio = static_cast<double>(outputSampleRate) / inputSampleRate;
    size_t inputSamples = input.size() / channels;
    size_t outputSamples = static_cast<size_t>(inputSamples * ratio);
    
    std::vector<float> output(outputSamples * channels);
    
    for (size_t outSample = 0; outSample < outputSamples; outSample++) {
        double inSamplePos = outSample / ratio;
        size_t inSample = static_cast<size_t>(inSamplePos);
        double fraction = inSamplePos - inSample;
        
        if (inSample + 1 < inputSamples) {
            // Linear interpolation between two samples
            for (int ch = 0; ch < channels; ch++) {
                float sample1 = input[inSample * channels + ch];
                float sample2 = input[(inSample + 1) * channels + ch];
                output[outSample * channels + ch] = sample1 + (sample2 - sample1) * fraction;
            }
        } else {
            // Use last sample
            for (int ch = 0; ch < channels; ch++) {
                output[outSample * channels + ch] = input[(inputSamples - 1) * channels + ch];
            }
        }
    }
    
    return output;
}