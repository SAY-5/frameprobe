#include "metrics/latency_hist.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace frameprobe {

void LatencyHist::ensure_sorted() const {
    if (!sorted_) {
        std::sort(samples_.begin(), samples_.end());
        sorted_ = true;
    }
}

double LatencyHist::percentile(double p) const {
    if (samples_.empty())
        return 0.0;
    ensure_sorted();
    p = std::clamp(p, 0.0, 100.0);
    // Nearest-rank percentile.
    const double rank = (p / 100.0) * (static_cast<double>(samples_.size()) - 1.0);
    const std::size_t lo = static_cast<std::size_t>(std::floor(rank));
    const std::size_t hi = static_cast<std::size_t>(std::ceil(rank));
    if (lo == hi)
        return samples_[lo];
    const double frac = rank - static_cast<double>(lo);
    return samples_[lo] * (1.0 - frac) + samples_[hi] * frac;
}

double LatencyHist::mean() const {
    if (samples_.empty())
        return 0.0;
    const double sum = std::accumulate(samples_.begin(), samples_.end(), 0.0);
    return sum / static_cast<double>(samples_.size());
}

double LatencyHist::min() const {
    if (samples_.empty())
        return 0.0;
    ensure_sorted();
    return samples_.front();
}

double LatencyHist::max() const {
    if (samples_.empty())
        return 0.0;
    ensure_sorted();
    return samples_.back();
}

}  // namespace frameprobe
