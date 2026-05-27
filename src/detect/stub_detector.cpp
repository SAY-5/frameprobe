#include "detect/stub_detector.h"

namespace frameprobe {

StubDetector::StubDetector(StubConfig cfg) : cfg_(cfg) {
}

DetectionResult StubDetector::scripted(const StubConfig& cfg, std::uint64_t index) {
    DetectionResult result;
    result.frame_index = index;
    result.detections.reserve(cfg.stream.num_shapes);
    for (int s = 0; s < cfg.stream.num_shapes; ++s) {
        int x, y, w, h;
        SyntheticSource::shape_box(cfg.stream, index, s, x, y, w, h);
        Detection d;
        d.x = x;
        d.y = y;
        d.w = w;
        d.h = h;
        d.class_id = s;
        // Confidence is a fixed function of (index, shape) in [0.5, 1.0).
        const std::uint32_t mix =
            static_cast<std::uint32_t>(index) * 2246822519u + static_cast<std::uint32_t>(s) * 7u;
        d.confidence = 0.5f + static_cast<float>(mix % 500) / 1000.0f;
        result.detections.push_back(d);
    }
    return result;
}

DetectionResult StubDetector::detect(const Frame& frame) {
    // Burn the configured compute budget with a side-effecting spin so the
    // compiler cannot optimize it away.
    if (cfg_.compute_us > 0) {
        const auto deadline = Clock::now() + std::chrono::microseconds(cfg_.compute_us);
        volatile std::uint64_t acc = frame.index;
        while (Clock::now() < deadline) {
            acc = acc * 6364136223846793005ull + 1442695040888963407ull;
        }
        (void)acc;
    }
    return scripted(cfg_, frame.index);
}

}  // namespace frameprobe
