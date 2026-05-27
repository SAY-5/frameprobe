#include "source/synthetic_source.h"

#include <algorithm>

namespace frameprobe {

namespace {

void fill_rect(Frame& f, int x, int y, int w, int h, std::uint8_t b, std::uint8_t g,
               std::uint8_t r) {
    const int x1 = std::clamp(x + w, 0, f.width);
    const int y1 = std::clamp(y + h, 0, f.height);
    const int x0 = std::clamp(x, 0, f.width);
    const int y0 = std::clamp(y, 0, f.height);
    for (int yy = y0; yy < y1; ++yy) {
        for (int xx = x0; xx < x1; ++xx) {
            f.at(yy, xx, 0) = b;
            f.at(yy, xx, 1) = g;
            f.at(yy, xx, 2) = r;
        }
    }
}

}  // namespace

SyntheticSource::SyntheticSource(SyntheticConfig cfg) : cfg_(cfg) {
}

void SyntheticSource::shape_box(const SyntheticConfig& cfg, std::uint64_t idx, int s, int& x,
                                int& y, int& w, int& h) {
    // Each shape has a fixed size and a per-frame linear motion that wraps around
    // the frame. Purely a function of seed, shape index and frame index.
    const std::uint32_t base = cfg.seed * 2654435761u + static_cast<std::uint32_t>(s) * 40503u;
    w = 24 + static_cast<int>(base % 16);
    h = 24 + static_cast<int>((base >> 4) % 16);
    const int step_x = 1 + static_cast<int>(base % 3);
    const int step_y = 1 + static_cast<int>((base >> 2) % 3);
    const int span_x = std::max(1, cfg.width - w);
    const int span_y = std::max(1, cfg.height - h);
    x = static_cast<int>((static_cast<std::uint64_t>(base % span_x) + idx * step_x) % span_x);
    y = static_cast<int>((static_cast<std::uint64_t>((base >> 8) % span_y) + idx * step_y) %
                         span_y);
}

std::optional<Frame> SyntheticSource::next() {
    if (produced_ >= cfg_.frames)
        return std::nullopt;
    const std::uint64_t idx = produced_++;
    Frame f(idx, cfg_.width, cfg_.height);

    // Background gradient keyed off the frame index so the buffer is non-trivial.
    const std::uint8_t bg = static_cast<std::uint8_t>(16 + (idx % 32));
    std::fill(f.pixels.begin(), f.pixels.end(), bg);

    for (int s = 0; s < cfg_.num_shapes; ++s) {
        int x, y, w, h;
        shape_box(cfg_, idx, s, x, y, w, h);
        const auto color = static_cast<std::uint8_t>(64 + s * 48);
        fill_rect(f, x, y, w, h, color, static_cast<std::uint8_t>(255 - color), color);
    }

    f.arrival = Clock::now();
    return f;
}

}  // namespace frameprobe
