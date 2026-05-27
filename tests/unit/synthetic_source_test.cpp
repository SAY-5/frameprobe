#include "source/synthetic_source.h"

#include <gtest/gtest.h>

using namespace frameprobe;

TEST(SyntheticSource, ProducesExactFrameCount) {
    SyntheticConfig cfg;
    cfg.frames = 10;
    SyntheticSource src(cfg);
    int count = 0;
    while (auto f = src.next()) {
        EXPECT_EQ(f->index, static_cast<std::uint64_t>(count));
        EXPECT_EQ(f->width, cfg.width);
        EXPECT_EQ(f->height, cfg.height);
        EXPECT_EQ(f->byte_size(), static_cast<std::size_t>(cfg.width) * cfg.height * 3);
        ++count;
    }
    EXPECT_EQ(count, 10);
    EXPECT_FALSE(src.next().has_value());
}

TEST(SyntheticSource, IsDeterministicAcrossRuns) {
    SyntheticConfig cfg;
    cfg.frames = 5;
    SyntheticSource a(cfg);
    SyntheticSource b(cfg);
    for (int i = 0; i < 5; ++i) {
        auto fa = a.next();
        auto fb = b.next();
        ASSERT_TRUE(fa && fb);
        EXPECT_EQ(fa->pixels, fb->pixels);
    }
}

TEST(SyntheticSource, ShapeBoxesStayInBounds) {
    SyntheticConfig cfg;
    cfg.frames = 50;
    for (std::uint64_t idx = 0; idx < cfg.frames; ++idx) {
        for (int s = 0; s < cfg.num_shapes; ++s) {
            int x, y, w, h;
            SyntheticSource::shape_box(cfg, idx, s, x, y, w, h);
            EXPECT_GE(x, 0);
            EXPECT_GE(y, 0);
            EXPECT_LE(x + w, cfg.width);
            EXPECT_LE(y + h, cfg.height);
        }
    }
}
