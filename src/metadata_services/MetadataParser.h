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

#pragma once

#include "MediaLibrary.h"
#include "medialibrary/parser/IParserService.h"

namespace medialibrary
{

class AlbumTrack;

namespace parser
{

class MetadataAnalyzer : public IParserService
{
public:
    MetadataAnalyzer();

protected:
    bool cacheUnknownArtist();
    virtual bool initialize( IMediaLibrary* ml ) override;
    virtual Status run( IItem& item ) override;
    virtual const char* name() const override;
    virtual uint8_t nbThreads() const override;
    virtual void onFlushing() override;
    virtual void onRestarted() override;
    virtual Step targetedStep() const override;

    bool addPlaylistMedias( IItem& item ) const;
    void addPlaylistElement( IItem& item, std::shared_ptr<Playlist> playlistPtr,
                             const IItem& subitem ) const;
    bool parseAudioFile( IItem& task );
    bool parseVideoFile( IItem& task ) const;
    std::tuple<Status, bool> createFileAndMedia( IItem& item ) const;
    void createTracks( Media& m, const std::vector<IItem::Track>& tracks ) const;
    std::tuple<bool, bool> refreshFile( IItem& item ) const;
    std::tuple<bool, bool> refreshMedia( IItem& item ) const;
    std::pair<std::shared_ptr<Artist>, std::shared_ptr<Artist>> findOrCreateArtist( IItem& item ) const;
    std::shared_ptr<AlbumTrack> handleTrack( std::shared_ptr<Album> album, IItem& item,
                                             std::shared_ptr<Artist> artist, Genre* genre ) const;
    bool link(IItem& item, Album& album, std::shared_ptr<Artist> albumArtist, std::shared_ptr<Artist> artist );
    std::shared_ptr<Album> findAlbum( IItem& item, std::shared_ptr<Artist> albumArtist,
                                        std::shared_ptr<Artist> artist );
    std::shared_ptr<Genre> handleGenre( IItem& item ) const;

private:
    static int toInt( IItem& item, IItem::Metadata meta );

private:
    MediaLibrary* m_ml;
    std::shared_ptr<ModificationNotifier> m_notifier;

    std::shared_ptr<Artist> m_unknownArtist;
    std::shared_ptr<Artist> m_variousArtists;
    std::shared_ptr<Album> m_previousAlbum;
    int64_t m_previousFolderId;
};

}
}
