/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015 Hugo Beauzée-Luyssen, Videolabs
 *
 * Authors: Hugo Beauzée-Luyssen<hugo@beauzee.fr>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "Album.h"
#include "AlbumTrack.h"
#include "Artist.h"
#include "AudioTrack.h"
#include "Device.h"
#include "Media.h"
#include "File.h"
#include "Folder.h"
#include "Label.h"
#include "logging/Logger.h"
#include "Movie.h"
#include "ShowEpisode.h"
#include "SubtitleTrack.h"

#include "database/SqliteTools.h"
#include "database/SqliteQuery.h"
#include "VideoTrack.h"
#include "medialibrary/filesystem/IFile.h"
#include "medialibrary/filesystem/IDirectory.h"
#include "medialibrary/filesystem/IDevice.h"
#include "utils/Filename.h"

namespace medialibrary
{

const std::string Media::Table::Name = "Media";
const std::string Media::Table::PrimaryKeyColumn = "id_media";
int64_t Media::* const Media::Table::PrimaryKey = &Media::m_id;

Media::Media( MediaLibraryPtr ml, sqlite::Row& row )
    : m_ml( ml )
    // DB field extraction:
    , m_id( row.load<decltype(m_id)>( 0 ) )
    , m_type( row.load<decltype(m_type)>( 1 ) )
    , m_subType( row.load<decltype(m_subType)>( 2 ) )
    , m_duration( row.load<decltype(m_duration)>( 3 ) )
    , m_playCount( row.load<decltype(m_playCount)>( 4 ) )
    , m_lastPlayedDate( row.load<decltype(m_lastPlayedDate)>( 5 ) )
    // skip real_last_played_date as we don't need it in memory
    , m_insertionDate( row.load<decltype(m_insertionDate)>( 7 ) )
    , m_releaseDate( row.load<decltype(m_releaseDate)>( 8 ) )
    , m_thumbnailId( row.load<decltype(m_thumbnailId)>( 9 ) )
    , m_title( row.load<decltype(m_title)>( 10 ) )
    , m_filename( row.load<decltype(m_filename)>( 11 ) )
    , m_isFavorite( row.load<decltype(m_isFavorite)>( 12 ) )
    // Skip is_present
    // Skip device_id
    , m_nbPlaylists( row.load<unsigned int>( 15 ) )
    // Skip folder_id if any field gets added afterward

