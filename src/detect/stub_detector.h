#ifndef FRAMEPROBE_DETECT_STUB_DETECTOR_H
#define FRAMEPROBE_DETECT_STUB_DETECTOR_H

#include <chrono>
#include <cstdint>

#include "detect/detector.h"
#include "source/synthetic_source.h"

namespace frameprobe {

struct StubConfig {
    SyntheticConfig stream;  // the stream geometry this stub is scripted against
    // Deterministic per-frame compute cost. The detector spins on a busy loop for
    // `compute_us` microseconds so inference latency is real and tunable without
    // depending on a model. Spinning (not sleeping) keeps it CPU-bound like real
    // inference and visible to ThreadSanitizer scheduling.
    std::uint32_t compute_us = 0;
};

// A deterministic detector: it returns exactly the boxes the SyntheticSource
// drew for each frame, with a confidence derived from the frame index. Output is
// a pure function of the frame index, so detections are bit-identical across
// runs and thread counts.
class StubDetector : public Detector {
  public:
    explicit StubDetector(StubConfig cfg);

    DetectionResult detect(const Frame& frame) override;
    std::string name() const override {
        return "stub";
    }

    // Pure scripted result for a frame index, with no compute cost. Tests use
    // this as the deterministic oracle.
    static DetectionResult scripted(const StubConfig& cfg, std::uint64_t index);

  private:
    StubConfig cfg_;
};

}  // namespace frameprobe

#endif  // FRAMEPROBE_DETECT_STUB_DETECTOR_H
