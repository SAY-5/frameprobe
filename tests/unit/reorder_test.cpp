#include <gtest/gtest.h>

#include <algorithm>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

#include "detect/stub_detector.h"
#include "metrics/frame_stats.h"
#include "pipeline/harness.h"
#include "pipeline/queue.h"
#include "pipeline/sink.h"
#include "source/synthetic_source.h"

using namespace frameprobe;

namespace {

ResultEnvelope make_result(std::uint64_t index) {
    ResultEnvelope env;
    env.result.frame_index = index;
    env.arrival = Clock::now();
    env.inference_end = env.arrival + std::chrono::microseconds(100);
    return env;
}

}  // namespace

// Reorder-buffer correctness: results pushed in an arbitrary (shuffled) order
// must be emitted by the sink in strict frame order.
TEST(Reorder, SinkEmitsStrictFrameOrderFromShuffledInput) {
    const std::uint64_t frames = 1000;
    std::vector<std::uint64_t> order(frames);
    std::iota(order.begin(), order.end(), 0);
    std::mt19937 rng(2024);
    std::shuffle(order.begin(), order.end(), rng);

    BoundedQueue<ResultEnvelope> q(frames + 1);
    for (std::uint64_t idx : order) {
        q.push(make_result(idx));
    }
    q.close();

    FrameStats stats;
    Sink sink(q, stats);
    std::vector<std::uint64_t> emitted;
    sink.set_emit([&](const ResultEnvelope& e) { emitted.push_back(e.result.frame_index); });
    sink.run();

    ASSERT_EQ(emitted.size(), frames);
    for (std::uint64_t i = 0; i < frames; ++i) {
        EXPECT_EQ(emitted[i], i) << "out of order at position " << i;
    }
}

// With a worker pushing results concurrently and out of order, the sink still
// emits strictly in order.
TEST(Reorder, ConcurrentOutOfOrderProducerStillOrdered) {
    const std::uint64_t frames = 2000;
    BoundedQueue<ResultEnvelope> q(64);

    std::thread producer([&] {
        // Emit in two interleaved streams (odds first within blocks) to force
        // out-of-order arrival at the sink.
        for (std::uint64_t base = 0; base < frames; base += 2) {
            if (base + 1 < frames)
                q.push(make_result(base + 1));
            q.push(make_result(base));
        }
        q.close();
    });

    FrameStats stats;
    Sink sink(q, stats);
    std::vector<std::uint64_t> emitted;
    sink.set_emit([&](const ResultEnvelope& e) { emitted.push_back(e.result.frame_index); });
    sink.run();
    producer.join();

    ASSERT_EQ(emitted.size(), frames);
    for (std::uint64_t i = 0; i < frames; ++i) {
        EXPECT_EQ(emitted[i], i);
    }
}

// With N inference workers, the full pipeline still processes every frame
// exactly once (no loss, no duplication) in block mode.
TEST(Reorder, MultiWorkerPipelineProcessesAllFrames) {
    for (int workers : {1, 2, 4, 8}) {
        StubConfig sc;
        sc.stream.frames = 800;
        sc.compute_us = 20;
        PipelineConfig cfg;
        cfg.target_fps = 1000.0;
        cfg.queue_capacity = 8;
        cfg.inference_workers = workers;
        Harness h(std::make_unique<SyntheticSource>(sc.stream), std::make_unique<StubDetector>(sc),
                  cfg);
        FrameStats s = h.run();
        EXPECT_EQ(s.processed(), sc.stream.frames) << "workers=" << workers;
        EXPECT_EQ(s.dropped(), 0u) << "workers=" << workers;
    }
}