    // End of DB fields extraction
    , m_metadata( m_ml, IMetadata::EntityType::Media )
    , m_changed( false )
{
}

Media::Media( MediaLibraryPtr ml, const std::string& title, Type type )
    : m_ml( ml )
    , m_id( 0 )
    , m_type( type )
    , m_subType( SubType::Unknown )
    , m_duration( -1 )
    , m_playCount( 0 )
    , m_lastPlayedDate( 0 )
    , m_insertionDate( time( nullptr ) )
    , m_releaseDate( 0 )
    , m_thumbnailId( 0 )
    , m_title( title )
    // When creating a Media, meta aren't parsed, and therefor, the title is the filename
    , m_filename( title )
    , m_isFavorite( false )
    , m_nbPlaylists( 0 )
    , m_metadata( m_ml, IMetadata::EntityType::Media )
    , m_changed( false )
{
}

std::shared_ptr<Media> Media::create( MediaLibraryPtr ml, Type type,
                                      int64_t deviceId, int64_t folderId,
                                      const std::string& fileName )
{
    auto self = std::make_shared<Media>( ml, fileName, type );
    static const std::string req = "INSERT INTO " + Media::Table::Name +
            "(type, insertion_date, title, filename, device_id, folder_id) "
            "VALUES(?, ?, ?, ?, ?, ?)";

    if ( insert( ml, self, req, type, self->m_insertionDate, self->m_title,
                 self->m_filename, sqlite::ForeignKey{ deviceId },
                 sqlite::ForeignKey{ folderId } ) == false )
        return nullptr;
    return self;
}

AlbumTrackPtr Media::albumTrack() const
{
    if ( m_subType != SubType::AlbumTrack )
        return nullptr;
    if ( m_albumTrack == nullptr )
        m_albumTrack = AlbumTrack::fromMedia( m_ml, m_id );
    return m_albumTrack;
}

void Media::setAlbumTrack( AlbumTrackPtr albumTrack )
{
    m_albumTrack = albumTrack;
    m_subType = SubType::AlbumTrack;
    m_changed = true;
}

int64_t Media::duration() const
{
    return m_duration;
}

void Media::setDuration( int64_t duration )
{
    if ( m_duration == duration )
        return;
    m_duration = duration;
    m_changed = true;
}

ShowEpisodePtr Media::showEpisode() const
{
    if ( m_subType != SubType::ShowEpisode )
        return nullptr;

    if ( m_showEpisode == nullptr )
        m_showEpisode = ShowEpisode::fromMedia( m_ml, m_id );
    return m_showEpisode;
}

void Media::setShowEpisode( ShowEpisodePtr episode )
{
    m_showEpisode = episode;
    m_subType = SubType::ShowEpisode;
    m_changed = true;
}

Query<ILabel> Media::labels() const
{
    static const std::string req = "FROM " + Label::Table::Name + " l "
            "INNER JOIN LabelFileRelation lfr ON lfr.label_id = l.id_label "
            "WHERE lfr.media_id = ?";
    return make_query<Label, ILabel>( m_ml, "l.*", req, "", m_id );
}

uint32_t Media::playCount() const
{
    return m_playCount;
}

bool Media::increasePlayCount()
{
    static const std::string req = "UPDATE " + Media::Table::Name + " SET "
            "play_count = ?, last_played_date = ?, real_last_played_date = ? "
            "WHERE id_media = ?";
    auto lastPlayedDate = time( nullptr );
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_playCount + 1,
                                       lastPlayedDate, lastPlayedDate, m_id ) == false )
        return false;
    m_playCount++;
    m_lastPlayedDate = lastPlayedDate;
    return true;
}

bool Media::setPlayCount( uint32_t playCount )
{
    static const std::string req = "UPDATE " + Media::Table::Name + " SET "
            "play_count = ? WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, playCount, m_id ) == false )
        return false;
    m_playCount = playCount;
    return true;
}

time_t Media::lastPlayedDate() const
{
    return m_lastPlayedDate;
}

bool Media::isFavorite() const
{
    return m_isFavorite;
}

bool Media::setFavorite( bool favorite )
{
    static const std::string req = "UPDATE " + Media::Table::Name + " SET is_favorite = ? WHERE id_media = ?";
    if ( m_isFavorite == favorite )
        return true;
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, favorite, m_id ) == false )
        return false;
    m_isFavorite = favorite;
    return true;
}

const std::vector<FilePtr>& Media::files() const
{
    if ( m_files.empty() == true )
    {
        static const std::string req = "SELECT * FROM " + File::Table::Name
                + " WHERE media_id = ?";
        m_files = File::fetchAll<IFile>( m_ml, req, m_id );
    }
    return m_files;
}

const std::string& Media::fileName() const
{
    return m_filename;
}

MoviePtr Media::movie() const
{
    if ( m_subType != SubType::Movie )
        return nullptr;

    if ( m_movie == nullptr )
        m_movie = Movie::fromMedia( m_ml, m_id );
    return m_movie;
}

void Media::setMovie(MoviePtr movie)
{
    m_movie = movie;
    m_subType = SubType::Movie;
    m_changed = true;
}

bool Media::addVideoTrack( const std::string& codec, unsigned int width, unsigned int height,
                           uint32_t fpsNum, uint32_t fpsDen, uint32_t bitrate,
                           uint32_t sarNum, uint32_t sarDen, const std::string& language,
                           const std::string& description )
{
    return VideoTrack::create( m_ml, codec, width, height, fpsNum, fpsDen,
                               bitrate, sarNum, sarDen, m_id, language, description ) != nullptr;
}

