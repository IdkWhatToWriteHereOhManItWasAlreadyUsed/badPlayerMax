
#include <chrono>
#include <memory>
#include <thread>
#include <videoPlayer/media_player.h>
#include <iostream>
#include "Globals.h"

void VideoPlayerFunc()
{
    std::string video_path ;//= "video.mp4";
    std::cout << "Enter video file path: " << std::endl;
    std::getline(std::cin, video_path);
    MediaPlayer player;
    if (!player.initialize(video_path))
    {
        return;
    }
    player.run();
    player.cleanup();
}

int main()
{
    GLobal::shouldStop = false;
    std::thread th([] {VideoPlayerFunc();});
    GLobal::frameDisplayer = std::make_unique<OpenGLSomethingFrameDisplayerEVO>();

    GLobal::frameDisplayer->WaitForSetVideoSize();

    GLobal::frameDisplayer->InitialiseGame(1300, 900);

    GLobal::frameDisplayer->Start();
    GLobal::shouldStop = true;
    th.join();
    return 0;
}