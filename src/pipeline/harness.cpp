#include "pipeline/harness.h"

#include <algorithm>
#include <memory>
#include <thread>
#include <vector>

#include "pipeline/adaptive.h"
#include "pipeline/decoder.h"
#include "pipeline/inference.h"
#include "pipeline/sink.h"

namespace frameprobe {

namespace {

// Each inference worker needs its own detector instance (detectors are not
// required to be thread safe). For the StubDetector we can clone cheaply; for a
// single worker we reuse the harness-owned detector directly. Since the harness
// only knows the abstract Detector, we run the single supplied detector when
// workers==1 and otherwise require the caller-provided detector to be safe to
// share. The Stub/DNN detectors used here are stateless across detect() calls
// except for their config, so sharing read-only config across workers is safe.

}  // namespace

Harness::Harness(std::unique_ptr<FrameSource> source, std::unique_ptr<Detector> detector,
                 PipelineConfig cfg)
    : source_(std::move(source)), detector_(std::move(detector)), cfg_(cfg) {
}

FrameStats Harness::run() {
    FrameStats stats;
    stats.set_deadline_us(cfg_.deadline_us());

    BoundedQueue<FrameEnvelope> frame_q(cfg_.queue_capacity, cfg_.overflow);
    BoundedQueue<ResultEnvelope> result_q(cfg_.queue_capacity * 2, Overflow::kBlock);

    Decoder decoder(*source_, frame_q);

    std::unique_ptr<AdaptiveController> controller;
    if (cfg_.adaptive) {
        controller = std::make_unique<AdaptiveController>(cfg_.adapt_high_watermark,
                                                          cfg_.queue_capacity, cfg_.adapt_max_skip);
        decoder.set_skip_decider([&controller, &frame_q](std::uint64_t index) {
            return controller->should_skip(index, frame_q.size());
        });
    }

    Sink sink(result_q, stats);
    sink.set_queue_depth_probe([&frame_q]() { return frame_q.size(); });

    const auto start = Clock::now();

    std::thread decoder_thread([&decoder] { decoder.run(); });

    const int n_workers = std::max(1, cfg_.inference_workers);
    std::vector<InferenceWorker> workers;
    workers.reserve(n_workers);
    for (int i = 0; i < n_workers; ++i) {
        workers.emplace_back(frame_q, result_q, *detector_);
    }
    std::vector<std::thread> worker_threads;
    worker_threads.reserve(n_workers);
    for (auto& w : workers) {
        worker_threads.emplace_back([&w] { w.run(); });
    }

    std::thread sink_thread([&sink] { sink.run(); });

    decoder_thread.join();
    for (auto& t : worker_threads)
        t.join();
    // All inference workers are done; no more results will be produced.
    result_q.close();
    sink_thread.join();

    const auto end = Clock::now();
    stats.set_wall_us(std::chrono::duration<double, std::micro>(end - start).count());
    return stats;
}

}  // namespace frameprobe
