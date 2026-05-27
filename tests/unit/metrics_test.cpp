#include <gtest/gtest.h>

#include "metrics/frame_stats.h"
#include "metrics/latency_hist.h"

using namespace frameprobe;

TEST(LatencyHist, PercentilesOnKnownDistribution) {
    LatencyHist h;
    for (int i = 1; i <= 100; ++i)
        h.record_us(static_cast<double>(i));
    EXPECT_EQ(h.count(), 100u);
    EXPECT_NEAR(h.percentile(50), 50.5, 1.0);
    EXPECT_NEAR(h.percentile(95), 95.05, 1.0);
    EXPECT_NEAR(h.percentile(99), 99.01, 1.0);
    EXPECT_DOUBLE_EQ(h.min(), 1.0);
    EXPECT_DOUBLE_EQ(h.max(), 100.0);
    EXPECT_NEAR(h.mean(), 50.5, 1e-9);
}

TEST(LatencyHist, EmptyIsZero) {
    LatencyHist h;
    EXPECT_EQ(h.count(), 0u);
    EXPECT_DOUBLE_EQ(h.percentile(99), 0.0);
}

TEST(FrameStats, DeadlineMissAccounting) {
    FrameStats s;
    s.set_deadline_us(1000.0);
    s.record_processed(500.0, 0.9f);   // met
    s.record_processed(800.0, 0.8f);   // met
    s.record_processed(1500.0, 0.7f);  // missed
    s.record_processed(2000.0, 0.6f);  // missed
    EXPECT_EQ(s.processed(), 4u);
    EXPECT_EQ(s.deadline_misses(), 2u);
    EXPECT_DOUBLE_EQ(s.miss_rate(), 0.5);
    EXPECT_NEAR(s.mean_confidence(), 0.75, 1e-6);
}

TEST(FrameStats, SustainedFpsFromWallTime) {
    FrameStats s;
    for (int i = 0; i < 300; ++i)
        s.record_processed(1000.0, 0.5f);
    s.set_wall_us(10e6);  // 10 seconds
    EXPECT_NEAR(s.sustained_fps(), 30.0, 1e-6);
}

TEST(FrameStats, ReportIsWellFormed) {
    FrameStats s;
    s.set_deadline_us(33333.0);
    s.record_processed(1000.0, 0.9f);
    s.set_wall_us(1e6);
    const std::string r = s.report();
    EXPECT_NE(r.find("sustained_fps="), std::string::npos);
    EXPECT_NE(r.find("latency_p99_us="), std::string::npos);
    EXPECT_NE(r.find("miss_rate="), std::string::npos);
    EXPECT_NE(r.find("mean_confidence="), std::string::npos);
}

TEST(FrameStats, SkipAndDropCounters) {
    FrameStats s;
    s.record_skipped();
    s.record_skipped();
    s.record_dropped();
    EXPECT_EQ(s.skipped(), 2u);
    EXPECT_EQ(s.dropped(), 1u);
}
