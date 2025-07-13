#ifndef CONVERSIONMODEL_H
#define CONVERSIONMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QString>
#include <QList>
#include <memory>

struct ConversionItem {
    QString inputPath;
    QString outputPath;
    QString fileName;
    QString relativePath;
    qint64 fileSize = 0;
    QString status = "pending"; // pending, converting, completed, failed
    int progress = 0;
    QString error;
    QDateTime startTime;
    QDateTime endTime;
};

class ConversionModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int totalFiles READ totalFiles NOTIFY totalFilesChanged)
    Q_PROPERTY(int completedFiles READ completedFiles NOTIFY completedFilesChanged)
    Q_PROPERTY(int failedFiles READ failedFiles NOTIFY failedFilesChanged)
    
public:
    enum ConversionRoles {
        FileNameRole = Qt::UserRole + 1,
        InputPathRole,
        OutputPathRole,
        RelativePathRole,
        FileSizeRole,
        StatusRole,
        ProgressRole,
        ErrorRole,
        StartTimeRole,
        EndTimeRole
    };
    
    explicit ConversionModel(QObject *parent = nullptr);
    ~ConversionModel();
    
    // QAbstractListModel interface
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    
    // Data management
    void addFile(const ConversionItem &item);
    void addFiles(const QList<ConversionItem> &items);
    void clear();
    void updateFileStatus(const QString &inputPath, const QString &status, int progress = 0, const QString &error = QString());
    void updateFileProgress(const QString &inputPath, int progress);
    
    // Getters
    int totalFiles() const { return m_items.size(); }
    int completedFiles() const;
    int failedFiles() const;
    ConversionItem getItem(int index) const;
    ConversionItem getItemByPath(const QString &inputPath) const;
    
signals:
    void totalFilesChanged();
    void completedFilesChanged();
    void failedFilesChanged();
    void fileStatusChanged(const QString &inputPath, const QString &status);
    
private:
    QList<ConversionItem> m_items;
    QHash<QString, int> m_pathToIndex; // For quick lookup by path
    
    int findItemIndex(const QString &inputPath) const;
};

#endif // CONVERSIONMODEL_H