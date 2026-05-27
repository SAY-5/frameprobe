#ifndef FRAMEPROBE_PIPELINE_SINK_H
#define FRAMEPROBE_PIPELINE_SINK_H

#include <cstdint>
#include <functional>
#include <map>

#include "metrics/frame_stats.h"
#include "pipeline/harness.h"
#include "pipeline/queue.h"

namespace frameprobe {

// The metrics/sink stage, single-threaded. Pulls result envelopes (which may
// arrive out of frame order when several inference workers run), reorders them
// into strict frame order with a reorder buffer keyed on frame index, and
// records per-frame latency, confidence, deadline outcome, and skip/queue
// telemetry into FrameStats.
class Sink {
  public:
    // emit is called once per frame in strict frame order (tests use it to
    // assert reorder correctness). It may be null.
    using EmitFn = std::function<void(const ResultEnvelope&)>;

    Sink(BoundedQueue<ResultEnvelope>& in, FrameStats& stats) : in_(in), stats_(stats) {
    }

    void set_emit(EmitFn fn) {
        emit_ = std::move(fn);
    }
    // Lets the harness expose live queue depth so the sink can record the peak.
    void set_queue_depth_probe(std::function<std::size_t()> probe) {
        depth_probe_ = std::move(probe);
    }

    void run();

  private:
    void flush_ready();

    BoundedQueue<ResultEnvelope>& in_;
    FrameStats& stats_;
    EmitFn emit_;
    std::function<std::size_t()> depth_probe_;
    std::map<std::uint64_t, ResultEnvelope> pending_;
    std::uint64_t next_expected_ = 0;
};

}  // namespace frameprobe

#endif  // FRAMEPROBE_PIPELINE_SINK_H
