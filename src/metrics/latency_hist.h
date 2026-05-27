#ifndef FRAMEPROBE_METRICS_LATENCY_HIST_H
#define FRAMEPROBE_METRICS_LATENCY_HIST_H

#include <cstdint>
#include <vector>

namespace frameprobe {

// A simple sorted-sample latency tracker. Samples are recorded in microseconds;
// percentiles are computed by sorting on demand. Sample counts here are bounded
// (a few thousand frames per run) so an exact sort beats an approximate
// histogram and keeps the percentile assertions in tests exact.
class LatencyHist {
  public:
    void record_us(double us) {
        samples_.push_back(us);
    }

    std::size_t count() const {
        return samples_.size();
    }
    double percentile(double p) const;  // p in [0,100]
    double mean() const;
    double min() const;
    double max() const;

  private:
    mutable std::vector<double> samples_;
    mutable bool sorted_ = false;
    void ensure_sorted() const;
};

}  // namespace frameprobe

#endif  // FRAMEPROBE_METRICS_LATENCY_HIST_H
