#ifndef AUDIOCONVERTER_H
#define AUDIOCONVERTER_H

#include <QObject>
#include <QString>
#include <QThread>
#include <memory>
#include <atomic>

class OpusEncoderImpl;
class MetadataHandler;

struct ConversionTask {
    QString inputPath;
    QString outputPath;
    int index;
    int total;
};

class AudioConverter : public QObject
{
    Q_OBJECT
    
public:
    explicit AudioConverter(QObject *parent = nullptr);
    ~AudioConverter();
    
    void convertFile(const ConversionTask &task);
    void stopConversion();
    bool isConverting() const { return m_isConverting; }
    QString getLastError() const { return m_lastError; }
    
    // Encoding settings
    void setBitrate(int bitrate);
    void setComplexity(int complexity);
    void setVbr(bool enabled);
    
signals:
    void conversionStarted(const QString &inputFile);
    void conversionProgress(int percentage);
    void conversionCompleted(const QString &inputFile, const QString &outputFile);
    void conversionFailed(const QString &inputFile, const QString &error);
    void allConversionsCompleted();
    
private:
    std::unique_ptr<OpusEncoderImpl> m_encoder;
    std::unique_ptr<MetadataHandler> m_metadataHandler;
    std::atomic<bool> m_isConverting;
    std::atomic<bool> m_shouldStop;
    
    int m_bitrate = 128000; // 128 kbps default
    int m_complexity = 10;  // Maximum quality
    bool m_vbr = true;      // Variable bitrate
    QString m_lastError;
    
    bool ensureOutputDirectory(const QString &outputPath);
    QString generateOutputPath(const QString &inputPath, const QString &outputBase);
};

#endif // AUDIOCONVERTER_H