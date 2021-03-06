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

#ifndef VLCMETADATASERVICE_H
#define VLCMETADATASERVICE_H

#include "compat/ConditionVariable.h"
#include <vlcpp/vlc.hpp>
#include <mutex>

#include "medialibrary/parser/IParserService.h"
#include "parser/Parser.h"
#include "AlbumTrack.h"

namespace medialibrary
{
namespace parser
{

class VLCMetadataService : public IParserService
{
    public:
        explicit VLCMetadataService();

private:
        virtual bool initialize( IMediaLibrary* ml ) override;
        virtual parser::Status run( parser::IItem& item ) override;
        virtual const char* name() const override;
        virtual uint8_t nbThreads() const override;
        virtual void onFlushing() override;
        virtual void onRestarted() override;
        virtual parser::Step targetedStep() const override;

        void mediaToItem( VLC::Media& media, parser::IItem& item );

private:
        VLC::Instance m_instance;
        compat::Mutex m_mutex;
        compat::ConditionVariable m_cond;
};

}
}

#endif // VLCMETADATASERVICE_H
