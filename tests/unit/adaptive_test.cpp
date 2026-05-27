#include "pipeline/adaptive.h"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <thread>

#include "detect/stub_detector.h"
#include "pipeline/harness.h"
#include "source/synthetic_source.h"

using namespace frameprobe;

// The control law: at or below the high-watermark the stride is 1 (process every
// frame); past it the stride grows with depth up to max_skip; and it returns to
// 1 once depth drops back, which is exactly the recovery behaviour.
TEST(Adaptive, StrideGrowsWithBacklogAndRecovers) {
    AdaptiveController c(/*high_watermark=*/4, /*capacity=*/16, /*max_skip=*/8);

    EXPECT_EQ(c.stride_for(0), 1);
    EXPECT_EQ(c.stride_for(4), 1);                 // at watermark, still full rate
    EXPECT_GT(c.stride_for(8), 1);                 // backlog -> skipping
    EXPECT_GT(c.stride_for(16), c.stride_for(8));  // deeper backlog -> more skipping
    EXPECT_LE(c.stride_for(16), 8);                // clamped to max_skip
    // Recovery: once the queue drains back under the watermark, full rate.
    EXPECT_EQ(c.stride_for(2), 1);
}

TEST(Adaptive, FullRateWhenNeverBehind) {
    AdaptiveController c(4, 16, 8);
    for (std::uint64_t i = 0; i < 100; ++i) {
        EXPECT_FALSE(c.should_skip(i, /*queue_depth=*/0));
    }
    EXPECT_EQ(c.current_stride(), 1);
}

TEST(Adaptive, SkipsProportionallyUnderSustainedPressure) {
    AdaptiveController c(4, 16, 8);
    int skipped = 0;
    const int n = 800;
    for (std::uint64_t i = 0; i < static_cast<std::uint64_t>(n); ++i) {
        if (c.should_skip(i, /*queue_depth=*/16))
            ++skipped;  // pinned at capacity
    }
    // At full backlog the stride is max_skip (8), so it processes ~1/8 and skips
    // ~7/8.
    EXPECT_GT(skipped, n * 6 / 8);
    EXPECT_LT(skipped, n);
}

// End to end: feed frames far faster than the detector can process at full rate.
// Without adaptation the processed frames miss their deadline almost always;
// with adaptation the harness skips frames so the processed ones hold the
// deadline, and the skip count is reported.
TEST(Adaptive, HoldsDeadlineOnProcessedFramesUnderOverload) {
    auto run = [](bool adaptive) {
        StubConfig sc;
        sc.stream.frames = 800;
        sc.compute_us = 300;
        PipelineConfig cfg;
        cfg.target_fps = 500.0;  // 2 ms deadline, well above the 300 us compute
        cfg.queue_capacity = 16;
        cfg.adaptive = adaptive;
        Harness h(std::make_unique<SyntheticSource>(sc.stream), std::make_unique<StubDetector>(sc),
                  cfg);
        return h.run();
    };

    FrameStats base = run(false);
    FrameStats adapt = run(true);

    // Baseline is overloaded: nearly every processed frame misses the deadline.
    EXPECT_GT(base.miss_rate(), 0.8);
    EXPECT_EQ(base.skipped(), 0u);

    // Adaptive skips frames (reported distinctly) and holds the deadline for the
    // processed ones far better than the baseline.
    EXPECT_GT(adapt.skipped(), 0u);
    EXPECT_LT(adapt.miss_rate(), 0.40);
    EXPECT_LT(adapt.miss_rate(), base.miss_rate());
    // Every frame is accounted for as either processed or skipped.
    EXPECT_EQ(adapt.processed() + adapt.skipped(), 800u);
}

namespace {

// A source that paces frame production: it sleeps `delay_us` between frames so
// the decoder cannot outrun the detector. Used to model an input that arrives
// slower than the detector can process, where the controller should never skip.
class PacedSource : public FrameSource {
  public:
    PacedSource(SyntheticConfig cfg, std::uint32_t delay_us) : inner_(cfg), delay_us_(delay_us) {
    }
    std::optional<Frame> next() override {
        std::this_thread::sleep_for(std::chrono::microseconds(delay_us_));
        return inner_.next();
    }
    int width() const override {
        return inner_.width();
    }
    int height() const override {
        return inner_.height();
    }

  private:
    SyntheticSource inner_;
    std::uint32_t delay_us_;
};

}  // namespace

// End to end recovery: when frames arrive slower than the detector processes
// them (paced source, tiny compute), the queue never builds up, so the
// controller stays at full rate and skips nothing.
TEST(Adaptive, NoSkipWhenInputArrivesSlowly) {
    SyntheticConfig stream;
    stream.frames = 200;
    StubConfig sc;
    sc.stream = stream;
    sc.compute_us = 5;  // far faster than the input arrives

    PipelineConfig cfg;
    cfg.target_fps = 100.0;  // 10 ms deadline
    cfg.queue_capacity = 16;
    cfg.adaptive = true;
    Harness h(std::make_unique<PacedSource>(stream, /*delay_us=*/200),
              std::make_unique<StubDetector>(sc), cfg);
    FrameStats s = h.run();
    EXPECT_EQ(s.processed(), stream.frames);
    EXPECT_EQ(s.skipped(), 0u);
}
