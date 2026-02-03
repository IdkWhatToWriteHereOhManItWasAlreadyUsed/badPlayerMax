#include "demuxer.h"
#include <iostream>

void demuxer_thread_func(AVFormatContext* format_ctx, int video_stream_index,
    int audio_stream_index, AVRational video_time_base,
    AVRational audio_time_base, std::shared_ptr<SharedData> shared)
{
    while (shared->demuxer_running)
    {
        std::shared_ptr<AVPacket> packet(av_packet_alloc(), [](AVPacket* p)
        {
            av_packet_unref(p);
            av_packet_free(&p);
        });
        
        if (!packet)
        {
            std::cerr << "Failed to allocate packet in demuxer" << std::endl;
            break;
        }
        
        int ret = av_read_frame(format_ctx, packet.get());
        if (ret < 0)
        {
            if (ret == AVERROR_EOF)
            {
                {
                    std::lock_guard<std::mutex> lock(shared->packet_mutex);
                    if (video_stream_index != -1)
                    {
                        shared->video_packets.push(nullptr);
                    }
                    if (audio_stream_index != -1)
                    {
                        shared->audio_packets.push(nullptr);
                    }
                    shared->packet_cv.notify_all();
                }
                break;
            }
            continue;
        }
        
        std::unique_lock<std::mutex> lock(shared->packet_mutex);
        
        if (packet->stream_index == video_stream_index && video_stream_index != -1)
        {
            while (shared->video_packets.size() >= shared->MAX_PACKET_QUEUE_SIZE &&
                shared->demuxer_running)
            {
                shared->packet_cv.wait(lock);
            }
            
            if (!shared->demuxer_running)
                break;
            
            shared->video_packets.push(packet);
            shared->packet_cv.notify_all();
        }
        else if (packet->stream_index == audio_stream_index && audio_stream_index != -1)
        {
            while (shared->audio_packets.size() >= shared->MAX_PACKET_QUEUE_SIZE &&
                shared->demuxer_running)
            {
                shared->packet_cv.wait(lock);
            }
            
            if (!shared->demuxer_running)
                break;
            
            shared->audio_packets.push(packet);
            shared->packet_cv.notify_all();
        }
    }
    
    shared->demuxer_running = false;
    shared->packet_cv.notify_all();
}