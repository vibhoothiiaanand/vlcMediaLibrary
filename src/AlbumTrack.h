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

#ifndef ALBUMTRACK_H
#define ALBUMTRACK_H

#include <sqlite3.h>
#include <string>
#include <vector>

#include "medialibrary/IAlbumTrack.h"
#include "medialibrary/IMediaLibrary.h"
#include "database/DatabaseHelpers.h"

namespace medialibrary
{

class Album;
class AlbumTrack;
class Artist;
class Media;
class Genre;

class AlbumTrack : public IAlbumTrack, public DatabaseHelpers<AlbumTrack>
{
    public:
        struct Table
        {
            static const std::string Name;
            static const std::string PrimaryKeyColumn;
            static int64_t AlbumTrack::*const PrimaryKey;
        };
        AlbumTrack( MediaLibraryPtr ml, sqlite::Row& row );
        AlbumTrack( MediaLibraryPtr ml, int64_t mediaId, int64_t artistId, int64_t genreId,
                    unsigned int trackNumber, int64_t albumId, unsigned int discNumber );

        virtual int64_t id() const override;
        virtual ArtistPtr artist() const override;
        virtual int64_t artistId() const override;
        virtual GenrePtr genre() override;
        virtual int64_t genreId() const override;
        virtual unsigned int trackNumber() const override;
        virtual unsigned int discNumber() const override;
        virtual std::shared_ptr<IAlbum> album() override;
        virtual int64_t albumId() const override;

        static void createTable( sqlite::Connection* dbConnection );
        static void createTriggers( sqlite::Connection* dbConnection );
        static std::shared_ptr<AlbumTrack> create(MediaLibraryPtr ml, int64_t albumId,
                                    std::shared_ptr<Media> media, unsigned int trackNb,
                                    unsigned int discNumber, int64_t artistId, int64_t genreId,
                                    int64_t duration );
        static AlbumTrackPtr fromMedia( MediaLibraryPtr ml, int64_t mediaId );
        static Query<IMedia> fromGenre( MediaLibraryPtr ml, int64_t genreId, const QueryParameters* params );

    private:
        MediaLibraryPtr m_ml;
        int64_t m_id;
        const int64_t m_mediaId;
        const int64_t m_artistId;
        const int64_t m_genreId;
        const unsigned int m_trackNumber;
        const int64_t m_albumId;
        const unsigned int m_discNumber;

        mutable std::weak_ptr<Album> m_album;
        mutable std::shared_ptr<Artist> m_artist;
        mutable std::shared_ptr<Genre> m_genre;

        friend struct AlbumTrack::Table;
};

}

#endif // ALBUMTRACK_H
