#include "detect/stub_detector.h"

#include <gtest/gtest.h>

#include "source/synthetic_source.h"

using namespace frameprobe;

namespace {
StubConfig make_cfg(std::uint32_t compute_us = 0) {
    StubConfig sc;
    sc.stream.frames = 20;
    sc.compute_us = compute_us;
    return sc;
}
}  // namespace

TEST(StubDetector, DetectionsMatchScriptedGroundTruth) {
    StubConfig sc = make_cfg();
    StubDetector det(sc);
    SyntheticSource src(sc.stream);
    while (auto f = src.next()) {
        DetectionResult got = det.detect(*f);
        DetectionResult expected = StubDetector::scripted(sc, f->index);
        EXPECT_EQ(got.frame_index, expected.frame_index);
        ASSERT_EQ(got.detections.size(), expected.detections.size());
        for (std::size_t i = 0; i < got.detections.size(); ++i) {
            EXPECT_EQ(got.detections[i], expected.detections[i]);
        }
    }
}

TEST(StubDetector, BoxesMatchDrawnShapes) {
    StubConfig sc = make_cfg();
    DetectionResult r = StubDetector::scripted(sc, 7);
    ASSERT_EQ(r.detections.size(), static_cast<std::size_t>(sc.stream.num_shapes));
    for (int s = 0; s < sc.stream.num_shapes; ++s) {
        int x, y, w, h;
        SyntheticSource::shape_box(sc.stream, 7, s, x, y, w, h);
        EXPECT_EQ(r.detections[s].x, x);
        EXPECT_EQ(r.detections[s].y, y);
        EXPECT_EQ(r.detections[s].w, w);
        EXPECT_EQ(r.detections[s].h, h);
    }
}

TEST(StubDetector, ConfidenceInExpectedRange) {
    StubConfig sc = make_cfg();
    for (std::uint64_t i = 0; i < 100; ++i) {
        DetectionResult r = StubDetector::scripted(sc, i);
        for (const auto& d : r.detections) {
            EXPECT_GE(d.confidence, 0.5f);
            EXPECT_LT(d.confidence, 1.0f);
        }
    }
}

TEST(StubDetector, ComputeCostAddsLatency) {
    StubConfig sc = make_cfg(2000);  // 2ms
    StubDetector det(sc);
    SyntheticSource src(sc.stream);
    auto f = src.next();
    ASSERT_TRUE(f);
    const auto t0 = Clock::now();
    det.detect(*f);
    const auto t1 = Clock::now();
    const double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
    EXPECT_GE(us, 1500.0);  // allow scheduling slack below the 2000us target
}
