#include "ProgressModel.h"
#include "ConversionModel.h"
#include <QVariantMap>
#include <QFileInfo>
#include <QtMath>

ProgressModel::ProgressModel(QObject *parent)
    : QObject(parent)
    , m_updateTimer(new QTimer(this))
{
    m_updateTimer->setInterval(1000); // Update every second
    connect(m_updateTimer, &QTimer::timeout, this, &ProgressModel::updateTimes);
}

ProgressModel::~ProgressModel() = default;

void ProgressModel::setConversionModel(ConversionModel *model)
{
    if (m_conversionModel) {
        disconnect(m_conversionModel, nullptr, this, nullptr);
    }
    
    m_conversionModel = model;
    
    if (m_conversionModel) {
        connect(m_conversionModel, &ConversionModel::dataChanged, this, &ProgressModel::onModelDataChanged);
        connect(m_conversionModel, &ConversionModel::totalFilesChanged, this, &ProgressModel::totalFilesChanged);
        connect(m_conversionModel, &ConversionModel::completedFilesChanged, this, &ProgressModel::filesCompletedChanged);
        connect(m_conversionModel, &ConversionModel::failedFilesChanged, this, &ProgressModel::filesFailedChanged);
    }
}

QString ProgressModel::timeElapsed() const
{
    if (!m_isConverting || !m_startTime.isValid()) {
        return QString();
    }
    
    int seconds = m_startTime.secsTo(QDateTime::currentDateTime());
    return formatTime(seconds);
}

QString ProgressModel::timeRemaining() const
{
    if (!m_isConverting || m_overallProgress <= 0) {
        return QString();
    }
    
    int elapsed = m_startTime.secsTo(QDateTime::currentDateTime());
    if (elapsed <= 0 || m_overallProgress <= 0) {
        return QString();
    }
    
    int total = qRound(elapsed / m_overallProgress);
    int remaining = total - elapsed;
    
    return formatTime(remaining);
}

QVariantList ProgressModel::fileList() const
{
    QVariantList list;
    
    if (!m_conversionModel) {
        return list;
    }
    
    for (int i = 0; i < m_conversionModel->rowCount(); ++i) {
        QVariantMap item;
        item["fileName"] = m_conversionModel->data(m_conversionModel->index(i), ConversionModel::FileNameRole);
        item["status"] = m_conversionModel->data(m_conversionModel->index(i), ConversionModel::StatusRole);
        item["progress"] = m_conversionModel->data(m_conversionModel->index(i), ConversionModel::ProgressRole);
        item["error"] = m_conversionModel->data(m_conversionModel->index(i), ConversionModel::ErrorRole);
        list.append(item);
    }
    
    return list;
}

void ProgressModel::startConversion()
{
    m_startTime = QDateTime::currentDateTime();
    m_isConverting = true;
    m_updateTimer->start();
    
    emit isConvertingChanged();
    emit timeElapsedChanged();
}

void ProgressModel::stopConversion()
{
    m_isConverting = false;
    m_updateTimer->stop();
    
    emit isConvertingChanged();
}

void ProgressModel::reset()
{
    m_totalFiles = 0;
    m_filesCompleted = 0;
    m_filesFailed = 0;
    m_overallProgress = 0.0;
    m_currentFile.clear();
    m_currentFileProgress = 0.0;
    m_isConverting = false;
    m_conversionTimes.clear();
    m_totalBytesProcessed = 0;
    m_totalBytesToProcess = 0;
    
    m_updateTimer->stop();
    
    emit totalFilesChanged();
    emit filesCompletedChanged();
    emit filesFailedChanged();
    emit overallProgressChanged();
    emit currentFileChanged();
    emit currentFileProgressChanged();
    emit isConvertingChanged();
    emit fileListChanged();
}

void ProgressModel::updateFileProgress(const QString &filePath, int progress)
{
    m_currentFileProgress = progress / 100.0;
    emit currentFileProgressChanged();
}

void ProgressModel::updateFileStatus(const QString &filePath, const QString &status)
{
    if (status == "completed") {
        m_filesCompleted++;
        emit filesCompletedChanged();
    } else if (status == "failed") {
        m_filesFailed++;
        emit filesFailedChanged();
    }
    
    calculateProgress();
}

void ProgressModel::setCurrentFile(const QString &filePath)
{
    QFileInfo info(filePath);
    m_currentFile = info.fileName();
    emit currentFileChanged();
}

void ProgressModel::updateTimes()
{
    emit timeElapsedChanged();
    emit timeRemainingChanged();
}

void ProgressModel::onModelDataChanged()
{
    if (m_conversionModel) {
        m_totalFiles = m_conversionModel->totalFiles();
        m_filesCompleted = m_conversionModel->completedFiles();
        m_filesFailed = m_conversionModel->failedFiles();
        
        calculateProgress();
        emit fileListChanged();
    }
}

void ProgressModel::calculateProgress()
{
    if (m_totalFiles > 0) {
        m_overallProgress = static_cast<double>(m_filesCompleted + m_filesFailed) / m_totalFiles;
    } else {
        m_overallProgress = 0.0;
    }
    
    emit overallProgressChanged();
}

void ProgressModel::updateFileList()
{
    emit fileListChanged();
}

QString ProgressModel::formatTime(int seconds) const
{
    int hours = seconds / 3600;
    int minutes = (seconds % 3600) / 60;
    int secs = seconds % 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
            .arg(minutes)
            .arg(secs, 2, 10, QChar('0'));
    }
}