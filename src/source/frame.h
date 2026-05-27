#ifndef FRAMEPROBE_SOURCE_FRAME_H
#define FRAMEPROBE_SOURCE_FRAME_H

#include <chrono>
#include <cstdint>
#include <vector>

namespace frameprobe {

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;

// A single video frame. Pixels are stored as a contiguous BGR (3 channel)
// 8-bit buffer, row-major, identical in layout to a cv::Mat of type CV_8UC3.
// The core pipeline operates purely on this struct so it builds and tests
// without OpenCV; the OpenCV-backed sources fill the same buffer.
struct Frame {
    std::uint64_t index = 0;  // monotonically increasing frame number
    int width = 0;
    int height = 0;
    int channels = 3;
    std::vector<std::uint8_t> pixels;  // size == width*height*channels
    TimePoint arrival;                 // when the decoder produced the frame

    Frame() = default;
    Frame(std::uint64_t idx, int w, int h, int ch = 3)
        : index(idx),
          width(w),
          height(h),
          channels(ch),
          pixels(static_cast<std::size_t>(w) * h * ch, 0) {
    }

    std::size_t byte_size() const {
        return pixels.size();
    }

    std::uint8_t& at(int y, int x, int c) {
        return pixels[(static_cast<std::size_t>(y) * width + x) * channels + c];
    }
    std::uint8_t at(int y, int x, int c) const {
        return pixels[(static_cast<std::size_t>(y) * width + x) * channels + c];
    }
};

}  // namespace frameprobe

#endif  // FRAMEPROBE_SOURCE_FRAME_H
