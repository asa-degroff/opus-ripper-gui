#include "ConversionModel.h"
#include <QFileInfo>
#include <algorithm>

ConversionModel::ConversionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

ConversionModel::~ConversionModel() = default;

int ConversionModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return m_items.size();
}

QVariant ConversionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return QVariant();
    
    const ConversionItem &item = m_items.at(index.row());
    
    switch (role) {
    case FileNameRole:
        return item.fileName;
    case InputPathRole:
        return item.inputPath;
    case OutputPathRole:
        return item.outputPath;
    case RelativePathRole:
        return item.relativePath;
    case FileSizeRole:
        return item.fileSize;
    case StatusRole:
        return item.status;
    case ProgressRole:
        return item.progress;
    case ErrorRole:
        return item.error;
    case StartTimeRole:
        return item.startTime;
    case EndTimeRole:
        return item.endTime;
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ConversionModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[FileNameRole] = "fileName";
    roles[InputPathRole] = "inputPath";
    roles[OutputPathRole] = "outputPath";
    roles[RelativePathRole] = "relativePath";
    roles[FileSizeRole] = "fileSize";
    roles[StatusRole] = "status";
    roles[ProgressRole] = "progress";
    roles[ErrorRole] = "error";
    roles[StartTimeRole] = "startTime";
    roles[EndTimeRole] = "endTime";
    return roles;
}

void ConversionModel::addFile(const ConversionItem &item)
{
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append(item);
    m_pathToIndex[item.inputPath] = m_items.size() - 1;
    endInsertRows();
    
    emit totalFilesChanged();
}

void ConversionModel::addFiles(const QList<ConversionItem> &items)
{
    if (items.isEmpty())
        return;
    
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size() + items.size() - 1);
    for (const auto &item : items) {
        m_items.append(item);
        m_pathToIndex[item.inputPath] = m_items.size() - 1;
    }
    endInsertRows();
    
    emit totalFilesChanged();
}

void ConversionModel::clear()
{
    beginResetModel();
    m_items.clear();
    m_pathToIndex.clear();
    endResetModel();
    
    emit totalFilesChanged();
    emit completedFilesChanged();
    emit failedFilesChanged();
}

void ConversionModel::updateFileStatus(const QString &inputPath, const QString &status, int progress, const QString &error)
{
    int index = findItemIndex(inputPath);
    if (index < 0)
        return;
    
    m_items[index].status = status;
    m_items[index].progress = progress;
    m_items[index].error = error;
    
    if (status == "converting") {
        m_items[index].startTime = QDateTime::currentDateTime();
    } else if (status == "completed" || status == "failed") {
        m_items[index].endTime = QDateTime::currentDateTime();
    }
    
    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex);
    
    if (status == "completed" || status == "failed") {
        emit completedFilesChanged();
        if (status == "failed") {
            emit failedFilesChanged();
        }
    }
    
    emit fileStatusChanged(inputPath, status);
}

void ConversionModel::updateFileProgress(const QString &inputPath, int progress)
{
    int index = findItemIndex(inputPath);
    if (index < 0)
        return;
    
    m_items[index].progress = progress;
    
    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, {ProgressRole});
}

int ConversionModel::completedFiles() const
{
    return std::count_if(m_items.begin(), m_items.end(),
        [](const ConversionItem &item) { return item.status == "completed"; });
}

int ConversionModel::failedFiles() const
{
    return std::count_if(m_items.begin(), m_items.end(),
        [](const ConversionItem &item) { return item.status == "failed"; });
}

ConversionItem ConversionModel::getItem(int index) const
{
    if (index >= 0 && index < m_items.size()) {
        return m_items.at(index);
    }
    return ConversionItem();
}

ConversionItem ConversionModel::getItemByPath(const QString &inputPath) const
{
    int index = findItemIndex(inputPath);
    if (index >= 0) {
        return m_items.at(index);
    }
    return ConversionItem();
}

int ConversionModel::findItemIndex(const QString &inputPath) const
{
    auto it = m_pathToIndex.find(inputPath);
    if (it != m_pathToIndex.end()) {
        return it.value();
    }
    return -1;
}