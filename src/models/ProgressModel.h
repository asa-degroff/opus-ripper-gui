#ifndef PROGRESSMODEL_H
#define PROGRESSMODEL_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QVariantList>
#include <QList>

class ConversionModel;

class ProgressModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int totalFiles READ totalFiles NOTIFY totalFilesChanged)
    Q_PROPERTY(int filesCompleted READ filesCompleted NOTIFY filesCompletedChanged)
    Q_PROPERTY(int filesFailed READ filesFailed NOTIFY filesFailedChanged)
    Q_PROPERTY(double overallProgress READ overallProgress NOTIFY overallProgressChanged)
    Q_PROPERTY(QString currentFile READ currentFile NOTIFY currentFileChanged)
    Q_PROPERTY(double currentFileProgress READ currentFileProgress NOTIFY currentFileProgressChanged)
    Q_PROPERTY(bool isConverting READ isConverting NOTIFY isConvertingChanged)
    Q_PROPERTY(QString timeElapsed READ timeElapsed NOTIFY timeElapsedChanged)
    Q_PROPERTY(QString timeRemaining READ timeRemaining NOTIFY timeRemainingChanged)
    Q_PROPERTY(QVariantList fileList READ fileList NOTIFY fileListChanged)
    
public:
    explicit ProgressModel(QObject *parent = nullptr);
    ~ProgressModel();
    
    void setConversionModel(ConversionModel *model);
    
    // Properties
    int totalFiles() const { return m_totalFiles; }
    int filesCompleted() const { return m_filesCompleted; }
    int filesFailed() const { return m_filesFailed; }
    double overallProgress() const { return m_overallProgress; }
    QString currentFile() const { return m_currentFile; }
    double currentFileProgress() const { return m_currentFileProgress; }
    bool isConverting() const { return m_isConverting; }
    QString timeElapsed() const;
    QString timeRemaining() const;
    QVariantList fileList() const;
    
    // Control methods
    void startConversion();
    void stopConversion();
    void reset();
    
public slots:
    void updateFileProgress(const QString &filePath, int progress);
    void updateFileStatus(const QString &filePath, const QString &status);
    void setCurrentFile(const QString &filePath);
    
signals:
    void totalFilesChanged();
    void filesCompletedChanged();
    void filesFailedChanged();
    void overallProgressChanged();
    void currentFileChanged();
    void currentFileProgressChanged();
    void isConvertingChanged();
    void timeElapsedChanged();
    void timeRemainingChanged();
    void fileListChanged();
    
private slots:
    void updateTimes();
    void onModelDataChanged();
    
private:
    ConversionModel *m_conversionModel = nullptr;
    
    int m_totalFiles = 0;
    int m_filesCompleted = 0;
    int m_filesFailed = 0;
    double m_overallProgress = 0.0;
    QString m_currentFile;
    double m_currentFileProgress = 0.0;
    bool m_isConverting = false;
    
    QDateTime m_startTime;
    QTimer *m_updateTimer;
    
    // For time estimation
    QList<qint64> m_conversionTimes;
    qint64 m_totalBytesProcessed = 0;
    qint64 m_totalBytesToProcess = 0;
    
    void calculateProgress();
    void updateFileList();
    QString formatTime(int seconds) const;
};

#endif // PROGRESSMODEL_H