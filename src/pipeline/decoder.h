#ifndef FRAMEPROBE_PIPELINE_DECODER_H
#define FRAMEPROBE_PIPELINE_DECODER_H

#include <functional>

#include "pipeline/queue.h"
#include "source/frame.h"
#include "source/source.h"

namespace frameprobe {

// A frame as it travels from decoder to inference: the pixels plus a flag set by
// the adaptive controller telling inference to skip the model for this frame.
struct FrameEnvelope {
    Frame frame;
    bool skip = false;
};

// The decode stage. Pulls frames from the source and pushes them onto the frame
// queue, marking frames to skip when the adaptive controller says so. Runs until
// the source is exhausted, then closes the queue. All metric updates happen at
// the single-threaded sink to keep the data path race-free.
class Decoder {
  public:
    // Given the next frame index, returns true if the frame should be skipped
    // (decoded and forwarded but not run through the model).
    using SkipDecider = std::function<bool(std::uint64_t index)>;

    Decoder(FrameSource& source, BoundedQueue<FrameEnvelope>& out) : source_(source), out_(out) {
    }

    void set_skip_decider(SkipDecider decider) {
        skip_decider_ = std::move(decider);
    }

    void run();

  private:
    FrameSource& source_;
    BoundedQueue<FrameEnvelope>& out_;
    SkipDecider skip_decider_;
};

}  // namespace frameprobe

#endif  // FRAMEPROBE_PIPELINE_DECODER_H
