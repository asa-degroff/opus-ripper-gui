#include "ConversionController.h"
#include "core/FileScanner.h"
#include "core/AudioConverter.h"
#include "models/ConversionModel.h"
#include "models/ProgressModel.h"
#include <QDir>
#include <QFileInfo>
#include <QRunnable>
#include <QDebug>

class ConversionRunnable : public QRunnable
{
public:
    ConversionRunnable(AudioConverter *converter, const ConversionItem &item, int index, int total)
        : m_converter(converter)
        , m_item(item)
        , m_index(index)
        , m_total(total)
    {
        setAutoDelete(true);
    }
    
    void run() override
    {
        ConversionTask task;
        task.inputPath = m_item.inputPath;
        task.outputPath = m_item.outputPath;
        task.index = m_index;
        task.total = m_total;
        
        m_converter->convertFile(task);
    }
    
private:
    AudioConverter *m_converter;
    ConversionItem m_item;
    int m_index;
    int m_total;
};

ConversionController::ConversionController(QObject *parent)
    : QObject(parent)
    , m_conversionModel(std::make_unique<ConversionModel>(this))
    , m_progressModel(std::make_unique<ProgressModel>(this))
    , m_fileScanner(std::make_unique<FileScanner>(this))
    , m_threadPool(new QThreadPool(this))
{
    m_progressModel->setConversionModel(m_conversionModel.get());
    m_threadPool->setMaxThreadCount(m_threadCount);
    
    // Connect scanner signals
    connect(m_fileScanner.get(), &FileScanner::scanCompleted,
            this, &ConversionController::onScanCompleted);
    connect(m_fileScanner.get(), &FileScanner::scanStarted,
            [this]() {
                m_isScanning = true;
                emit isScanningChanged();
                emit scanStarted();
            });
    connect(m_fileScanner.get(), &FileScanner::scanError,
            this, &ConversionController::scanError);
}

ConversionController::~ConversionController() = default;

void ConversionController::setInputDirectory(const QString &directory)
{
    if (m_inputDirectory != directory) {
        m_inputDirectory = directory;
        emit inputDirectoryChanged();
    }
}

void ConversionController::setOutputDirectory(const QString &directory)
{
    if (m_outputDirectory != directory) {
        m_outputDirectory = directory;
        emit outputDirectoryChanged();
    }
}

void ConversionController::setBitrate(int bitrate)
{
    if (m_bitrate != bitrate) {
        m_bitrate = bitrate;
        emit bitrateChanged();
    }
}

void ConversionController::setComplexity(int complexity)
{
    if (m_complexity != complexity) {
        m_complexity = complexity;
        emit complexityChanged();
    }
}

void ConversionController::setVbr(bool vbr)
{
    if (m_vbr != vbr) {
        m_vbr = vbr;
        emit vbrChanged();
    }
}

void ConversionController::setThreadCount(int count)
{
    count = qBound(1, count, maxThreadCount());
    if (m_threadCount != count) {
        m_threadCount = count;
        m_threadPool->setMaxThreadCount(count);
        emit threadCountChanged();
    }
}

void ConversionController::setPreserveFolderStructure(bool preserve)
{
    if (m_preserveFolderStructure != preserve) {
        m_preserveFolderStructure = preserve;
        emit preserveFolderStructureChanged();
    }
}

void ConversionController::setOverwriteExisting(bool overwrite)
{
    if (m_overwriteExisting != overwrite) {
        m_overwriteExisting = overwrite;
        emit overwriteExistingChanged();
    }
}

void ConversionController::scanForFiles()
{
    if (m_inputDirectory.isEmpty()) {
        emit scanError("Please select an input directory");
        return;
    }
    
    m_conversionModel->clear();
    m_filesFound = 0;
    emit filesFoundChanged();
    
    m_fileScanner->scanDirectory(m_inputDirectory);
}

void ConversionController::startConversion()
{
    if (m_outputDirectory.isEmpty()) {
        emit conversionError("Please select an output directory");
        return;
    }
    
    if (m_filesFound == 0) {
        emit conversionError("No files to convert");
        return;
    }
    
    m_isConverting = true;
    m_filesCompleted = 0;
    emit isConvertingChanged();
    emit filesCompletedChanged();
    emit conversionStarted();
    
    m_progressModel->startConversion();
    
    // Start conversion tasks
    processNextFile();
}

