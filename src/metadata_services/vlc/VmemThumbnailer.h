/*****************************************************************************
 * Media Library
 *****************************************************************************
 * Copyright (C) 2015-2018 Hugo Beauzée-Luyssen, Videolabs, VideoLAN
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

#include "imagecompressors/IImageCompressor.h"
#include "compat/ConditionVariable.h"
#include "medialibrary/Types.h"
#include "medialibrary/IThumbnailer.h"
#include "Types.h"

#include <vlcpp/vlc.hpp>

#include <atomic>

namespace medialibrary
{

class VmemThumbnailer : public IThumbnailer
{
    struct Task
    {
        Task( MediaPtr media, std::string mrl );

        compat::Mutex mutex;
        compat::ConditionVariable cond;
        MediaPtr media;
        std::string mrl;
        uint32_t width;
        uint32_t height;
        VLC::MediaPlayer mp;
        std::atomic_bool thumbnailRequired;
    };

public:
    VmemThumbnailer( MediaLibraryPtr ml );
    virtual bool generate( MediaPtr media, const std::string& mrl ) override;
    bool seekAhead( Task& task );
    void setupVout( Task& task );
    bool takeThumbnail( Task& task );
    bool compress( Task& task );

private:
    // Force a base width, let height be computed depending on A/R
    static const uint32_t DesiredWidth = 320;
    static const uint32_t DesiredHeight = 200; // Aim for a 16:10 thumbnail

private:
    MediaLibraryPtr m_ml;
    std::unique_ptr<uint8_t[]> m_buff;
    uint32_t m_prevSize;
    std::unique_ptr<IImageCompressor> m_compressor;
};

}
