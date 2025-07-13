#ifndef METADATAHANDLER_H
#define METADATAHANDLER_H

#include <QObject>
#include <QString>
#include <QMap>
#include <QByteArray>
#include <memory>

// Forward declarations
namespace TagLib {
    class FileRef;
    class Tag;
    namespace FLAC {
        class File;
    }
    namespace Ogg {
        class XiphComment;
        namespace Opus {
            class File;
        }
    }
}

struct AudioMetadata {
    QString title;
    QString artist;
    QString album;
    QString albumArtist;
    QString genre;
    QString comment;
    QString date;
    int year = 0;
    int track = 0;
    int discNumber = 0;
    QByteArray albumArt;
    QString albumArtMimeType;
    
    // Additional Vorbis comment fields
    QMap<QString, QString> customTags;
};

class MetadataHandler : public QObject
{
    Q_OBJECT
    
public:
    explicit MetadataHandler(QObject *parent = nullptr);
    ~MetadataHandler();
    
    // Read metadata from FLAC file
    bool readFlacMetadata(const QString &filePath, AudioMetadata &metadata);
    
    // Write metadata to Opus file
    bool writeOpusMetadata(const QString &filePath, const AudioMetadata &metadata);
    
    // Copy metadata from FLAC to Opus
    bool copyMetadata(const QString &flacPath, const QString &opusPath);
    
    QString getLastError() const { return m_lastError; }
    
signals:
    void metadataError(const QString &error);
    
private:
    QString m_lastError;
    
    // Helper functions
    bool extractFlacPictures(TagLib::FLAC::File *file, AudioMetadata &metadata);
    bool embedOpusPicture(TagLib::Ogg::XiphComment *xiphComment, const QByteArray &pictureData, const QString &mimeType);
    void copyStandardTags(TagLib::Tag *source, TagLib::Tag *dest);
    void copyVorbisComments(TagLib::FLAC::File *flacFile, TagLib::Ogg::Opus::File *opusFile);
};

#endif // METADATAHANDLER_H