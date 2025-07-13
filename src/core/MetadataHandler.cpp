#include "MetadataHandler.h"
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/flacfile.h>
#include <taglib/opusfile.h>
#include <taglib/tpropertymap.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/xiphcomment.h>
#include <taglib/flacpicture.h>
#include <QDebug>
#include <QBuffer>

MetadataHandler::MetadataHandler(QObject *parent)
    : QObject(parent)
{
}

MetadataHandler::~MetadataHandler() = default;

bool MetadataHandler::readFlacMetadata(const QString &filePath, AudioMetadata &metadata)
{
    TagLib::FLAC::File file(filePath.toStdString().c_str());
    
    if (!file.isValid()) {
        m_lastError = "Invalid FLAC file";
        emit metadataError(m_lastError);
        return false;
    }
    
    // Get Xiph comment (Vorbis comments) for complete metadata access
    TagLib::Ogg::XiphComment *xiphComment = file.xiphComment();
    if (xiphComment) {
        // Use PropertyMap for comprehensive tag reading
        TagLib::PropertyMap properties = xiphComment->properties();
        
        // Read standard tags with proper UTF-8 handling
        if (properties.contains("TITLE") && !properties["TITLE"].isEmpty()) {
            metadata.title = QString::fromUtf8(properties["TITLE"].front().toCString(true));
        }
        if (properties.contains("ARTIST") && !properties["ARTIST"].isEmpty()) {
            metadata.artist = QString::fromUtf8(properties["ARTIST"].front().toCString(true));
        }
        if (properties.contains("ALBUM") && !properties["ALBUM"].isEmpty()) {
            metadata.album = QString::fromUtf8(properties["ALBUM"].front().toCString(true));
        }
        if (properties.contains("ALBUMARTIST") && !properties["ALBUMARTIST"].isEmpty()) {
            metadata.albumArtist = QString::fromUtf8(properties["ALBUMARTIST"].front().toCString(true));
        }
        if (properties.contains("GENRE") && !properties["GENRE"].isEmpty()) {
            metadata.genre = QString::fromUtf8(properties["GENRE"].front().toCString(true));
        }
        if (properties.contains("COMMENT") && !properties["COMMENT"].isEmpty()) {
            metadata.comment = QString::fromUtf8(properties["COMMENT"].front().toCString(true));
        }
        if (properties.contains("DATE") && !properties["DATE"].isEmpty()) {
            metadata.date = QString::fromUtf8(properties["DATE"].front().toCString(true));
            // Try to extract year from date
            bool ok;
            int year = metadata.date.left(4).toInt(&ok);
            if (ok) metadata.year = year;
        }
        if (properties.contains("TRACKNUMBER") && !properties["TRACKNUMBER"].isEmpty()) {
            metadata.track = properties["TRACKNUMBER"].front().toInt();
        }
        if (properties.contains("DISCNUMBER") && !properties["DISCNUMBER"].isEmpty()) {
            metadata.discNumber = properties["DISCNUMBER"].front().toInt();
        }
        
        // Store all custom tags for complete preservation
        for (auto it = properties.begin(); it != properties.end(); ++it) {
            QString key = QString::fromUtf8(it->first.toCString(true));
            if (!it->second.isEmpty()) {
                QString value = QString::fromUtf8(it->second.front().toCString(true));
                metadata.customTags[key] = value;
            }
        }
    }
    
    // Extract pictures
    extractFlacPictures(&file, metadata);
    
    return true;
}

