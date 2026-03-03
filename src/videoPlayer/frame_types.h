#ifndef FRAME_TYPES_H
#define FRAME_TYPES_H

#include <cstdint>

extern "C"
{
#include <libavutil/mem.h>
}

struct AudioFrame
{
    uint8_t* data;
    int size;
    int64_t pts;
    int sample_rate;
    int samples;
    
    AudioFrame()
        : data(nullptr)
        , size(0)
        , pts(0)
        , sample_rate(0)
        , samples(0)
    {
    }
    
    ~AudioFrame()
    {
        if (data)
        {
            av_free(data);
        }
    }
};

struct VideoFrame
{
    uint8_t* data;
    int width;
    int height;
    int64_t pts;
    double display_time;
    
    VideoFrame()
        : data(nullptr)
        , width(0)
        , height(0)
        , pts(0)
        , display_time(0.0)
    {
    }
    
    ~VideoFrame()
    {
        if (data)
        {
            av_free(data);
        }
    }
};

#endif