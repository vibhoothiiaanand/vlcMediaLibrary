#ifndef TYPES_H
#define TYPES_H

#include <memory>

class IAlbum;
class IAlbumTrack;
class IAudioTrack;
class IDiscoverer;
class IFile;
class IFolder;
class ILabel;
class IMetadataService;
class IMovie;
class IShow;
class IShowEpisode;
class IVideoTrack;
class ILogger;

struct sqlite3;

typedef std::shared_ptr<IFile> FilePtr;
typedef std::shared_ptr<IFolder> FolderPtr;
typedef std::shared_ptr<ILabel> LabelPtr;
typedef std::shared_ptr<IAlbum> AlbumPtr;
typedef std::shared_ptr<IAlbumTrack> AlbumTrackPtr;
typedef std::shared_ptr<IShow> ShowPtr;
typedef std::shared_ptr<IShowEpisode> ShowEpisodePtr;
typedef std::shared_ptr<IMovie> MoviePtr;
typedef std::shared_ptr<IAudioTrack> AudioTrackPtr;
typedef std::shared_ptr<IVideoTrack> VideoTrackPtr;

typedef std::weak_ptr<sqlite3> DBConnection;

enum ServiceStatus
{
    /// All good
    StatusSuccess,
    /// Something failed, but it's not critical (For instance, no internet connection for a
    /// module that uses an online database)
    StatusError,
    /// We can't compute this file for now (for instance the file was on a network drive which
    /// isn't connected anymore)
    StatusTemporaryUnavailable,
    /// Something failed and we won't continue
    StatusFatal
};

#endif // TYPES_H
