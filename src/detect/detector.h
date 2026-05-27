#ifndef FRAMEPROBE_DETECT_DETECTOR_H
#define FRAMEPROBE_DETECT_DETECTOR_H

#include <cstdint>
#include <string>
#include <vector>

#include "source/frame.h"

namespace frameprobe {

// One detected object in a frame, in pixel coordinates.
struct Detection {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int class_id = 0;
    float confidence = 0.0f;

    bool operator==(const Detection& o) const {
        return x == o.x && y == o.y && w == o.w && h == o.h && class_id == o.class_id &&
               confidence == o.confidence;
    }
};

// The full inference result for a single frame.
struct DetectionResult {
    std::uint64_t frame_index = 0;
    std::vector<Detection> detections;

    float max_confidence() const {
        float m = 0.0f;
        for (const auto& d : detections) {
            if (d.confidence > m)
                m = d.confidence;
        }
        return m;
    }
};

// Detector interface. Implementations must be deterministic in their detection
// output for a given frame so the pipeline is testable, while still allowing a
// configurable compute cost to model real inference latency.
class Detector {
  public:
    virtual ~Detector() = default;
    virtual DetectionResult detect(const Frame& frame) = 0;
    virtual std::string name() const = 0;
};

}  // namespace frameprobe

#endif  // FRAMEPROBE_DETECT_DETECTOR_H
