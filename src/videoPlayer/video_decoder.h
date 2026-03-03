#ifndef VIDEO_DECODER_H
#define VIDEO_DECODER_H

#include "shared_data.h"

extern "C"
{
#include <libavcodec/avcodec.h>
}

void decode_video(AVCodecContext* video_codec_ctx, AVRational video_time_base,
    std::shared_ptr<SharedData> shared);

#endif