Query<IVideoTrack> Media::videoTracks() const
{
    static const std::string req = "FROM " + VideoTrack::Table::Name +
            " WHERE media_id = ?";
    return make_query<VideoTrack, IVideoTrack>( m_ml, "*", req, "", m_id );
}

bool Media::addAudioTrack( const std::string& codec, unsigned int bitrate,
                          unsigned int sampleRate, unsigned int nbChannels,
                          const std::string& language, const std::string& desc )
{
    return AudioTrack::create( m_ml, codec, bitrate, sampleRate, nbChannels, language, desc, m_id ) != nullptr;
}

bool Media::addSubtitleTrack( std::string codec, std::string language,
                              std::string description, std::string encoding )
{
    return SubtitleTrack::create( m_ml, std::move( codec ), std::move( language ),
                                  std::move( description ), std::move( encoding ),
                                  m_id ) != nullptr;
}

Query<IAudioTrack> Media::audioTracks() const
{
    static const std::string req = "FROM " + AudioTrack::Table::Name +
            " WHERE media_id = ?";
    return make_query<AudioTrack, IAudioTrack>( m_ml, "*", req, "", m_id );
}

Query<ISubtitleTrack> Media::subtitleTracks() const
{
    static const std::string req = "FROM " + SubtitleTrack::Table::Name +
            " WHERE media_id = ?";
    return make_query<SubtitleTrack, ISubtitleTrack>( m_ml, "*", req, "", m_id );
}

const std::string& Media::thumbnail() const
{
    if ( isThumbnailGenerated() == false )
        return Thumbnail::EmptyMrl;

    if ( m_thumbnail == nullptr )
    {
        auto thumbnail = Thumbnail::fetch( m_ml, m_thumbnailId );
        if ( thumbnail == nullptr )
            return Thumbnail::EmptyMrl;
        m_thumbnail = std::move( thumbnail );
    }
    if ( m_thumbnail->isValid() == false )
        return Thumbnail::EmptyMrl;
    return m_thumbnail->mrl();
}

bool Media::isThumbnailGenerated() const
{
    return m_thumbnailId != 0;
}

unsigned int Media::insertionDate() const
{
    return static_cast<unsigned int>( m_insertionDate );
}

unsigned int Media::releaseDate() const
{
    return m_releaseDate;
}

uint32_t Media::nbPlaylists() const
{
    return m_nbPlaylists.load( std::memory_order_relaxed );
}

void Media::udpateNbPlaylist(int32_t increment) const
{
    // Only update the cached representation, let the triggers handle the DB values
    m_nbPlaylists.fetch_add( increment, std::memory_order_relaxed );
}

const IMetadata& Media::metadata( IMedia::MetadataType type ) const
{
    using MDType = typename std::underlying_type<IMedia::MetadataType>::type;
    if ( m_metadata.isReady() == false )
        m_metadata.init( m_id, IMedia::NbMeta );
    return m_metadata.get( static_cast<MDType>( type ) );
}

bool Media::setMetadata( IMedia::MetadataType type, const std::string& value )
{
    using MDType = typename std::underlying_type<IMedia::MetadataType>::type;
    if ( m_metadata.isReady() == false )
        m_metadata.init( m_id, IMedia::NbMeta );
    return m_metadata.set( static_cast<MDType>( type ), value );
}

bool Media::setMetadata( IMedia::MetadataType type, int64_t value )
{
    using MDType = typename std::underlying_type<IMedia::MetadataType>::type;
    if ( m_metadata.isReady() == false )
        m_metadata.init( m_id, IMedia::NbMeta );
    return m_metadata.set( static_cast<MDType>( type ), value );
}

bool Media::unsetMetadata(IMedia::MetadataType type)
{
    using MDType = typename std::underlying_type<IMedia::MetadataType>::type;
    if ( m_metadata.isReady() == false )
        m_metadata.init( m_id, IMedia::NbMeta );
    return m_metadata.unset( static_cast<MDType>( type ) );
}

