#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include "shared_data.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <SDL2/SDL.h>
}

bool initialize_audio(AVFormatContext* format_ctx, int audio_stream_index,
    AVCodecContext*& audio_codec_ctx, std::shared_ptr<SharedData> shared);

void decode_audio(AVCodecContext* audio_codec_ctx, AVRational audio_time_base,
    std::shared_ptr<SharedData> shared);

void cleanup_audio();

#endif