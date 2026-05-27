#ifndef FRAMEPROBE_SOURCE_SYNTHETIC_SOURCE_H
#define FRAMEPROBE_SOURCE_SYNTHETIC_SOURCE_H

#include <cstdint>

#include "source/source.h"

namespace frameprobe {

// Generates a deterministic synthetic stream: a fixed number of frames, each a
// solid background with N rectangles whose positions advance by a fixed step
// per frame. Geometry is a pure function of (seed, frame index), so the stream
// is bit-identical across runs. This is the ground truth the StubDetector is
// scripted against.
struct SyntheticConfig {
    int width = 320;
    int height = 240;
    std::uint64_t frames = 300;
    int num_shapes = 2;
    std::uint32_t seed = 1;
};

class SyntheticSource : public FrameSource {
  public:
    explicit SyntheticSource(SyntheticConfig cfg);

    std::optional<Frame> next() override;
    int width() const override {
        return cfg_.width;
    }
    int height() const override {
        return cfg_.height;
    }

    // Deterministic ground-truth box for shape `s` at frame `idx`. The
    // StubDetector reuses this so detections match the drawn shapes exactly.
    static void shape_box(const SyntheticConfig& cfg, std::uint64_t idx, int s, int& x, int& y,
                          int& w, int& h);

  private:
    SyntheticConfig cfg_;
    std::uint64_t produced_ = 0;
};

}  // namespace frameprobe

#endif  // FRAMEPROBE_SOURCE_SYNTHETIC_SOURCE_H
