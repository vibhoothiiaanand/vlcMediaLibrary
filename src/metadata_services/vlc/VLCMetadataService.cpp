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

#ifndef HAVE_LIBVLC
# error This file requires libvlc
#endif

#include <chrono>

#include "VLCMetadataService.h"
#include "Media.h"
#include "utils/VLCInstance.h"
#include "metadata_services/vlc/Common.hpp"
#include "utils/Filename.h"

namespace medialibrary
{
namespace parser
{

VLCMetadataService::VLCMetadataService()
    : m_instance( VLCInstance::get() )
{
}

bool VLCMetadataService::initialize( IMediaLibrary* )
{
    return true;
}

Status VLCMetadataService::run( IItem& item )
{
    auto mrl = item.mrl();
    LOG_INFO( "Parsing ", mrl );

    // Having a valid media means we're re-executing this parser after the thumbnailer,
    // which isn't expected, as we always mark this task as completed.
    VLC::Media vlcMedia{ m_instance, mrl, VLC::Media::FromType::FromLocation };

    VLC::Media::ParsedStatus status;
    bool done = false;

    auto event = vlcMedia.eventManager().onParsedChanged( [this, &status, &done](VLC::Media::ParsedStatus s ) {
        std::lock_guard<compat::Mutex> lock( m_mutex );
        status = s;
        done = true;
        m_cond.notify_all();
    });
    {
        std::unique_lock<compat::Mutex> lock( m_mutex );

        if ( vlcMedia.parseWithOptions( VLC::Media::ParseFlags::Local | VLC::Media::ParseFlags::Network |
                                             VLC::Media::ParseFlags::FetchLocal, 5000 ) == false )
            return Status::Fatal;
        m_cond.wait( lock, [&done]() {
            return done == true;
        });
    }
    event->unregister();
    if ( status == VLC::Media::ParsedStatus::Failed || status == VLC::Media::ParsedStatus::Timeout )
        return Status::Fatal;
    auto tracks = vlcMedia.tracks();
    auto artworkMrl = vlcMedia.meta( libvlc_meta_ArtworkURL );
    if ( ( tracks.size() == 0 && vlcMedia.subitems()->count() == 0 ) ||
         utils::file::schemeIs( "attachment://", artworkMrl ) == true )
    {
        if ( tracks.size() == 0 && vlcMedia.subitems()->count() == 0 )
            LOG_WARN( "Failed to fetch any tracks for ", mrl, ". Falling back to playback" );
        VLC::MediaPlayer mp( vlcMedia );
        auto res = MetadataCommon::startPlayback( vlcMedia, mp );
        if ( res == false )
            return Status::Fatal;
    }
    mediaToItem( vlcMedia, item );
    return Status::Success;
}

const char* VLCMetadataService::name() const
{
    return "VLC";
}

uint8_t VLCMetadataService::nbThreads() const
{
    return 1;
}

void VLCMetadataService::onFlushing()
{
}

void VLCMetadataService::onRestarted()
{
}

Step VLCMetadataService::targetedStep() const
{
    return Step::MetadataExtraction;
}

void VLCMetadataService::mediaToItem( VLC::Media& media, IItem& item )
{
    item.setMeta( IItem::Metadata::Title,
                  media.meta( libvlc_meta_Title ) );
    item.setMeta( IItem::Metadata::ArtworkUrl,
                  media.meta( libvlc_meta_ArtworkURL ) );
    item.setMeta( IItem::Metadata::ShowName,
                  media.meta( libvlc_meta_ShowName ) );
    item.setMeta( IItem::Metadata::Episode,
                  media.meta( libvlc_meta_Episode ) );
    item.setMeta( IItem::Metadata::Album,
                  media.meta( libvlc_meta_Album ) );
    item.setMeta( IItem::Metadata::Genre,
                  media.meta( libvlc_meta_Genre ) );
    item.setMeta( IItem::Metadata::Date,
                  media.meta( libvlc_meta_Date ) );
    item.setMeta( IItem::Metadata::AlbumArtist,
                  media.meta( libvlc_meta_AlbumArtist ) );
    item.setMeta( IItem::Metadata::Artist,
                  media.meta( libvlc_meta_Artist ) );
    item.setMeta( IItem::Metadata::TrackNumber,
                  media.meta( libvlc_meta_TrackNumber ) );
    item.setMeta( IItem::Metadata::DiscNumber,
                  media.meta( libvlc_meta_DiscNumber ) );
    item.setMeta( IItem::Metadata::DiscTotal,
                  media.meta( libvlc_meta_DiscTotal ) );
    item.setDuration( media.duration() );

    auto tracks = media.tracks();
    for ( const auto& track : tracks )
    {
        IItem::Track t;

        if ( track.type() == VLC::MediaTrack::Type::Audio )
        {
            t.type = IItem::Track::Type::Audio;
            t.a.nbChannels = track.channels();
            t.a.rate = track.rate();
        }
        else if ( track.type() == VLC::MediaTrack::Type::Video )
        {
            t.type = IItem::Track::Type::Video;
            t.v.fpsNum = track.fpsNum();
            t.v.fpsDen = track.fpsDen();
            t.v.width = track.width();
            t.v.height = track.height();
            t.v.sarNum = track.sarNum();
            t.v.sarDen = track.sarDen();
        }
        else if ( track.type() == VLC::MediaTrack::Type::Subtitle )
        {
            t.type = IItem::Track::Type::Subtitle;
            t.codec = track.codec();
            t.language = track.language();
            t.description = track.description();
            const auto& enc = track.encoding();
            strncpy( t.s.encoding, enc.c_str(), sizeof(t.s.encoding) - 1 );
        }
        else
            continue;
        auto codec = track.codec();
        std::string fcc( reinterpret_cast<const char*>( &codec ), 4 );
        t.codec = std::move( fcc );

        t.bitrate = track.bitrate();
        t.language = track.language();
        t.description = track.description();

        item.addTrack( std::move( t ) );
    }

    auto subItems = media.subitems();
    if ( subItems != nullptr )
    {
        for ( auto i = 0; i < subItems->count(); ++i )
        {
            auto vlcMedia = subItems->itemAtIndex( i );
            assert( vlcMedia != nullptr );
            // Always add 1 to the playlist/subitem index, as 0 is an invalid index
            // in this context
            IItem& subItem = item.createSubItem( vlcMedia->mrl(), i + 1u );
            mediaToItem( *vlcMedia, subItem );
        }
    }
}

}
}
