#include "MetadataHandler.h"
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/flacfile.h>
#include <taglib/opusfile.h>
#include <taglib/tpropertymap.h>
#include <taglib/attachedpictureframe.h>
#include <QDebug>

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
    
    TagLib::Tag *tag = file.tag();
    if (tag) {
        metadata.title = QString::fromStdString(tag->title().to8Bit(true));
        metadata.artist = QString::fromStdString(tag->artist().to8Bit(true));
        metadata.album = QString::fromStdString(tag->album().to8Bit(true));
        metadata.genre = QString::fromStdString(tag->genre().to8Bit(true));
        metadata.comment = QString::fromStdString(tag->comment().to8Bit(true));
        metadata.year = tag->year();
        metadata.track = tag->track();
    }
    
    // Extract pictures
    extractFlacPictures(&file, metadata);
    
    // TODO: Extract additional Vorbis comments
    
    return true;
}

bool MetadataHandler::writeOpusMetadata(const QString &filePath, const AudioMetadata &metadata)
{
    // TODO: Implement proper Ogg Opus metadata handling
    // For now, skip metadata writing for our simple format
    qDebug() << "Metadata writing not yet implemented for simple Opus format";
    return true;
    
    /*
    TagLib::Ogg::Opus::File file(filePath.toStdString().c_str());
    
    if (!file.isValid()) {
        m_lastError = "Invalid Opus file";
        emit metadataError(m_lastError);
        return false;
    }
    
    TagLib::Tag *tag = file.tag();
    if (tag) {
        tag->setTitle(TagLib::String(metadata.title.toStdString()));
        tag->setArtist(TagLib::String(metadata.artist.toStdString()));
        tag->setAlbum(TagLib::String(metadata.album.toStdString()));
        tag->setGenre(TagLib::String(metadata.genre.toStdString()));
        tag->setComment(TagLib::String(metadata.comment.toStdString()));
        tag->setYear(metadata.year);
        tag->setTrack(metadata.track);
    }
    
    // TODO: Embed album art
    
    return file.save();
    */
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
    // TODO: Implement picture extraction
    return true;
}

bool MetadataHandler::embedOpusPicture(TagLib::Ogg::Opus::File *file, const QByteArray &pictureData, const QString &mimeType)
{
    // TODO: Implement picture embedding
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
    // TODO: Implement Vorbis comment copying
}