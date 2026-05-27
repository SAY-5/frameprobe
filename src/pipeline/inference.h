#ifndef FRAMEPROBE_PIPELINE_INFERENCE_H
#define FRAMEPROBE_PIPELINE_INFERENCE_H

#include <atomic>

#include "detect/detector.h"
#include "pipeline/decoder.h"
#include "pipeline/harness.h"
#include "pipeline/queue.h"

namespace frameprobe {

// The inference stage. Pulls frame envelopes from the frame queue, runs the
// detector (unless the frame is flagged skip), timestamps inference end, and
// pushes a ResultEnvelope to the result queue.
//
// Multiple InferenceWorker instances can share the same input/output queues
// (v3); each holds its own Detector instance so detectors need not be thread
// safe. Workers finish out of order; the sink reorders.
class InferenceWorker {
  public:
    InferenceWorker(BoundedQueue<FrameEnvelope>& in, BoundedQueue<ResultEnvelope>& out,
                    Detector& detector)
        : in_(in), out_(out), detector_(detector) {
    }

    void run();

  private:
    BoundedQueue<FrameEnvelope>& in_;
    BoundedQueue<ResultEnvelope>& out_;
    Detector& detector_;
};

}  // namespace frameprobe

#endif  // FRAMEPROBE_PIPELINE_INFERENCE_H
