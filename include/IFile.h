#ifndef IFILE_H
#define IFILE_H

#include <vector>
#include <memory>
#include "ITrackInformation.h"

class IAlbumTrack;
class ILabel;
class IShowEpisode;
class ITrackInformation;

class IFile
{
    public:
        virtual ~IFile() {}

        virtual unsigned int id() const = 0;
        virtual std::shared_ptr<IAlbumTrack> albumTrack() = 0;
        virtual unsigned int duration() = 0;
        virtual std::shared_ptr<IShowEpisode> showEpisode() = 0;
        virtual int playCount() = 0;
        virtual const std::string& mrl() = 0;
        virtual std::shared_ptr<ILabel> addLabel( const std::string& label ) = 0;
        virtual bool removeLabel( const std::shared_ptr<ILabel>& label ) = 0;
        virtual const std::vector<std::shared_ptr<ILabel>>& labels() = 0;
};

#endif // IFILE_H
