#ifndef FRAMEPROBE_SOURCE_VIDEO_SOURCE_H
#define FRAMEPROBE_SOURCE_VIDEO_SOURCE_H

#ifdef FRAMEPROBE_WITH_OPENCV

#include <memory>
#include <string>

#include "source/source.h"

namespace cv {
class VideoCapture;
}

namespace frameprobe {

// Reads frames from a file or device via cv::VideoCapture and copies each into a
// FrameProbe Frame (BGR CV_8UC3). Only built when FRAMEPROBE_WITH_OPENCV is set.
class VideoSource : public FrameSource {
  public:
    explicit VideoSource(const std::string& path);
    ~VideoSource() override;

    std::optional<Frame> next() override;
    int width() const override {
        return width_;
    }
    int height() const override {
        return height_;
    }

  private:
    std::unique_ptr<cv::VideoCapture> cap_;
    int width_ = 0;
    int height_ = 0;
    std::uint64_t produced_ = 0;
};

}  // namespace frameprobe

#endif  // FRAMEPROBE_WITH_OPENCV
#endif  // FRAMEPROBE_SOURCE_VIDEO_SOURCE_H
