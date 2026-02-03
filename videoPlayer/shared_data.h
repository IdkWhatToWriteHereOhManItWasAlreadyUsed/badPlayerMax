#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include "audio_clock.h"
#include "frame_types.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

extern "C"
{
#include <libavcodec/avcodec.h>
}

struct SharedData
{
    std::mutex audio_mutex;
    std::queue<std::shared_ptr<AudioFrame>> audio_queue;
    std::condition_variable audio_cv;
    
    std::mutex video_mutex;
    std::queue<std::shared_ptr<VideoFrame>> video_queue;
    std::condition_variable video_cv;
    
    AudioClock audio_clock;
    
    std::atomic<bool> audio_running{true};
    std::atomic<bool> video_running{true};
    std::atomic<bool> demuxer_running{true};
    
    std::mutex packet_mutex;
    std::queue<std::shared_ptr<AVPacket>> video_packets;
    std::queue<std::shared_ptr<AVPacket>> audio_packets;
    std::condition_variable packet_cv;
    
    const size_t MAX_PACKET_QUEUE_SIZE = 100;
    const size_t MAX_FRAME_QUEUE_SIZE = 30;
    
    std::atomic<int64_t> audio_samples_played_{0};
    std::atomic<int64_t> last_audio_update_{0};
};

#endif