void Media::setReleaseDate( unsigned int date )
{
    if ( m_releaseDate == date )
        return;
    m_releaseDate = date;
    m_changed = true;
}

bool Media::setThumbnail( const std::string& thumbnailMrl, Thumbnail::Origin origin,
                          bool isGenerated )
{
    if ( m_thumbnailId != 0 )
        return Thumbnail::setMrlFromPrimaryKey( m_ml, m_thumbnail, m_thumbnailId,
                                                thumbnailMrl, origin );

    std::unique_ptr<sqlite::Transaction> t;
    if ( sqlite::Transaction::transactionInProgress() == false )
        t = m_ml->getConn()->newTransaction();
    std::shared_ptr<Thumbnail> thumbnail;
    if ( thumbnailMrl.empty() )
    {
        assert( origin == Thumbnail::Origin::Media );
        assert( isGenerated == true );
        thumbnail = Thumbnail::createForFailure( m_ml );
    }
    else
        thumbnail = Thumbnail::create( m_ml, thumbnailMrl, origin, isGenerated );
    if ( thumbnail == nullptr )
        return false;

    static const std::string req = "UPDATE " + Media::Table::Name + " SET "
            "thumbnail_id = ? WHERE id_media = ?";
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, thumbnail->id(), m_id ) == false )
        return false;
    m_thumbnailId = thumbnail->id();
    m_thumbnail = std::move( thumbnail );
    if ( t != nullptr )
        t->commit();
    return true;
}

bool Media::setThumbnail( const std::string& thumbnailMrl )
{
    return setThumbnail( thumbnailMrl, Thumbnail::Origin::UserProvided, false );
}

bool Media::save()
{
    static const std::string req = "UPDATE " + Media::Table::Name + " SET "
            "type = ?, subtype = ?, duration = ?, release_date = ?,"
            "title = ? WHERE id_media = ?";
    if ( m_changed == false )
        return true;
    if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, m_type, m_subType, m_duration,
                                       m_releaseDate, m_title, m_id ) == false )
    {
        return false;
    }
    m_changed = false;
    return true;
}

std::shared_ptr<File> Media::addFile( const fs::IFile& fileFs, int64_t parentFolderId,
                                      bool isFolderFsRemovable, IFile::Type type )
{
    return File::createFromMedia( m_ml, m_id, type, fileFs, parentFolderId, isFolderFsRemovable);
}

FilePtr Media::addExternalMrl( const std::string& mrl , IFile::Type type )
{
    try
    {
        return File::createFromMedia( m_ml, m_id, type, mrl );
    }
    catch ( const sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Failed to add media external MRL: ", ex.what() );
        return nullptr;
    }
}

void Media::removeFile( File& file )
{
    file.destroy();
    auto it = std::remove_if( begin( m_files ), end( m_files ), [&file]( const FilePtr& f ) {
        return f->id() == file.id();
    });
    if ( it != end( m_files ) )
        m_files.erase( it );
}