void ConversionController::stopConversion()
{
    m_isConverting = false;
    m_threadPool->clear();
    m_progressModel->stopConversion();
    
    emit isConvertingChanged();
}

void ConversionController::pauseConversion()
{
    // TODO: Implement pause functionality
}

void ConversionController::resumeConversion()
{
    // TODO: Implement resume functionality
}

void ConversionController::onScanCompleted(int totalFiles, qint64 totalSize)
{
    Q_UNUSED(totalSize)
    
    m_isScanning = false;
    m_filesFound = totalFiles;
    
    // Add scanned files to conversion model
    QList<ConversionItem> items;
    for (const auto &scannedFile : m_fileScanner->getScannedFiles()) {
        ConversionItem item;
        item.inputPath = scannedFile.absolutePath;
        item.relativePath = scannedFile.relativePath;
        item.fileName = QFileInfo(scannedFile.absolutePath).fileName();
        item.fileSize = scannedFile.size;
        item.outputPath = generateOutputPath(item.inputPath, item.relativePath);
        
        items.append(item);
    }
    
    m_conversionModel->addFiles(items);
    
    emit isScanningChanged();
    emit filesFoundChanged();
    emit scanCompleted(totalFiles);
}

void ConversionController::onFileConverted(const QString &inputFile, const QString &outputFile)
{
    Q_UNUSED(outputFile)
    
    m_conversionModel->updateFileStatus(inputFile, "completed");
    m_filesCompleted++;
    emit filesCompletedChanged();
    
    if (m_filesCompleted >= m_filesFound) {
        onAllConversionsCompleted();
    } else {
        processNextFile();
    }
}

void ConversionController::onConversionFailed(const QString &inputFile, const QString &error)
{
    m_conversionModel->updateFileStatus(inputFile, "failed", 0, error);
    m_filesCompleted++;
    emit filesCompletedChanged();
    
    if (m_filesCompleted >= m_filesFound) {
        onAllConversionsCompleted();
    } else {
        processNextFile();
    }
}

void ConversionController::onAllConversionsCompleted()
{
    m_isConverting = false;
    m_progressModel->stopConversion();
    
    emit isConvertingChanged();
    emit conversionCompleted();
}

void ConversionController::processNextFile()
{
    // Queue all pending files for conversion
    for (int i = 0; i < m_conversionModel->totalFiles(); ++i) {
        ConversionItem item = m_conversionModel->getItem(i);
        if (item.status == "pending") {
            // Update status to converting
            m_conversionModel->updateFileStatus(item.inputPath, "converting");
            m_progressModel->setCurrentFile(item.inputPath);
            
            AudioConverter *converter = new AudioConverter();
            converter->setBitrate(m_bitrate);
            converter->setComplexity(m_complexity);
            converter->setVbr(m_vbr);
            
            // Connect signals
            connect(converter, &AudioConverter::conversionStarted,
                    [this](const QString &file) {
                        m_progressModel->setCurrentFile(file);
                    });
            
            connect(converter, &AudioConverter::conversionProgress,
                    [this, item](int progress) {
                        m_conversionModel->updateFileProgress(item.inputPath, progress);
                        m_progressModel->updateFileProgress(item.inputPath, progress);
                    });
            
            connect(converter, &AudioConverter::conversionCompleted,
                    this, &ConversionController::onFileConverted);
            
            connect(converter, &AudioConverter::conversionFailed,
                    this, &ConversionController::onConversionFailed);
            
            ConversionRunnable *task = new ConversionRunnable(converter, item, i, m_filesFound);
            m_threadPool->start(task);
        }
    }
}

QString ConversionController::generateOutputPath(const QString &inputPath, const QString &relativePath)
{
    QFileInfo inputInfo(inputPath);
    QString outputFileName = inputInfo.completeBaseName() + ".opus";
    
    if (m_preserveFolderStructure) {
        QFileInfo relativeInfo(relativePath);
        QString relativeDirPath = relativeInfo.path();
        if (relativeDirPath != ".") {
            QDir outputDir(m_outputDirectory);
            return outputDir.filePath(relativeDirPath + "/" + outputFileName);
        }
    }
    
    return QDir(m_outputDirectory).filePath(outputFileName);
}

bool ConversionController::shouldSkipFile(const QString &outputPath)
{
    if (m_overwriteExisting) {
        return false;
    }
    
    return QFile::exists(outputPath);
}