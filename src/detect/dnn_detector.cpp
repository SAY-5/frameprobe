#include "detect/dnn_detector.h"

#ifdef FRAMEPROBE_WITH_OPENCV

#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>
#include <stdexcept>

namespace frameprobe {

DnnDetector::DnnDetector(DnnConfig cfg) : cfg_(std::move(cfg)) {
    net_ = cv::dnn::readNetFromONNX(cfg_.model_path);
    if (net_.empty()) {
        throw std::runtime_error("DnnDetector: failed to load model " + cfg_.model_path);
    }
}

DnnDetector::~DnnDetector() = default;

DetectionResult DnnDetector::detect(const Frame& frame) {
    DetectionResult result;
    result.frame_index = frame.index;

    cv::Mat mat(frame.height, frame.width, CV_8UC3, const_cast<std::uint8_t*>(frame.pixels.data()));
    cv::Mat blob =
        cv::dnn::blobFromImage(mat, 1.0 / 127.5, cv::Size(cfg_.input_size, cfg_.input_size),
                               cv::Scalar(127.5, 127.5, 127.5), true, false);
    net_.setInput(blob);
    cv::Mat out = net_.forward();

    // MobileNet-SSD style output: [1,1,N,7] -> [_, class, conf, x1,y1,x2,y2].
    const int n = out.size[2];
    const auto* data = reinterpret_cast<float*>(out.data);
    for (int i = 0; i < n; ++i) {
        const float conf = data[i * 7 + 2];
        if (conf < cfg_.conf_threshold)
            continue;
        Detection d;
        d.class_id = static_cast<int>(data[i * 7 + 1]);
        d.confidence = conf;
        d.x = static_cast<int>(data[i * 7 + 3] * frame.width);
        d.y = static_cast<int>(data[i * 7 + 4] * frame.height);
        d.w = static_cast<int>(data[i * 7 + 5] * frame.width) - d.x;
        d.h = static_cast<int>(data[i * 7 + 6] * frame.height) - d.y;
        result.detections.push_back(d);
    }
    return result;
}

}  // namespace frameprobe

#endif  // FRAMEPROBE_WITH_OPENCV
