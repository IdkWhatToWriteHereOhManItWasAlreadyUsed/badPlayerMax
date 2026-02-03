#include "audio_decoder.h"
#include "shared_data.h"
#include <iostream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/time.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}


void audio_callback(void* userdata, Uint8* stream, int len);

bool initialize_audio(AVFormatContext* format_ctx, int audio_stream_index,
    AVCodecContext*& audio_codec_ctx, std::shared_ptr<SharedData> shared)
{
    if (SDL_Init(SDL_INIT_AUDIO) < 0)
    {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    AVCodecParameters* audio_codec_params = format_ctx->streams[audio_stream_index]->codecpar;
    const AVCodec* audio_codec = avcodec_find_decoder(audio_codec_params->codec_id);
    
    if (!audio_codec)
    {
        std::cerr << "Unsupported audio codec" << std::endl;
        return false;
    }
    
    audio_codec_ctx = avcodec_alloc_context3(audio_codec);
    if (!audio_codec_ctx)
    {
        std::cerr << "Could not allocate audio codec context" << std::endl;
        return false;
    }
    
    if (avcodec_parameters_to_context(audio_codec_ctx, audio_codec_params) < 0)
    {
        std::cerr << "Could not copy audio codec parameters" << std::endl;
        avcodec_free_context(&audio_codec_ctx);
        return false;
    }
    
    if (avcodec_open2(audio_codec_ctx, audio_codec, nullptr) < 0)
    {
        std::cerr << "Could not open audio codec" << std::endl;
        avcodec_free_context(&audio_codec_ctx);
        return false;
    }
    
    SDL_AudioSpec wanted_spec, obtained_spec;
    
    wanted_spec.freq = 48000;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = 2;
    wanted_spec.samples = 4096;
    wanted_spec.callback = audio_callback;
    wanted_spec.userdata = shared.get();
    
    if (SDL_OpenAudio(&wanted_spec, &obtained_spec) < 0)
    {
        std::cerr << "Failed to open audio device: " << SDL_GetError() << std::endl;
        avcodec_free_context(&audio_codec_ctx);
        return false;
    }
    
    SDL_PauseAudio(0);
    return true;
}

void audio_callback(void* userdata, Uint8* stream, int len)
{
    SharedData* shared = static_cast<SharedData*>(userdata);
    std::unique_lock<std::mutex> lock(shared->audio_mutex);
    
    if (shared->audio_queue.empty())
    {
        memset(stream, 0, len);
        return;
    }
    
    int filled = 0;
    int total_samples_played = 0;
    
    while (filled < len && !shared->audio_queue.empty())
    {
        auto& frame = shared->audio_queue.front();
        
        int bytes_per_sample = 2 * sizeof(int16_t);
        int samples_in_chunk = std::min(frame->size - filled, len - filled) / bytes_per_sample;
        
        int to_copy = std::min(frame->size, len - filled);
        memcpy(stream + filled, frame->data, to_copy);
        filled += to_copy;
        total_samples_played += samples_in_chunk;
        
        if (to_copy < frame->size)
        {
            auto new_frame = std::make_shared<AudioFrame>();
            new_frame->size = frame->size - to_copy;
            new_frame->data = (uint8_t*)av_malloc(new_frame->size);
            new_frame->pts = frame->pts;
            new_frame->sample_rate = frame->sample_rate;
            new_frame->samples = frame->samples - samples_in_chunk;
            memcpy(new_frame->data, frame->data + to_copy, new_frame->size);
            
            frame = new_frame;
        }
        else
        {
            shared->audio_queue.pop();
            shared->audio_cv.notify_one();
            
            shared->audio_samples_played_ += frame->samples;
        }
    }
    
    if (filled < len)
    {
        memset(stream + filled, 0, len - filled);
    }
}

void decode_audio(AVCodecContext* audio_codec_ctx, AVRational audio_time_base,
    std::shared_ptr<SharedData> shared)
{
    AVFrame* frame = av_frame_alloc();
    if (!frame)
    {
        std::cerr << "Failed to allocate audio frame" << std::endl;
        return;
    }
    
    SwrContext* swr_ctx = swr_alloc();
    if (!swr_ctx)
    {
        std::cerr << "Failed to allocate swresample context" << std::endl;
        av_frame_free(&frame);
        return;
    }
    
    // ЗДЕСЬ ИСПРАВЛЕНИЕ: используем новый API для FFmpeg 5.0+
    // Создаем выходной layout для стерео
    AVChannelLayout out_ch_layout;
    av_channel_layout_default(&out_ch_layout, 2); // 2 канала = стерео
    
    // Настраиваем swresample с помощью новой функции
    int ret = swr_alloc_set_opts2(&swr_ctx,
                                 &out_ch_layout,                // выходной layout (стерео)
                                 AV_SAMPLE_FMT_S16,             // выходной формат
                                 48000,                         // выходная частота
                                 &audio_codec_ctx->ch_layout,   // входной layout
                                 audio_codec_ctx->sample_fmt,   // входной формат
                                 audio_codec_ctx->sample_rate,  // входная частота
                                 0,                             // log offset
                                 NULL);                         // log context
    
    if (ret < 0)
    {
        std::cerr << "Failed to set swresample options" << std::endl;
        swr_free(&swr_ctx);
        av_frame_free(&frame);
        return;
    }
    
    // УДАЛИТЬ СТАРОЕ:
    /*
    av_opt_set_int(swr_ctx, "in_channel_count", audio_codec_ctx->ch_layout.nb_channels, 0);
    av_opt_set_int(swr_ctx, "out_channel_count", 2, 0);
    av_opt_set_int(swr_ctx, "in_channel_layout", audio_codec_ctx->ch_layout.u.mask, 0);
    av_opt_set_int(swr_ctx, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", audio_codec_ctx->sample_rate, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", 48000, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", audio_codec_ctx->sample_fmt, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    */
    
    if (swr_init(swr_ctx) < 0)
    {
        std::cerr << "Failed to initialize swresample" << std::endl;
        swr_free(&swr_ctx);
        av_frame_free(&frame);
        return;
    }
    const int output_sample_rate = 48000;
    
    while (shared->audio_running)
    {
        std::shared_ptr<AVPacket> packet;
        
        {
            std::unique_lock<std::mutex> lock(shared->packet_mutex);
            shared->packet_cv.wait(lock, [shared]()
            {
                return !shared->audio_packets.empty() || !shared->demuxer_running;
            });
            
            if (!shared->audio_running || (!shared->demuxer_running && shared->audio_packets.empty()))
            {
                break;
            }
            
            if (!shared->audio_packets.empty())
            {
                packet = shared->audio_packets.front();
                shared->audio_packets.pop();
                shared->packet_cv.notify_one();
            }
        }
        
        if (!packet)
        {
            avcodec_send_packet(audio_codec_ctx, nullptr);
        }
        else
        {
            int ret = avcodec_send_packet(audio_codec_ctx, packet.get());
            if (ret < 0)
            {
                continue;
            }
        }
        
        while (true)
        {
            int ret = avcodec_receive_frame(audio_codec_ctx, frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                break;
            }
            else if (ret < 0)
            {
                std::cerr << "Error during audio decoding" << std::endl;
                break;
            }
            
            int dst_nb_samples = av_rescale_rnd(
                swr_get_delay(swr_ctx, frame->sample_rate) + frame->nb_samples,
                output_sample_rate, frame->sample_rate, AV_ROUND_UP);
            
            uint8_t** dst_data = nullptr;
            int dst_linesize;
            ret = av_samples_alloc_array_and_samples(
                &dst_data, &dst_linesize, 2, dst_nb_samples, AV_SAMPLE_FMT_S16, 0);
                
            if (ret < 0)
            {
                std::cerr << "Failed to allocate audio samples" << std::endl;
                av_frame_unref(frame);
                continue;
            }
            
            int converted_samples = swr_convert(swr_ctx, dst_data, dst_nb_samples,
                (const uint8_t**)frame->data, frame->nb_samples);
                
            if (converted_samples > 0)
            {
                int data_size = av_samples_get_buffer_size(
                    nullptr, 2, converted_samples, AV_SAMPLE_FMT_S16, 1);
                    
                auto audio_frame = std::make_shared<AudioFrame>();
                audio_frame->data = (uint8_t*)av_malloc(data_size);
                audio_frame->size = data_size;
                audio_frame->pts = frame->pts;
                audio_frame->sample_rate = output_sample_rate;
                audio_frame->samples = converted_samples;
                memcpy(audio_frame->data, dst_data[0], data_size);
                
                {
                    std::unique_lock<std::mutex> lock(shared->audio_mutex);
                    shared->audio_cv.wait(lock, [shared]()
                    {
                        return shared->audio_queue.size() < shared->MAX_FRAME_QUEUE_SIZE ||
                            !shared->audio_running;
                    });
                    
                    if (!shared->audio_running)
                        break;
                    
                    shared->audio_queue.push(audio_frame);
                    
                    if (shared->last_audio_update_ == 0)
                    {
                        shared->audio_clock.update(frame->pts, av_q2d(audio_time_base), 0,
                            output_sample_rate);
                        shared->last_audio_update_ = av_gettime();
                    }
                }
            }
            
            if (dst_data)
            {
                av_freep(&dst_data[0]);
            }
            av_freep(&dst_data);
            
            av_frame_unref(frame);
        }
    }
    
    // Не забудьте освободить AVChannelLayout в конце
    av_channel_layout_uninit(&out_ch_layout);
    swr_free(&swr_ctx);
    av_frame_free(&frame);
}

void cleanup_audio()
{
    SDL_CloseAudio();
    SDL_Quit();
}