#include "AudioConverter.h"
#include "OpusEncoder.h"
#include "MetadataHandler.h"
#include <QDir>
#include <QFileInfo>

AudioConverter::AudioConverter(QObject *parent)
    : QObject(parent)
    , m_encoder(std::make_unique<OpusEncoderImpl>(this))
    , m_metadataHandler(std::make_unique<MetadataHandler>(this))
    , m_isConverting(false)
    , m_shouldStop(false)
{
}

AudioConverter::~AudioConverter() = default;

void AudioConverter::convertFile(const ConversionTask &task)
{
    m_isConverting = true;
    m_shouldStop = false;
    
    emit conversionStarted(task.inputPath);
    
    // Ensure output directory exists
    if (!ensureOutputDirectory(task.outputPath)) {
        m_lastError = "Failed to create output directory";
        emit conversionFailed(task.inputPath, m_lastError);
        m_isConverting = false;
        return;
    }
    
    // Connect progress signals
    connect(m_encoder.get(), &OpusEncoderImpl::progressUpdated,
            this, &AudioConverter::conversionProgress);
    
    // Perform conversion
    bool success = m_encoder->encodeFlacToOpus(task.inputPath, task.outputPath);
    
    if (success) {
        // Copy metadata
        if (!m_metadataHandler->copyMetadata(task.inputPath, task.outputPath)) {
            qDebug() << "Warning: Failed to copy metadata for" << task.inputPath;
        }
        
        m_lastError.clear();
        emit conversionCompleted(task.inputPath, task.outputPath);
    } else {
        m_lastError = m_encoder->getLastError();
        emit conversionFailed(task.inputPath, m_lastError);
    }
    
    // Disconnect progress signals
    disconnect(m_encoder.get(), &OpusEncoderImpl::progressUpdated,
               this, &AudioConverter::conversionProgress);
    
    m_isConverting = false;
    
    // Check if this was the last file
    if (task.index == task.total - 1) {
        emit allConversionsCompleted();
    }
}

void AudioConverter::stopConversion()
{
    m_shouldStop = true;
    if (m_encoder) {
        m_encoder->stop();
    }
}

void AudioConverter::setBitrate(int bitrate)
{
    m_bitrate = bitrate;
    m_encoder->setBitrate(bitrate);
}

void AudioConverter::setComplexity(int complexity)
{
    m_complexity = complexity;
    m_encoder->setComplexity(complexity);
}

void AudioConverter::setVbr(bool enabled)
{
    m_vbr = enabled;
    m_encoder->setVbr(enabled);
}

bool AudioConverter::ensureOutputDirectory(const QString &outputPath)
{
    QFileInfo info(outputPath);
    QDir dir = info.dir();
    return dir.mkpath(".");
}

QString AudioConverter::generateOutputPath(const QString &inputPath, const QString &outputBase)
{
    QFileInfo info(inputPath);
    QString relativePath = info.path();
    QString baseName = info.completeBaseName();
    return QDir(outputBase).filePath(relativePath + "/" + baseName + ".opus");
}