std::string Media::addRequestJoin( const QueryParameters* params, bool forceFile,
                                    bool forceAlbumTrack )
{
    bool albumTrack = forceAlbumTrack;
    bool artist = false;
    bool album = false;
    bool file = forceFile;
    auto sort = params != nullptr ? params->sort : SortingCriteria::Alpha;

    switch( sort )
    {
        case SortingCriteria::Default:
        case SortingCriteria::Alpha:
        case SortingCriteria::PlayCount:
        case SortingCriteria::Duration:
        case SortingCriteria::InsertionDate:
        case SortingCriteria::ReleaseDate:
        case SortingCriteria::Filename:
            /* All those are stored in the media itself */
            break;
        case SortingCriteria::LastModificationDate:
        case SortingCriteria::FileSize:
            file = true;
            break;
        case SortingCriteria::Artist:
            artist = true;
            albumTrack = true;
            break;
        case SortingCriteria::Album:
            /* We need the album track to get the album id & the album for its title */
            albumTrack = true;
            album = true;
            break;
        case SortingCriteria::TrackNumber:
            albumTrack = true;
            break;
        case SortingCriteria::NbAudio:
        case SortingCriteria::NbVideo:
        case SortingCriteria::NbMedia:
            // Unrelated to media requests
            break;
    }
    std::string req;
    // Use "LEFT JOIN to allow for ordering different media type
    // For instance ordering by albums on all media would not fetch the video if
    // we were using INNER JOIN
    if ( albumTrack == true )
        req += " LEFT JOIN " + AlbumTrack::Table::Name + " att ON m.id_media = att.media_id ";
    if ( album == true )
    {
        assert( albumTrack == true );
        req += " LEFT JOIN " + Album::Table::Name + " alb ON att.album_id = alb.id_album ";
    }
    if ( artist == true )
    {
        assert( albumTrack == true );
        req += " LEFT JOIN " + Artist::Table::Name + " art ON att.artist_id = art.id_artist ";
    }
    if ( file == true )
        req += " LEFT JOIN " + File::Table::Name + " f ON m.id_media = f.media_id ";

    return req;
}

std::string Media::sortRequest( const QueryParameters* params )
{
    std::string req = " ORDER BY ";

    auto sort = params != nullptr ? params->sort : SortingCriteria::Default;
    auto desc = params != nullptr ? params->desc : false;
    switch ( sort )
    {
    case SortingCriteria::Duration:
        req += "m.duration";
        break;
    case SortingCriteria::InsertionDate:
        req += "m.insertion_date";
        break;
    case SortingCriteria::ReleaseDate:
        req += "m.release_date";
        break;
    case SortingCriteria::PlayCount:
        req += "m.play_count";
        desc = !desc; // Make decreasing order default for play count sorting
        break;
    case SortingCriteria::Filename:
        req += "m.filename";
        break;
    case SortingCriteria::LastModificationDate:
        req += "f.last_modification_date";
        break;
    case SortingCriteria::FileSize:
        req += "f.size";
        break;
    case SortingCriteria::Album:
        if ( desc == true )
            req += "alb.title DESC, att.track_number";
        else
            req += "alb.title, att.track_number";
        break;
    case SortingCriteria::Artist:
        req += "art.name";
        break;
    case SortingCriteria::TrackNumber:
        if ( desc == true )
            req += "att.track_number DESC, att.disc_number";
        else
            req += "att.track_number, att.disc_number";
        break;
    default:
        LOG_WARN( "Unsupported sorting criteria, falling back to SortingCriteria::Default (Alpha)" );
        /* fall-through */
    case SortingCriteria::Default:
    case SortingCriteria::Alpha:
        req += "m.title";
        break;
    }
    if ( desc == true && sort != SortingCriteria::Album )
        req += " DESC";
    return req;
}

Query<IMedia> Media::listAll( MediaLibraryPtr ml, IMedia::Type type,
                              const QueryParameters* params )
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true, false );
    req +=  " WHERE m.type = ?"
            " AND (f.type = ? OR f.type = ?)"
            " AND m.is_present != 0";

    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ), type,
                                      IFile::Type::Main, IFile::Type::Disc );
}

int64_t Media::id() const
{
    return m_id;
}

IMedia::Type Media::type() const
{
    return m_type;
}

IMedia::SubType Media::subType() const
{
    return m_subType;
}

void Media::setSubType( IMedia::SubType subType )
{
    if ( subType == m_subType )
        return;
    m_subType = subType;
    m_changed = true;
}

void Media::setType( Type type )
{
    if ( m_type == type )
        return;
    m_type = type;
    m_changed = true;
}

const std::string& Media::title() const
{
    return m_title;
}

bool Media::setTitle( const std::string& title )
{
    static const std::string req = "UPDATE " + Media::Table::Name + " SET title = ? WHERE id_media = ?";
    if ( m_title == title )
        return true;
    try
    {
        if ( sqlite::Tools::executeUpdate( m_ml->getConn(), req, title, m_id ) == false )
            return false;
    }
    catch ( const sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Failed to set media title: ", ex.what() );
        return false;
    }

    m_title = title;
    return true;
}