bool MetadataHandler::writeOpusMetadata(const QString &filePath, const AudioMetadata &metadata)
{
    TagLib::Ogg::Opus::File file(filePath.toStdString().c_str());
    
    if (!file.isValid()) {
        m_lastError = "Invalid Opus file";
        emit metadataError(m_lastError);
        return false;
    }
    
    // Get Xiph comment for Opus file
    TagLib::Ogg::XiphComment *xiphComment = file.tag();
    if (xiphComment) {
        // Clear existing tags to avoid duplicates
        xiphComment->removeAllFields();
        
        // Write standard tags with proper UTF-8 encoding
        if (!metadata.title.isEmpty()) {
            xiphComment->addField("TITLE", TagLib::String(metadata.title.toUtf8().constData(), TagLib::String::UTF8));
        }
        if (!metadata.artist.isEmpty()) {
            xiphComment->addField("ARTIST", TagLib::String(metadata.artist.toUtf8().constData(), TagLib::String::UTF8));
        }
        if (!metadata.album.isEmpty()) {
            xiphComment->addField("ALBUM", TagLib::String(metadata.album.toUtf8().constData(), TagLib::String::UTF8));
        }
        if (!metadata.albumArtist.isEmpty()) {
            xiphComment->addField("ALBUMARTIST", TagLib::String(metadata.albumArtist.toUtf8().constData(), TagLib::String::UTF8));
        }
        if (!metadata.genre.isEmpty()) {
            xiphComment->addField("GENRE", TagLib::String(metadata.genre.toUtf8().constData(), TagLib::String::UTF8));
        }
        if (!metadata.comment.isEmpty()) {
            xiphComment->addField("COMMENT", TagLib::String(metadata.comment.toUtf8().constData(), TagLib::String::UTF8));
        }
        if (!metadata.date.isEmpty()) {
            xiphComment->addField("DATE", TagLib::String(metadata.date.toUtf8().constData(), TagLib::String::UTF8));
        }
        if (metadata.track > 0) {
            xiphComment->addField("TRACKNUMBER", TagLib::String::number(metadata.track));
        }
        if (metadata.discNumber > 0) {
            xiphComment->addField("DISCNUMBER", TagLib::String::number(metadata.discNumber));
        }
        
        // Write all custom tags
        for (auto it = metadata.customTags.constBegin(); it != metadata.customTags.constEnd(); ++it) {
            // Skip standard fields to avoid duplicates
            QString upperKey = it.key().toUpper();
            if (upperKey != "TITLE" && upperKey != "ARTIST" && upperKey != "ALBUM" && 
                upperKey != "ALBUMARTIST" && upperKey != "GENRE" && upperKey != "COMMENT" && 
                upperKey != "DATE" && upperKey != "TRACKNUMBER" && upperKey != "DISCNUMBER") {
                xiphComment->addField(it.key().toUtf8().constData(), 
                                    TagLib::String(it.value().toUtf8().constData(), TagLib::String::UTF8));
            }
        }
        
        // Embed album art if available
        if (!metadata.albumArt.isEmpty()) {
            embedOpusPicture(xiphComment, metadata.albumArt, metadata.albumArtMimeType);
        }
    }
    
    return file.save();
}

bool MetadataHandler::copyMetadata(const QString &flacPath, const QString &opusPath)
{
    AudioMetadata metadata;
    if (!readFlacMetadata(flacPath, metadata)) {
        return false;
    }
    
    return writeOpusMetadata(opusPath, metadata);
}

bool MetadataHandler::extractFlacPictures(TagLib::FLAC::File *file, AudioMetadata &metadata)
{
    if (!file) return false;
    
    const TagLib::List<TagLib::FLAC::Picture *> &pictures = file->pictureList();
    
    // Look for front cover first, then use first available picture
    TagLib::FLAC::Picture *selectedPicture = nullptr;
    
    for (auto picture : pictures) {
        if (picture->type() == TagLib::FLAC::Picture::FrontCover) {
            selectedPicture = picture;
            break;
        }
        if (!selectedPicture) {
            selectedPicture = picture;
        }
    }
    
    if (selectedPicture) {
        // Extract picture data
        const TagLib::ByteVector &data = selectedPicture->data();
        metadata.albumArt = QByteArray(data.data(), data.size());
        
        // Extract MIME type
        metadata.albumArtMimeType = QString::fromUtf8(selectedPicture->mimeType().toCString(true));
        
        return true;
    }
    
    return false;
}

bool MetadataHandler::embedOpusPicture(TagLib::Ogg::XiphComment *xiphComment, const QByteArray &pictureData, const QString &mimeType)
{
    if (!xiphComment || pictureData.isEmpty()) return false;
    
    // Create FLAC picture structure
    TagLib::FLAC::Picture picture;
    picture.setType(TagLib::FLAC::Picture::FrontCover);
    picture.setMimeType(mimeType.toUtf8().constData());
    picture.setData(TagLib::ByteVector(pictureData.data(), pictureData.size()));
    
    // Convert to base64 for METADATA_BLOCK_PICTURE format
    TagLib::ByteVector blockData = picture.render();
    TagLib::String base64Data = blockData.toBase64();
    
    // Add as METADATA_BLOCK_PICTURE field (standard for Opus/Vorbis)
    xiphComment->addField("METADATA_BLOCK_PICTURE", base64Data);
    
    return true;
}

void MetadataHandler::copyStandardTags(TagLib::Tag *source, TagLib::Tag *dest)
{
    if (source && dest) {
        dest->setTitle(source->title());
        dest->setArtist(source->artist());
        dest->setAlbum(source->album());
        dest->setGenre(source->genre());
        dest->setComment(source->comment());
        dest->setYear(source->year());
        dest->setTrack(source->track());
    }
}

void MetadataHandler::copyVorbisComments(TagLib::FLAC::File *flacFile, TagLib::Ogg::Opus::File *opusFile)
{
    if (!flacFile || !opusFile) return;
    
    TagLib::Ogg::XiphComment *flacComment = flacFile->xiphComment();
    TagLib::Ogg::XiphComment *opusComment = opusFile->tag();
    
    if (!flacComment || !opusComment) return;
    
    // Get all fields from FLAC
    const TagLib::Ogg::FieldListMap &fieldMap = flacComment->fieldListMap();
    
    // Copy all fields to Opus
    for (auto it = fieldMap.begin(); it != fieldMap.end(); ++it) {
        const TagLib::String &key = it->first;
        const TagLib::StringList &values = it->second;
        
        // Add all values for this field
        for (auto value : values) {
            opusComment->addField(key, value);
        }
    }
}