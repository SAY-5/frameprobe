#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

#include "detect/stub_detector.h"
#include "pipeline/harness.h"
#include "source/synthetic_source.h"

#ifdef FRAMEPROBE_WITH_OPENCV
#include "detect/dnn_detector.h"
#include "source/video_source.h"
#endif

namespace {

using namespace frameprobe;

struct Args {
    std::string source = "synthetic";
    std::string video_path;
    std::string detector = "stub";
    std::string model_path;
    double fps = 30.0;
    std::uint64_t frames = 300;
    std::size_t queue = 16;
    std::string backpressure = "block";
    std::uint32_t compute_us = 0;
    int workers = 1;
    bool adaptive = false;
    int width = 320;
    int height = 240;
    int shapes = 2;
    std::uint32_t seed = 1;
};

[[noreturn]] void usage(int code) {
    std::cout << "frameprobe --source synthetic|video [options]\n"
                 "  --source <synthetic|video>   frame source (default synthetic)\n"
                 "  --video <path>               input file for --source video\n"
                 "  --detector <stub|dnn>        detector (default stub)\n"
                 "  --model <path>               ONNX model for --detector dnn\n"
                 "  --fps <n>                    target frame rate / deadline (default 30)\n"
                 "  --frames <n>                 synthetic frame count (default 300)\n"
                 "  --queue <n>                  frame queue capacity (default 16)\n"
                 "  --backpressure <block|drop>  overflow policy (default block)\n"
                 "  --compute-us <n>             stub per-frame compute cost (default 0)\n"
                 "  --workers <n>                inference worker threads (default 1)\n"
                 "  --adaptive                   enable adaptive frame-skipping\n"
                 "  --width/--height/--shapes/--seed   synthetic stream geometry\n";
    std::exit(code);
}

const char* next_value(int argc, char** argv, int& i) {
    if (i + 1 >= argc)
        usage(2);
    return argv[++i];
}

Args parse(int argc, char** argv) {
    Args a;
    for (int i = 1; i < argc; ++i) {
        const std::string opt = argv[i];
        if (opt == "--source")
            a.source = next_value(argc, argv, i);
        else if (opt == "--video")
            a.video_path = next_value(argc, argv, i);
        else if (opt == "--detector")
            a.detector = next_value(argc, argv, i);
        else if (opt == "--model")
            a.model_path = next_value(argc, argv, i);
        else if (opt == "--fps")
            a.fps = std::strtod(next_value(argc, argv, i), nullptr);
        else if (opt == "--frames")
            a.frames = std::strtoull(next_value(argc, argv, i), nullptr, 10);
        else if (opt == "--queue")
            a.queue = std::strtoull(next_value(argc, argv, i), nullptr, 10);
        else if (opt == "--backpressure")
            a.backpressure = next_value(argc, argv, i);
        else if (opt == "--compute-us")
            a.compute_us =
                static_cast<std::uint32_t>(std::strtoul(next_value(argc, argv, i), nullptr, 10));
        else if (opt == "--workers")
            a.workers = static_cast<int>(std::strtol(next_value(argc, argv, i), nullptr, 10));
        else if (opt == "--adaptive")
            a.adaptive = true;
        else if (opt == "--width")
            a.width = static_cast<int>(std::strtol(next_value(argc, argv, i), nullptr, 10));
        else if (opt == "--height")
            a.height = static_cast<int>(std::strtol(next_value(argc, argv, i), nullptr, 10));
        else if (opt == "--shapes")
            a.shapes = static_cast<int>(std::strtol(next_value(argc, argv, i), nullptr, 10));
        else if (opt == "--seed")
            a.seed =
                static_cast<std::uint32_t>(std::strtoul(next_value(argc, argv, i), nullptr, 10));
        else if (opt == "-h" || opt == "--help")
            usage(0);
        else {
            std::cerr << "unknown option: " << opt << "\n";
            usage(2);
        }
    }
    return a;
}

}  // namespace

int main(int argc, char** argv) {
    Args a = parse(argc, argv);

    SyntheticConfig stream;
    stream.width = a.width;
    stream.height = a.height;
    stream.frames = a.frames;
    stream.num_shapes = a.shapes;
    stream.seed = a.seed;

    std::unique_ptr<FrameSource> source;
    if (a.source == "synthetic") {
        source = std::make_unique<SyntheticSource>(stream);
    } else if (a.source == "video") {
#ifdef FRAMEPROBE_WITH_OPENCV
        source = std::make_unique<VideoSource>(a.video_path);
#else
        std::cerr << "video source requires building with -DFRAMEPROBE_WITH_OPENCV=ON\n";
        return 2;
#endif
    } else {
        std::cerr << "unknown source: " << a.source << "\n";
        return 2;
    }

    std::unique_ptr<Detector> detector;
    if (a.detector == "stub") {
        StubConfig sc;
        sc.stream = stream;
        sc.compute_us = a.compute_us;
        detector = std::make_unique<StubDetector>(sc);
    } else if (a.detector == "dnn") {
#ifdef FRAMEPROBE_WITH_OPENCV
        DnnConfig dc;
        dc.model_path = a.model_path;
        detector = std::make_unique<DnnDetector>(dc);
#else
        std::cerr << "dnn detector requires building with -DFRAMEPROBE_WITH_OPENCV=ON\n";
        return 2;
#endif
    } else {
        std::cerr << "unknown detector: " << a.detector << "\n";
        return 2;
    }

    PipelineConfig cfg;
    cfg.target_fps = a.fps;
    cfg.queue_capacity = a.queue;
    cfg.overflow = (a.backpressure == "drop") ? Overflow::kDrop : Overflow::kBlock;
    cfg.inference_workers = a.workers;
    cfg.adaptive = a.adaptive;

    Harness harness(std::move(source), std::move(detector), cfg);
    FrameStats stats = harness.run();
    std::cout << stats.report();
    return 0;
}