void Media::setTitleBuffered( const std::string& title )
{
    if ( m_title == title )
        return;
    m_title = title;
    m_changed = true;
}

void Media::setFileName( std::string fileName )
{
    if ( fileName == m_filename )
        return;
    static const std::string req = "UPDATE " + Media::Table::Name + " SET filename = ? WHERE id_media = ?";
    sqlite::Tools::executeUpdate( m_ml->getConn(), req, fileName, m_id );
    m_filename = std::move( fileName );
}

void Media::createTable( sqlite::Connection* connection, uint32_t modelVersion )
{
    std::string reqs[] = {
        #include "database/tables/Media_v14.sql"
    };
    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( connection, req );
    if ( modelVersion >= 14 )
    {
        // Don't create this index before model 14, as the real_last_played_date
        // was introduced in model version 14
        const auto req = "CREATE INDEX IF NOT EXISTS media_last_usage_dates_idx ON " + Media::Table::Name +
                "(last_played_date, real_last_played_date, insertion_date)";
        sqlite::Tools::executeRequest( connection, req );
    }
}

void Media::createTriggers( sqlite::Connection* connection, uint32_t modelVersion )
{
    const std::string reqs[] = {
        #include "database/tables/Media_triggers_v14.sql"
    };

    for ( const auto& req : reqs )
        sqlite::Tools::executeRequest( connection, req );

    if ( modelVersion >= 14 )
    {
        sqlite::Tools::executeRequest( connection,
            "CREATE TRIGGER IF NOT EXISTS increment_media_nb_playlist AFTER INSERT ON "
            " PlaylistMediaRelation "
            " BEGIN "
                " UPDATE " + Media::Table::Name + " SET nb_playlists = nb_playlists + 1 "
                    " WHERE id_media = new.media_id;"
            " END;"
        );

        sqlite::Tools::executeRequest( connection,
            "CREATE TRIGGER IF NOT EXISTS decrement_media_nb_playlist AFTER DELETE ON "
            " PlaylistMediaRelation "
            " BEGIN "
                " UPDATE " + Media::Table::Name + " SET nb_playlists = nb_playlists - 1 "
                    " WHERE id_media = old.media_id;"
            " END;"
        );
    }
}

bool Media::addLabel( LabelPtr label )
{
    if ( m_id == 0 || label->id() == 0 )
    {
        LOG_ERROR( "Both file & label need to be inserted in database before being linked together" );
        return false;
    }
    try
    {
        return sqlite::Tools::withRetries( 3, [this]( LabelPtr label ) {
            auto t = m_ml->getConn()->newTransaction();

            const char* req = "INSERT INTO LabelFileRelation VALUES(?, ?)";
            if ( sqlite::Tools::executeInsert( m_ml->getConn(), req, label->id(), m_id ) == 0 )
                return false;
            const std::string reqFts = "UPDATE " + Media::Table::Name + "Fts "
                "SET labels = labels || ' ' || ? WHERE rowid = ?";
            if ( sqlite::Tools::executeUpdate( m_ml->getConn(), reqFts, label->name(), m_id ) == false )
                return false;
            t->commit();
            return true;
        }, std::move( label ) );
    }
    catch ( const sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Failed to add label: ", ex.what() );
        return false;
    }
}

