#ifndef FRAMEPROBE_PIPELINE_ADAPTIVE_H
#define FRAMEPROBE_PIPELINE_ADAPTIVE_H

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace frameprobe {

// Proportional frame-skip controller (v4). The decoder consults it for each
// frame. The controller watches the live frame-queue depth: the deeper the
// backlog past a high-watermark, the larger the skip stride K, so it processes
// roughly every Kth frame to catch up, and returns to full rate (K=1) once the
// backlog drains.
//
// Skip decision is driven only by queue depth, which the decoder reads from the
// bounded queue (its own synchronized counter), so the controller itself holds
// no shared mutable pipeline state beyond an atomic stride for reporting.
class AdaptiveController {
  public:
    AdaptiveController(std::size_t high_watermark, std::size_t capacity, int max_skip)
        : high_watermark_(high_watermark == 0 ? std::max<std::size_t>(1, capacity / 4)
                                              : high_watermark),
          capacity_(capacity == 0 ? 1 : capacity),
          max_skip_(std::max(1, max_skip)) {
    }

    // Returns true if the frame at `index` should be skipped, given the current
    // queue depth. Updates the reported stride.
    bool should_skip(std::uint64_t index, std::size_t queue_depth) {
        const int stride = stride_for(queue_depth);
        stride_.store(stride, std::memory_order_relaxed);
        if (stride <= 1)
            return false;
        // Process frame 0 of every stride window, skip the rest.
        return (index % static_cast<std::uint64_t>(stride)) != 0;
    }

    int current_stride() const {
        return stride_.load(std::memory_order_relaxed);
    }

    // Proportional law: below the high-watermark, stride 1 (process all). Above
    // it, stride grows linearly with how far depth has pushed past the
    // watermark toward capacity, clamped to max_skip.
    int stride_for(std::size_t queue_depth) const {
        if (queue_depth <= high_watermark_)
            return 1;
        const double over = static_cast<double>(queue_depth - high_watermark_);
        const double span = std::max(1.0, static_cast<double>(capacity_ - high_watermark_));
        const double frac = std::min(1.0, over / span);
        const int stride = 1 + static_cast<int>(frac * static_cast<double>(max_skip_ - 1) + 0.5);
        return std::clamp(stride, 1, max_skip_);
    }

  private:
    const std::size_t high_watermark_;
    const std::size_t capacity_;
    const int max_skip_;
    std::atomic<int> stride_{1};
};

}  // namespace frameprobe

#endif  // FRAMEPROBE_PIPELINE_ADAPTIVE_H
