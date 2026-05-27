#include "detect/serialize.h"

#include <gtest/gtest.h>

using namespace frameprobe;

namespace {
DetectionResult sample() {
    DetectionResult r;
    r.frame_index = 42;
    r.detections.push_back({1, 2, 3, 4, 5, 0.75f});
    r.detections.push_back({10, 20, 30, 40, 1, 0.5f});
    return r;
}
}  // namespace

TEST(Serialize, RoundTrip) {
    DetectionResult r = sample();
    auto buf = wire::serialize(r);
    auto back = wire::deserialize(buf);
    ASSERT_TRUE(back.has_value());
    EXPECT_EQ(back->frame_index, r.frame_index);
    ASSERT_EQ(back->detections.size(), r.detections.size());
    for (std::size_t i = 0; i < r.detections.size(); ++i) {
        EXPECT_EQ(back->detections[i], r.detections[i]);
    }
}

TEST(Serialize, RejectsBadMagic) {
    auto buf = wire::serialize(sample());
    buf[0] ^= 0xFF;
    EXPECT_FALSE(wire::deserialize(buf).has_value());
}

TEST(Serialize, RejectsTruncation) {
    auto buf = wire::serialize(sample());
    buf.resize(buf.size() - 4);
    EXPECT_FALSE(wire::deserialize(buf).has_value());
}

TEST(Serialize, RejectsAbsurdCountWithoutOverAllocating) {
    // magic + frame_index + a count far larger than the buffer can hold.
    std::vector<std::uint8_t> buf;
    auto put32 = [&](std::uint32_t v) {
        for (int i = 0; i < 4; ++i)
            buf.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
    };
    put32(wire::kMagic);
    for (int i = 0; i < 8; ++i)
        buf.push_back(0);  // frame_index
    put32(0xFFFFFFFFu);    // huge count
    EXPECT_FALSE(wire::deserialize(buf).has_value());
}

TEST(Serialize, EmptyBufferRejected) {
    EXPECT_FALSE(wire::deserialize(nullptr, 0).has_value());
    std::vector<std::uint8_t> empty;
    EXPECT_FALSE(wire::deserialize(empty).has_value());
}
