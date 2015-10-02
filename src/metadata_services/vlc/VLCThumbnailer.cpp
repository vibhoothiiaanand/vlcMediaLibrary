#include "VLCThumbnailer.h"

#include <cstring>
#include <jpeglib.h>

#include "IFile.h"
#include "logging/Logger.h"

VLCThumbnailer::VLCThumbnailer(const VLC::Instance &vlc)
    : m_instance( vlc )
    , m_snapshotRequired( false )
    , m_height( 0 )
{
}

bool VLCThumbnailer::initialize(IMetadataServiceCb *callback, IMediaLibrary *ml)
{
    m_cb = callback;
    m_ml = ml;
    return true;
}

unsigned int VLCThumbnailer::priority() const
{
    // This needs to be lower than the VLCMetadataService, since we want to know the file type.
    return 50;
}

bool VLCThumbnailer::run( FilePtr file, void *data )
{
    if ( file->type() == IFile::Type::UnknownType )
    {
        // If we don't know the file type yet, it actually looks more like a bug
        // since this should run after file type deduction, and not run in case
        // that step fails.
        m_cb->done( file, StatusError, data );
        return false;
    }
    else if ( file->type() != IFile::Type::VideoType )
    {
        // There's no point in generating a thumbnail for a non-video file.
        m_cb->done( file, StatusSuccess, data );
        return true;
    }

    VLC::Media media( m_instance, file->mrl(), VLC::Media::FromPath );
    media.addOption( ":no-audio" );
    VLC::MediaPlayer mp( media );

    setupVout( mp );

    if ( startPlayback( file, mp, data ) == false )
        return false;

    // Seek ahead to have a significant preview
    if ( seekAhead( file, mp, data ) == false )
        return false;

    if ( takeSnapshot( file, mp, data ) == false )
        return false;
    return true;
}

bool VLCThumbnailer::startPlayback( FilePtr file, VLC::MediaPlayer &mp, void* data )
{
    bool failed = true;

    std::unique_lock<std::mutex> lock( m_mutex );

    mp.eventManager().onPlaying([this, &failed]() {
        std::unique_lock<std::mutex> lock( m_mutex );
        failed = false;
        m_cond.notify_all();
    });
    mp.eventManager().onEncounteredError([this]() {
        std::unique_lock<std::mutex> lock( m_mutex );
        m_cond.notify_all();
    });
    mp.play();
    bool success = m_cond.wait_for( lock, std::chrono::seconds( 3 ), [&mp]() {
        return mp.state() == libvlc_Playing || mp.state() == libvlc_Error;
    });
    if ( success == false || failed == true )
    {
        // In case of timeout or error, don't go any further
        m_cb->done( file, StatusError, data );
        return false;
    }
    return true;
}

bool VLCThumbnailer::seekAhead( FilePtr file, VLC::MediaPlayer& mp, void* data )
{
    std::unique_lock<std::mutex> lock( m_mutex );
    float pos = .0f;
    auto event = mp.eventManager().onPositionChanged([this, &pos](float p) {
        std::unique_lock<std::mutex> lock( m_mutex );
        pos = p;
        m_cond.notify_all();
    });
    mp.setPosition( .4f );
    bool success = m_cond.wait_for( lock, std::chrono::seconds( 3 ), [&pos]() {
        return pos >= .1f;
    });
    // Since we're locking a mutex for each position changed, let's unregister ASAP
    event->unregister();
    if ( success == false )
    {
        m_cb->done( file, StatusError, data );
        return false;
    }
    return true;
}

void VLCThumbnailer::setupVout( VLC::MediaPlayer& mp )
{
    mp.setVideoFormatCallbacks(
        // Setup
        [this, &mp](char* chroma, unsigned int* width, unsigned int *height, unsigned int *pitches, unsigned int *lines) {
            strcpy( chroma, "RV32" );

            const float inputAR = (float)*width / *height;

            *width = Width;
            auto prevHeight = m_height;
            m_height = (float)Width / inputAR + 1;
            // If our buffer isn't enough anymore, reallocate a new one.
            if ( m_height > prevHeight )
                m_buff.reset( new uint8_t[Width * m_height * Bpp] );
            *height = m_height;
            *pitches = Width * Bpp;
            *lines = m_height;
            return 1;
        },
        // Cleanup
        nullptr);
    mp.setVideoCallbacks(
        // Lock
        [this](void** pp_buff) {
            *pp_buff = m_buff.get();
            return nullptr;
        },
        //unlock
        [this](void*, void*const*) {
            std::unique_lock<std::mutex> lock(m_mutex);
            if ( m_snapshotRequired == true )
            {
                m_snapshotRequired = false;
                m_cond.notify_all();
            }
        }
        ,
        //display
        nullptr
    );
}

bool VLCThumbnailer::takeSnapshot(FilePtr file, VLC::MediaPlayer &mp, void *data)
{
    std::unique_ptr<uint8_t[]> buff;
    // lock, signal that we want a snapshot, and wait.
    {
        std::unique_lock<std::mutex> lock( m_mutex );
        m_snapshotRequired = true;
        bool success = m_cond.wait_for( lock, std::chrono::seconds( 3 ), [this]() {
            // Keep waiting if the vmem thread hasn't restored m_snapshotRequired to false
            return m_snapshotRequired == true;
        });
        if ( success == false )
        {
            m_cb->done( file, StatusError, data );
            return false;
        }
        // Prevent the vmem from screwing our snapshot over.
        buff = std::move( m_buff );
        // set a new buffer for next snapshot, and to avoid crashing while we stop the media player
        m_buff.reset( new uint8_t[Width * m_height * Bpp] );
    }
    mp.stop();
    return compress( buff.get(), file, data );
}

bool VLCThumbnailer::compress(uint8_t* buff, FilePtr file, void *data)
{
    auto path = m_ml->snapshotPath();
    path += "/";
    path += std::to_string( file->id() ) + ".jpg";

#ifdef WITH_JPEG
    jpeg_compress_struct compInfo;
    jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];

    compInfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&compInfo);

    //FIXME: Abstract this away, though libjpeg requires a FILE*...
    auto fOut = fopen(path.c_str(), "wb");
    // ensure we always close the file.
    auto fOutPtr = std::unique_ptr<FILE, int(*)(FILE*)>( fOut, &fclose );
    if ( fOut == nullptr )
    {
        LOG_ERROR("Failed to open snapshot file ", path);
        m_cb->done( file, StatusError, data );
        return false;
    }
    jpeg_stdio_dest(&compInfo, fOut);

    compInfo.image_width = Width;
    compInfo.image_height = m_height;
    compInfo.input_components = Bpp;
    compInfo.in_color_space = JCS_EXT_BGRX;
    jpeg_set_defaults( &compInfo );
    jpeg_set_quality( &compInfo, 85, TRUE );

    jpeg_start_compress( &compInfo, TRUE );

    auto stride = compInfo.image_width * Bpp;

    while (compInfo.next_scanline < compInfo.image_height) {
      row_pointer[0] = &buff[compInfo.next_scanline * stride];
      jpeg_write_scanlines(&compInfo, row_pointer, 1);
    }
    jpeg_finish_compress(&compInfo);
    jpeg_destroy_compress(&compInfo);
#else
#error FIXME
#endif

    m_cb->done( file, StatusSuccess, data );
    file->setSnapshot( path );
    return true;
}
