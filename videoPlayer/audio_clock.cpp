#include "audio_clock.h"
#include <algorithm>

void AudioClock::update(int64_t pts, double time_base, int samples_played, int sample_rate)
{
    double pts_seconds = pts * time_base;
    double played_seconds = static_cast<double>(samples_played) / sample_rate;
    
    current_pts_.store(pts_seconds + played_seconds);
    last_update_.store(av_gettime());
}

double AudioClock::get_time()
{
    int64_t now = av_gettime();
    int64_t elapsed = now - last_update_.load();
    double elapsed_seconds = static_cast<double>(elapsed) / 1000000.0 * speed_.load();
    
    return current_pts_.load() + elapsed_seconds;
}

void AudioClock::set_speed(double speed)
{
    speed_.store(std::clamp(speed, 0.5, 2.0));
}

double AudioClock::get_speed() const
{
    return speed_.load();
}