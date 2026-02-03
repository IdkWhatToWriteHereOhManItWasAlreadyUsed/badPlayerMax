#include "media_player.h"
#include "audio_clock.h"
#include "shared_data.h"
#include "demuxer.h"
#include "audio_decoder.h"
#include "video_decoder.h"

#include <iostream>
#include <thread>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}

#include <Globals.h>

struct MediaPlayer::Impl
{
    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* video_codec_ctx = nullptr;
    AVCodecContext* audio_codec_ctx = nullptr;
    
    int video_stream_index = -1;
    int audio_stream_index = -1;
    
    AVRational video_time_base;
    AVRational audio_time_base;
    
    std::shared_ptr<SharedData> shared_data;
    
    std::thread demuxer_thread;
    std::thread video_thread;
    std::thread audio_thread;
    
    bool audio_initialized = false;
};

MediaPlayer::MediaPlayer()
    : impl_(std::make_unique<Impl>())
{
    impl_->shared_data = std::make_shared<SharedData>();
    avformat_network_init();
}

MediaPlayer::~MediaPlayer()
{
    cleanup();
    avformat_network_deinit();
}

bool MediaPlayer::initialize(const std::string& video_path)
{
    if (avformat_open_input(&impl_->format_ctx, video_path.c_str(), nullptr, nullptr) != 0)
    {
        std::cerr << "Could not open video file: " << video_path << std::endl;
        return false;
    }
    
    if (avformat_find_stream_info(impl_->format_ctx, nullptr) < 0)
    {
        std::cerr << "Could not find stream information" << std::endl;
        avformat_close_input(&impl_->format_ctx);
        return false;
    }
    
    for (unsigned int i = 0; i < impl_->format_ctx->nb_streams; i++)
    {
        AVStream* stream = impl_->format_ctx->streams[i];
        AVCodecParameters* codec_params = stream->codecpar;
        
        if (codec_params->codec_type == AVMEDIA_TYPE_VIDEO && impl_->video_stream_index == -1)
        {
            impl_->video_stream_index = i;
        }
        else if (codec_params->codec_type == AVMEDIA_TYPE_AUDIO && impl_->audio_stream_index == -1)
        {
            impl_->audio_stream_index = i;
        }
    }
    
    if (impl_->video_stream_index == -1)
    {
        std::cerr << "Could not find video stream" << std::endl;
        avformat_close_input(&impl_->format_ctx);
        return false;
    }
    
    AVCodecParameters* video_codec_params = impl_->format_ctx->streams[impl_->video_stream_index]->codecpar;
    const AVCodec* video_codec = avcodec_find_decoder(video_codec_params->codec_id);
    
    if (!video_codec)
    {
        std::cerr << "Unsupported video codec" << std::endl;
        avformat_close_input(&impl_->format_ctx);
        return false;
    }
    
    impl_->video_codec_ctx = avcodec_alloc_context3(video_codec);
    
    if (!impl_->video_codec_ctx)
    {
        std::cerr << "Could not allocate video codec context" << std::endl;
        avformat_close_input(&impl_->format_ctx);
        return false;
    }
    
    if (avcodec_parameters_to_context(impl_->video_codec_ctx, video_codec_params) < 0)
    {
        std::cerr << "Could not copy video codec parameters" << std::endl;
        avcodec_free_context(&impl_->video_codec_ctx);
        avformat_close_input(&impl_->format_ctx);
        return false;
    }
    
    if (avcodec_open2(impl_->video_codec_ctx, video_codec, nullptr) < 0)
    {
        std::cerr << "Could not open video codec" << std::endl;
        avcodec_free_context(&impl_->video_codec_ctx);
        avformat_close_input(&impl_->format_ctx);
        return false;
    }
    
    impl_->video_time_base = impl_->format_ctx->streams[impl_->video_stream_index]->time_base;
    
    if (impl_->audio_stream_index != -1)
    {
        impl_->audio_time_base = impl_->format_ctx->streams[impl_->audio_stream_index]->time_base;
    }
    
    GLobal::frameDisplayer->SetVideoSize(impl_->video_codec_ctx->width, impl_->video_codec_ctx->height);
    return true;
}

void MediaPlayer::run()
{
    GLobal::frameDisplayer->WaitForGameInit();

    /*
    uint8_t*  blackFrame [480*360*3];
    memset(blackFrame, 255, sizeof(blackFrame)); 
    auto temp = &blackFrame;
    GLobal::frameDisplayer->DisplayFrame((uint8_t**)&temp);   
    std::this_thread::sleep_for(std::chrono::milliseconds(15555));
    */
    impl_->demuxer_thread = std::thread(demuxer_thread_func, impl_->format_ctx,
        impl_->video_stream_index, impl_->audio_stream_index,
        impl_->video_time_base, impl_->audio_time_base, impl_->shared_data);
    
    if (impl_->audio_stream_index != -1)
    {
        impl_->audio_initialized = initialize_audio(impl_->format_ctx, impl_->audio_stream_index,
            impl_->audio_codec_ctx, impl_->shared_data);
        
        if (impl_->audio_initialized)
        {
            impl_->audio_thread = std::thread(decode_audio, impl_->audio_codec_ctx,
                impl_->audio_time_base, impl_->shared_data);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    impl_->video_thread = std::thread(decode_video, impl_->video_codec_ctx,
        impl_->video_time_base, impl_->shared_data);
    
    impl_->video_thread.join();
    
    impl_->shared_data->video_running = false;
    impl_->shared_data->audio_running = false;
    impl_->shared_data->demuxer_running = false;
    
    {
        std::lock_guard<std::mutex> lock(impl_->shared_data->packet_mutex);
        while (!impl_->shared_data->video_packets.empty())
        {
            impl_->shared_data->video_packets.pop();
        }
        while (!impl_->shared_data->audio_packets.empty())
        {
            impl_->shared_data->audio_packets.pop();
        }
        impl_->shared_data->packet_cv.notify_all();
    }
    
    if (impl_->audio_thread.joinable())
    {
        impl_->audio_thread.join();
    }
    
    if (impl_->demuxer_thread.joinable())
    {
        impl_->demuxer_thread.join();
    }
    
    {
        std::lock_guard<std::mutex> lock(impl_->shared_data->audio_mutex);
        while (!impl_->shared_data->audio_queue.empty())
        {
            impl_->shared_data->audio_queue.pop();
        }
    }
}

void MediaPlayer::cleanup()
{
    if (impl_->audio_initialized)
    {
        cleanup_audio();
    }
    
    if (impl_->audio_codec_ctx)
    {
        avcodec_free_context(&impl_->audio_codec_ctx);
    }
    
    if (impl_->video_codec_ctx)
    {
        avcodec_free_context(&impl_->video_codec_ctx);
    }
    
    if (impl_->format_ctx)
    {
        avformat_close_input(&impl_->format_ctx);
    }
}