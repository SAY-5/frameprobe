#ifndef FRAMEPROBE_PIPELINE_HARNESS_H
#define FRAMEPROBE_PIPELINE_HARNESS_H

#include <cstdint>
#include <memory>
#include <string>

#include "detect/detector.h"
#include "metrics/frame_stats.h"
#include "pipeline/queue.h"
#include "source/frame.h"
#include "source/source.h"

namespace frameprobe {

// Envelope carrying a result plus the timing checkpoints used to compute
// end-to-end latency at the sink.
struct ResultEnvelope {
    DetectionResult result;
    TimePoint arrival;        // decoder produced the frame
    TimePoint inference_end;  // inference finished
    bool skipped = false;     // v4: produced but deliberately not inferred
};

struct PipelineConfig {
    double target_fps = 30.0;
    std::size_t queue_capacity = 16;
    Overflow overflow = Overflow::kBlock;
    int inference_workers = 1;  // v3
    bool adaptive = false;      // v4
    // v4 controller knobs.
    std::size_t adapt_high_watermark = 0;  // 0 -> derived from queue capacity
    int adapt_max_skip = 8;

    double deadline_us() const {
        return target_fps > 0.0 ? 1e6 / target_fps : 0.0;
    }
};

// Runs the full three-stage pipeline to completion and returns the collected
// stats. Decoder, inference worker(s) and sink each run on their own thread.
class Harness {
  public:
    Harness(std::unique_ptr<FrameSource> source, std::unique_ptr<Detector> detector,
            PipelineConfig cfg);

    FrameStats run();

  private:
    std::unique_ptr<FrameSource> source_;
    std::unique_ptr<Detector> detector_;
    PipelineConfig cfg_;
};

}  // namespace frameprobe

#endif  // FRAMEPROBE_PIPELINE_HARNESS_H
