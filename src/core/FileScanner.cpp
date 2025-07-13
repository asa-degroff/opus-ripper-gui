#include "FileScanner.h"
#include <QDir>
#include <QDirIterator>
#include <QThread>
#include <QDebug>

FileScanner::FileScanner(QObject *parent)
    : QObject(parent)
{
}

FileScanner::~FileScanner() = default;

void FileScanner::scanDirectory(const QString &directory)
{
    if (m_isScanning) {
        return;
    }
    
    m_scannedFiles.clear();
    m_totalSize = 0;
    m_shouldStop = false;
    
    QThread *thread = new QThread();
    FileScannerWorker *worker = new FileScannerWorker(this, directory);
    worker->moveToThread(thread);
    
    connect(thread, &QThread::started, worker, &FileScannerWorker::process);
    connect(worker, &FileScannerWorker::finished, thread, &QThread::quit);
    connect(worker, &FileScannerWorker::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    
    m_isScanning = true;
    emit scanStarted();
    thread->start();
}

void FileScanner::stopScanning()
{
    m_shouldStop = true;
}

void FileScanner::scanDirectoryRecursive(const QString &directory, const QString &baseDirectory)
{
    QDirIterator it(directory, QDir::Files, QDirIterator::Subdirectories);
    
    int filesFound = 0;
    int totalFilesChecked = 0;
    while (it.hasNext() && !m_shouldStop) {
        it.next();
        QFileInfo fileInfo = it.fileInfo();
        totalFilesChecked++;
        
        if (isFlacFile(fileInfo)) {
            ScannedFile file;
            file.absolutePath = fileInfo.absoluteFilePath();
            file.relativePath = QDir(baseDirectory).relativeFilePath(file.absolutePath);
            file.size = fileInfo.size();
            file.lastModified = fileInfo.lastModified();
            
            m_scannedFiles.append(file);
            m_totalSize += file.size;
            filesFound++;
            
            emit fileFound(file.absolutePath);
            
            if (filesFound % 10 == 0) {
                emit scanProgress(filesFound, m_totalSize);
            }
        }
    }
    
    m_isScanning = false;
    
    if (!m_shouldStop) {
        emit scanCompleted(m_scannedFiles.size(), m_totalSize);
    }
}

bool FileScanner::isFlacFile(const QFileInfo &fileInfo) const
{
    QString suffix = fileInfo.suffix().toLower();
    
    // Check if the suffix matches (without the dot since suffix() returns without dot)
    for (const QString &ext : m_flacExtensions) {
        if (ext == "." + suffix) {
            return true;
        }
    }
    return false;
}

// FileScannerWorker implementation
FileScannerWorker::FileScannerWorker(FileScanner *scanner, const QString &directory)
    : m_scanner(scanner)
    , m_directory(directory)
{
}

void FileScannerWorker::process()
{
    m_scanner->scanDirectoryRecursive(m_directory, m_directory);
    emit finished();
}