#ifndef FILESCANNER_H
#define FILESCANNER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QFileInfo>
#include <QDateTime>
#include <QThread>
#include <atomic>

struct ScannedFile {
    QString absolutePath;
    QString relativePath; // Relative to input directory
    qint64 size;
    QDateTime lastModified;
};

class FileScanner : public QObject
{
    Q_OBJECT
    
public:
    explicit FileScanner(QObject *parent = nullptr);
    ~FileScanner();
    
    void scanDirectory(const QString &directory);
    void stopScanning();
    
    QList<ScannedFile> getScannedFiles() const { return m_scannedFiles; }
    int getFileCount() const { return m_scannedFiles.size(); }
    qint64 getTotalSize() const { return m_totalSize; }
    bool isScanning() const { return m_isScanning; }
    
signals:
    void scanStarted();
    void fileFound(const QString &filePath);
    void scanProgress(int filesFound, qint64 totalSize);
    void scanCompleted(int totalFiles, qint64 totalSize);
    void scanError(const QString &error);
    
private:
    void scanDirectoryRecursive(const QString &directory, const QString &baseDirectory);
    bool isFlacFile(const QFileInfo &fileInfo) const;
    
    friend class FileScannerWorker;
    
    QList<ScannedFile> m_scannedFiles;
    qint64 m_totalSize = 0;
    std::atomic<bool> m_isScanning{false};
    std::atomic<bool> m_shouldStop{false};
    
    // File extensions to scan
    const QStringList m_flacExtensions = {".flac", ".fla"};
};

// Worker class for background scanning
class FileScannerWorker : public QObject
{
    Q_OBJECT
    
public:
    explicit FileScannerWorker(FileScanner *scanner, const QString &directory);
    
public slots:
    void process();
    
signals:
    void finished();
    
private:
    FileScanner *m_scanner;
    QString m_directory;
};

#endif // FILESCANNER_H