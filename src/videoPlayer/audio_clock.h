#ifndef AUDIO_CLOCK_H
#define AUDIO_CLOCK_H

#include <atomic>

extern "C"
{
#include <libavutil/time.h>
}

class AudioClock
{
private:
    std::atomic<double> current_pts_{0.0};
    std::atomic<int64_t> last_update_{0};
    std::atomic<double> speed_{1.0};

public:
    void update(int64_t pts, double time_base, int samples_played, int sample_rate);
    double get_time();
    void set_speed(double speed);
    double get_speed() const;
};

#endif