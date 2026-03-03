#ifndef MEDIA_PLAYER_H
#define MEDIA_PLAYER_H

#include <memory>
#include <string>

class MediaPlayer
{
public:
    MediaPlayer();
    ~MediaPlayer();
    
    bool initialize(const std::string& video_path);
    void run();
    void cleanup();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

#endif