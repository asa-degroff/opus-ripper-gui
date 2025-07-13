#ifndef CONVERSIONCONTROLLER_H
#define CONVERSIONCONTROLLER_H

#include <QObject>
#include <QString>
#include <QThreadPool>
#include <memory>
#include <atomic>

#include "models/ConversionModel.h"
#include "models/ProgressModel.h"

class FileScanner;
class AudioConverter;
class ConversionRunnable;

class ConversionController : public QObject
{
    Q_OBJECT
    
    // Directory properties
    Q_PROPERTY(QString inputDirectory READ inputDirectory WRITE setInputDirectory NOTIFY inputDirectoryChanged)
    Q_PROPERTY(QString outputDirectory READ outputDirectory WRITE setOutputDirectory NOTIFY outputDirectoryChanged)
    
    // Status properties
    Q_PROPERTY(bool isScanning READ isScanning NOTIFY isScanningChanged)
    Q_PROPERTY(bool isConverting READ isConverting NOTIFY isConvertingChanged)
    Q_PROPERTY(int filesFound READ filesFound NOTIFY filesFoundChanged)
    Q_PROPERTY(int filesCompleted READ filesCompleted NOTIFY filesCompletedChanged)
    
    // Settings properties
    Q_PROPERTY(int bitrate READ bitrate WRITE setBitrate NOTIFY bitrateChanged)
    Q_PROPERTY(int complexity READ complexity WRITE setComplexity NOTIFY complexityChanged)
    Q_PROPERTY(bool vbr READ vbr WRITE setVbr NOTIFY vbrChanged)
    Q_PROPERTY(int threadCount READ threadCount WRITE setThreadCount NOTIFY threadCountChanged)
    Q_PROPERTY(int maxThreadCount READ maxThreadCount CONSTANT)
    Q_PROPERTY(bool preserveFolderStructure READ preserveFolderStructure WRITE setPreserveFolderStructure NOTIFY preserveFolderStructureChanged)
    Q_PROPERTY(bool overwriteExisting READ overwriteExisting WRITE setOverwriteExisting NOTIFY overwriteExistingChanged)
    
    // Models
    Q_PROPERTY(ConversionModel* conversionModel READ conversionModel CONSTANT)
    Q_PROPERTY(ProgressModel* progressModel READ progressModel CONSTANT)
    
public:
    explicit ConversionController(QObject *parent = nullptr);
    ~ConversionController();
    
    // Directory management
    QString inputDirectory() const { return m_inputDirectory; }
    Q_INVOKABLE void setInputDirectory(const QString &directory);
    
    QString outputDirectory() const { return m_outputDirectory; }
    Q_INVOKABLE void setOutputDirectory(const QString &directory);
    
    // Status getters
    bool isScanning() const { return m_isScanning; }
    bool isConverting() const { return m_isConverting; }
    int filesFound() const { return m_filesFound; }
    int filesCompleted() const { return m_filesCompleted; }
    
    // Settings getters/setters
    int bitrate() const { return m_bitrate; }
    void setBitrate(int bitrate);
    
    int complexity() const { return m_complexity; }
    void setComplexity(int complexity);
    
    bool vbr() const { return m_vbr; }
    void setVbr(bool vbr);
    
    int threadCount() const { return m_threadCount; }
    void setThreadCount(int count);
    
    int maxThreadCount() const { return QThread::idealThreadCount(); }
    
    bool preserveFolderStructure() const { return m_preserveFolderStructure; }
    void setPreserveFolderStructure(bool preserve);
    
    bool overwriteExisting() const { return m_overwriteExisting; }
    void setOverwriteExisting(bool overwrite);
    
    // Models
    ConversionModel* conversionModel() const { return m_conversionModel.get(); }
    ProgressModel* progressModel() const { return m_progressModel.get(); }
    
public slots:
    void scanForFiles();
    void startConversion();
    void stopConversion();
    void pauseConversion();
    void resumeConversion();
    void updateFileProgress(const QString &filePath, int progress);
    
signals:
    void inputDirectoryChanged();
    void outputDirectoryChanged();
    void isScanningChanged();
    void isConvertingChanged();
    void filesFoundChanged();
    void filesCompletedChanged();
    void bitrateChanged();
    void complexityChanged();
    void vbrChanged();
    void threadCountChanged();
    void preserveFolderStructureChanged();
    void overwriteExistingChanged();
    
    void scanStarted();
    void scanCompleted(int filesFound);
    void scanError(const QString &error);
    
    void conversionStarted();
    void conversionCompleted();
    void conversionError(const QString &error);
    
public slots:
    void onScanCompleted(int totalFiles, qint64 totalSize);
    void onFileConverted(const QString &inputFile, const QString &outputFile);
    void onConversionFailed(const QString &inputFile, const QString &error);
    
private slots:
    void onAllConversionsCompleted();
    
private:
    // Models
    std::unique_ptr<ConversionModel> m_conversionModel;
    std::unique_ptr<ProgressModel> m_progressModel;
    
    // Core components
    std::unique_ptr<FileScanner> m_fileScanner;
    QThreadPool *m_threadPool;
    
    // Directories
    QString m_inputDirectory;
    QString m_outputDirectory;
    
    // Status
    std::atomic<bool> m_isScanning{false};
    std::atomic<bool> m_isConverting{false};
    std::atomic<int> m_filesFound{0};
    std::atomic<int> m_filesCompleted{0};
    
    // Settings
    int m_bitrate = 128000;
    int m_complexity = 10;
    bool m_vbr = true;
    int m_threadCount = 4;
    bool m_preserveFolderStructure = true;
    bool m_overwriteExisting = false;
    
    // Helper methods
    void processNextFile();
    QString generateOutputPath(const QString &inputPath, const QString &relativePath);
    bool shouldSkipFile(const QString &outputPath);
    
    friend class ConversionRunnable;
};

#endif // CONVERSIONCONTROLLER_H