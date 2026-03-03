#ifndef DEMUXER_H
#define DEMUXER_H

#include "shared_data.h"

extern "C"
{
#include <libavformat/avformat.h>
}

void demuxer_thread_func(AVFormatContext* format_ctx, int video_stream_index,
    int audio_stream_index, AVRational video_time_base,
    AVRational audio_time_base, std::shared_ptr<SharedData> shared);

#endif