bool Media::removeLabel( LabelPtr label )
{
    if ( m_id == 0 || label->id() == 0 )
    {
        LOG_ERROR( "Can't unlink a label/file not inserted in database" );
        return false;
    }
    try
    {
        return sqlite::Tools::withRetries( 3, [this]( LabelPtr label ) {
            auto t = m_ml->getConn()->newTransaction();

            const char* req = "DELETE FROM LabelFileRelation WHERE label_id = ? AND media_id = ?";
            if ( sqlite::Tools::executeDelete( m_ml->getConn(), req, label->id(), m_id ) == false )
                return false;
            const std::string reqFts = "UPDATE " + Media::Table::Name + "Fts "
                    "SET labels = TRIM(REPLACE(labels, ?, '')) WHERE rowid = ?";
            if ( sqlite::Tools::executeUpdate( m_ml->getConn(), reqFts, label->name(), m_id ) == false )
                return false;
            t->commit();
            return true;
        }, std::move( label ) );
    }
    catch ( const sqlite::errors::Generic& ex )
    {
        LOG_ERROR( "Failed to remove label: ", ex.what() );
        return false;
    }
}

Query<IMedia> Media::search( MediaLibraryPtr ml, const std::string& title,
                             const QueryParameters* params )
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true, false );

    req +=  " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::Table::Name + "Fts"
            " WHERE " + Media::Table::Name + "Fts MATCH '*' || ? || '*')"
            " AND m.is_present = 1"
            " AND (f.type = ? OR f.type = ?)"
            " AND m.type != ? AND m.type != ?";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ), title,
                                      File::Type::Main, File::Type::Disc,
                                      Media::Type::External, Media::Type::Stream );
}

Query<IMedia> Media::search( MediaLibraryPtr ml, const std::string& title,
                             Media::Type type, const QueryParameters* params )
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true, false );
    req +=  " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::Table::Name + "Fts"
            " WHERE " + Media::Table::Name + "Fts MATCH '*' || ? || '*')"
            " AND m.is_present = 1"
            " AND (f.type = ? OR f.type = ?)"
            " AND m.type = ?";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ), title,
                                      File::Type::Main, File::Type::Disc,
                                      type );
}

Query<IMedia> Media::searchAlbumTracks(MediaLibraryPtr ml, const std::string& pattern, int64_t albumId, const QueryParameters* params)
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true, true );
    req +=  " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::Table::Name + "Fts"
            " WHERE " + Media::Table::Name + "Fts MATCH '*' || ? || '*')"
            " AND att.album_id = ?"
            " AND m.is_present = 1"
            " AND f.type = ?"
            " AND m.subtype = ?";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ), pattern, albumId,
                                      File::Type::Main, Media::SubType::AlbumTrack );
}

Query<IMedia> Media::searchArtistTracks(MediaLibraryPtr ml, const std::string& pattern, int64_t artistId, const QueryParameters* params)
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true, true );

    req +=  " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::Table::Name + "Fts"
            " WHERE " + Media::Table::Name + "Fts MATCH '*' || ? || '*')"
            " AND att.artist_id = ?"
            " AND m.is_present = 1"
            " AND f.type = ?"
            " AND m.subtype = ?";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ), pattern, artistId,
                                      File::Type::Main, Media::SubType::AlbumTrack );
}

Query<IMedia> Media::searchGenreTracks(MediaLibraryPtr ml, const std::string& pattern, int64_t genreId, const QueryParameters* params)
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true, true );

    req +=  " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::Table::Name + "Fts"
            " WHERE " + Media::Table::Name + "Fts MATCH '*' || ? || '*')"
            " AND att.genre_id = ?"
            " AND m.is_present = 1"
            " AND f.type = ?"
            " AND m.subtype = ?";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ), pattern, genreId,
                                      File::Type::Main, Media::SubType::AlbumTrack );
}

Query<IMedia> Media::searchShowEpisodes(MediaLibraryPtr ml, const std::string& pattern,
                                        int64_t showId, const QueryParameters* params)
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true, false );

    req +=  " INNER JOIN " + ShowEpisode::Table::Name + " ep ON ep.media_id = m.id_media "
            " WHERE"
            " m.id_media IN (SELECT rowid FROM " + Media::Table::Name + "Fts"
            " WHERE " + Media::Table::Name + "Fts MATCH '*' || ? || '*')"
            " AND ep.show_id = ?"
            " AND m.is_present = 1"
            " AND f.type = ?"
            " AND m.subtype = ?";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ), pattern, showId,
                                      File::Type::Main, Media::SubType::ShowEpisode );
}

