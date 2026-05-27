#include <gtest/gtest.h>

#include <memory>

#include "detect/stub_detector.h"
#include "pipeline/harness.h"
#include "source/synthetic_source.h"

using namespace frameprobe;

namespace {
StubConfig stub_for(std::uint64_t frames, std::uint32_t compute_us = 0) {
    StubConfig sc;
    sc.stream.frames = frames;
    sc.compute_us = compute_us;
    return sc;
}
}  // namespace

// Exercises the full three-thread pipeline under a small queue so back-pressure
// engages. Run under ThreadSanitizer in CI to catch queue/condvar races.
TEST(PipelineConcurrency, BlockModeProcessesEveryFrame) {
    const std::uint64_t frames = 500;
    StubConfig sc = stub_for(frames, 50);
    PipelineConfig cfg;
    cfg.target_fps = 1000.0;
    cfg.queue_capacity = 4;
    cfg.overflow = Overflow::kBlock;

    Harness h(std::make_unique<SyntheticSource>(sc.stream), std::make_unique<StubDetector>(sc),
              cfg);
    FrameStats s = h.run();
    EXPECT_EQ(s.processed(), frames);
    EXPECT_EQ(s.dropped(), 0u);
}

TEST(PipelineConcurrency, DropModeNeverProcessesMoreThanProduced) {
    const std::uint64_t frames = 500;
    StubConfig sc = stub_for(frames, 100);
    PipelineConfig cfg;
    cfg.target_fps = 1000.0;
    cfg.queue_capacity = 4;
    cfg.overflow = Overflow::kDrop;

    Harness h(std::make_unique<SyntheticSource>(sc.stream), std::make_unique<StubDetector>(sc),
              cfg);
    FrameStats s = h.run();
    EXPECT_LE(s.processed(), frames);
    EXPECT_GT(s.processed(), 0u);
}

TEST(PipelineConcurrency, ReportHasDeadlineFromFps) {
    StubConfig sc = stub_for(50);
    PipelineConfig cfg;
    cfg.target_fps = 30.0;
    Harness h(std::make_unique<SyntheticSource>(sc.stream), std::make_unique<StubDetector>(sc),
              cfg);
    FrameStats s = h.run();
    EXPECT_NEAR(s.deadline_us(), 1e6 / 30.0, 1.0);
}
