#include "source/video_source.h"

#ifdef FRAMEPROBE_WITH_OPENCV

#include <opencv2/opencv.hpp>
#include <stdexcept>

namespace frameprobe {

VideoSource::VideoSource(const std::string& path) : cap_(std::make_unique<cv::VideoCapture>(path)) {
    if (!cap_->isOpened()) {
        throw std::runtime_error("VideoSource: cannot open " + path);
    }
    width_ = static_cast<int>(cap_->get(cv::CAP_PROP_FRAME_WIDTH));
    height_ = static_cast<int>(cap_->get(cv::CAP_PROP_FRAME_HEIGHT));
}

VideoSource::~VideoSource() = default;

std::optional<Frame> VideoSource::next() {
    cv::Mat mat;
    if (!cap_->read(mat) || mat.empty())
        return std::nullopt;
    if (mat.type() != CV_8UC3) {
        cv::cvtColor(mat, mat, cv::COLOR_GRAY2BGR);
    }
    Frame f(produced_++, mat.cols, mat.rows);
    if (mat.isContinuous()) {
        std::memcpy(f.pixels.data(), mat.data, f.pixels.size());
    } else {
        for (int y = 0; y < mat.rows; ++y) {
            std::memcpy(f.pixels.data() + static_cast<std::size_t>(y) * mat.cols * 3, mat.ptr(y),
                        static_cast<std::size_t>(mat.cols) * 3);
        }
    }
    f.arrival = Clock::now();
    return f;
}

}  // namespace frameprobe

#endif  // FRAMEPROBE_WITH_OPENCV
