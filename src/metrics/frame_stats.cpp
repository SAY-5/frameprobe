#include "metrics/frame_stats.h"

#include <iomanip>
#include <sstream>

namespace frameprobe {

void FrameStats::record_processed(double latency_us, float confidence) {
    latency_.record_us(latency_us);
    conf_sum_ += confidence;
    processed_++;
    if (deadline_us_ > 0.0 && latency_us > deadline_us_) {
        deadline_misses_++;
    }
}

void FrameStats::observe_queue_depth(std::size_t depth) {
    if (depth > peak_queue_depth_)
        peak_queue_depth_ = depth;
}

double FrameStats::miss_rate() const {
    if (processed_ == 0)
        return 0.0;
    return static_cast<double>(deadline_misses_) / static_cast<double>(processed_);
}

double FrameStats::sustained_fps() const {
    if (wall_us_ <= 0.0)
        return 0.0;
    return static_cast<double>(processed_) / (wall_us_ / 1e6);
}

double FrameStats::mean_confidence() const {
    if (processed_ == 0)
        return 0.0;
    return conf_sum_ / static_cast<double>(processed_);
}

std::string FrameStats::report() const {
    std::ostringstream os;
    os << std::fixed << std::setprecision(2);
    os << "frames_processed=" << processed_ << "\n";
    os << "frames_dropped=" << dropped_ << "\n";
    os << "frames_skipped=" << skipped_ << "\n";
    os << "sustained_fps=" << sustained_fps() << "\n";
    os << "deadline_us=" << deadline_us_ << "\n";
    os << "deadline_misses=" << deadline_misses_ << "\n";
    os << "miss_rate=" << miss_rate() << "\n";
    os << "latency_p50_us=" << latency_.percentile(50) << "\n";
    os << "latency_p95_us=" << latency_.percentile(95) << "\n";
    os << "latency_p99_us=" << latency_.percentile(99) << "\n";
    os << "latency_mean_us=" << latency_.mean() << "\n";
    os << "mean_confidence=" << mean_confidence() << "\n";
    os << "peak_queue_depth=" << peak_queue_depth_ << "\n";
    return os.str();
}

}  // namespace frameprobe
