#ifndef FRAMEPROBE_METRICS_FRAME_STATS_H
#define FRAMEPROBE_METRICS_FRAME_STATS_H

#include <cstdint>
#include <string>

#include "metrics/latency_hist.h"

namespace frameprobe {

// Aggregates the per-frame outcomes the sink observes and produces the run
// report: sustained FPS, latency percentiles, deadline-miss rate, mean
// confidence, queue depth peak, and (v4) skip accounting.
class FrameStats {
  public:
    // deadline_us is the per-frame budget derived from the target FPS.
    void set_deadline_us(double deadline_us) {
        deadline_us_ = deadline_us;
    }
    double deadline_us() const {
        return deadline_us_;
    }

    // Record a processed frame: end-to-end latency and detection confidence.
    void record_processed(double latency_us, float confidence);
    void record_dropped() {
        dropped_++;
    }
    void record_skipped() {
        skipped_++;
    }
    void observe_queue_depth(std::size_t depth);

    // Marks the wall-clock window used to compute sustained FPS.
    void set_wall_us(double wall_us) {
        wall_us_ = wall_us;
    }

    std::uint64_t processed() const {
        return processed_;
    }
    std::uint64_t dropped() const {
        return dropped_;
    }
    std::uint64_t skipped() const {
        return skipped_;
    }
    std::uint64_t deadline_misses() const {
        return deadline_misses_;
    }
    double miss_rate() const;      // misses / processed
    double sustained_fps() const;  // processed / wall_seconds
    double mean_confidence() const;
    std::size_t peak_queue_depth() const {
        return peak_queue_depth_;
    }
    const LatencyHist& latency() const {
        return latency_;
    }

    // Stable, well-formed text report (also usable as a smoke check in tests).
    std::string report() const;

  private:
    LatencyHist latency_;
    double deadline_us_ = 0.0;
    double wall_us_ = 0.0;
    double conf_sum_ = 0.0;
    std::uint64_t processed_ = 0;
    std::uint64_t dropped_ = 0;
    std::uint64_t skipped_ = 0;
    std::uint64_t deadline_misses_ = 0;
    std::size_t peak_queue_depth_ = 0;
};

}  // namespace frameprobe

#endif  // FRAMEPROBE_METRICS_FRAME_STATS_H
