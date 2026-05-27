#ifndef FRAMEPROBE_DETECT_DNN_DETECTOR_H
#define FRAMEPROBE_DETECT_DNN_DETECTOR_H

#ifdef FRAMEPROBE_WITH_OPENCV

#include <opencv2/dnn.hpp>
#include <string>

#include "detect/detector.h"

namespace frameprobe {

// Real object detector backed by OpenCV's DNN module. Loads an ONNX model and
// runs a forward pass per frame. Gated behind FRAMEPROBE_WITH_OPENCV and a model
// path the operator supplies (see models/README.md); the CI default is the
// StubDetector so the build stays hermetic.
struct DnnConfig {
    std::string model_path;
    float conf_threshold = 0.5f;
    int input_size = 300;
};

class DnnDetector : public Detector {
  public:
    explicit DnnDetector(DnnConfig cfg);
    ~DnnDetector() override;

    DetectionResult detect(const Frame& frame) override;
    std::string name() const override {
        return "dnn";
    }

  private:
    DnnConfig cfg_;
    cv::dnn::Net net_;
};

}  // namespace frameprobe

#endif  // FRAMEPROBE_WITH_OPENCV
#endif  // FRAMEPROBE_DETECT_DNN_DETECTOR_H