Query<IMedia> Media::searchInPlaylist( MediaLibraryPtr ml, const std::string& pattern,
                                       int64_t playlistId, const QueryParameters* params )
{
    std::string req = "FROM " + Media::Table::Name + " m ";

    req += addRequestJoin( params, true, false );

    req += "LEFT JOIN PlaylistMediaRelation pmr ON pmr.media_id = m.id_media "
           "WHERE pmr.playlist_id = ? AND m.is_present != 0 AND "
           "m.id_media IN (SELECT rowid FROM " + Media::Table::Name + "Fts "
           "WHERE " + Media::Table::Name + "Fts MATCH '*' || ? || '*')";
    return make_query<Media, IMedia>( ml, "m.*", std::move( req ),
                                      sortRequest( params ), playlistId, pattern );
}

Query<IMedia> Media::fetchHistory( MediaLibraryPtr ml )
{
    static const std::string req = "FROM " + Media::Table::Name +
            " WHERE last_played_date IS NOT NULL"
            " AND type != ?";
    return make_query<Media, IMedia>( ml, "*", req,
                                      "ORDER BY last_played_date DESC",
                                      IMedia::Type::Stream );
}

Query<IMedia> Media::fetchStreamHistory(MediaLibraryPtr ml)
{
    static const std::string req = "FROM " + Media::Table::Name +
            " WHERE last_played_date IS NOT NULL"
            " AND type = ?";
    return make_query<Media, IMedia>( ml, "*", req,
                                      "ORDER BY last_played_date DESC",
                                      IMedia::Type::Stream );
}

Query<IMedia> Media::fromFolderId( MediaLibraryPtr ml, IMedia::Type type,
                                   int64_t folderId, const QueryParameters* params )
{
    // This assumes the folder is present, as folders are not expected to be
    // manipulated when the device is not present
    std::string req = "FROM " + Table::Name +  " m ";
    req += addRequestJoin( params, false, false );
    req += " WHERE m.folder_id = ?";
    if ( type != Type::Unknown )
    {
        req += " AND m.type = ?";
        return make_query<Media, IMedia>( ml, "*", req, sortRequest( params ),
                                          folderId, type );
    }
    // Don't explicitely filter by type since only video/audio media have a
    // non NULL folder_id
    return make_query<Media, IMedia>( ml, "*", req, sortRequest( params ),
                                      folderId );
}

void Media::clearHistory( MediaLibraryPtr ml )
{
    auto dbConn = ml->getConn();
    auto t = dbConn->newTransaction();
    static const std::string req = "UPDATE " + Media::Table::Name + " SET "
            "play_count = 0,"
            "last_played_date = NULL";

    using MDType = typename std::underlying_type<IMedia::MetadataType>::type;
    Metadata::unset( dbConn, IMetadata::EntityType::Media,
                     static_cast<MDType>( IMedia::MetadataType::Progress ) );

    sqlite::Tools::executeUpdate( dbConn, req );
    t->commit();
}

void Media::removeOldMedia( MediaLibraryPtr ml, std::chrono::seconds maxLifeTime )
{
    // Media that were never played have a real_last_played_date = NULL, so they
    // won't match for real_last_played_date < X
    // However we need to take care about media that were inserted but never played
    const std::string req = "DELETE FROM " + Media::Table::Name + " "
            "WHERE ( real_last_played_date < ? OR "
                "( real_last_played_date IS NULL AND insertion_date < ? ) )"
            "AND ( type = ? OR type = ? ) "
            "AND nb_playlists = 0";
    auto deadline = std::chrono::duration_cast<std::chrono::seconds>(
                (std::chrono::system_clock::now() - maxLifeTime).time_since_epoch() );
    sqlite::Tools::executeDelete( ml->getConn(), req, deadline.count(),
                                  deadline.count(),
                                  IMedia::Type::External, IMedia::Type::Stream );
}

}
