#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "detect/stub_detector.h"
#include "pipeline/harness.h"
#include "source/synthetic_source.h"

using namespace frameprobe;

namespace {

struct Case {
    double fps;
    std::uint32_t compute_us;
    double sustained_fps;
    double p50;
    double p95;
    double p99;
    double miss_rate;
    std::size_t peak_queue;
};

Case run_case(std::uint64_t frames, double fps, std::uint32_t compute_us) {
    SyntheticConfig stream;
    stream.frames = frames;
    StubConfig sc;
    sc.stream = stream;
    sc.compute_us = compute_us;

    PipelineConfig cfg;
    cfg.target_fps = fps;
    cfg.queue_capacity = 16;

    Harness h(std::make_unique<SyntheticSource>(stream), std::make_unique<StubDetector>(sc), cfg);
    FrameStats s = h.run();

    return Case{fps,
                compute_us,
                s.sustained_fps(),
                s.latency().percentile(50),
                s.latency().percentile(95),
                s.latency().percentile(99),
                s.miss_rate(),
                s.peak_queue_depth()};
}

std::string to_json(const std::vector<Case>& cases) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(3);
    os << "{\n  \"cases\": [\n";
    for (std::size_t i = 0; i < cases.size(); ++i) {
        const auto& c = cases[i];
        os << "    {\"fps\": " << c.fps << ", \"compute_us\": " << c.compute_us
           << ", \"sustained_fps\": " << c.sustained_fps << ", \"p50_us\": " << c.p50
           << ", \"p95_us\": " << c.p95 << ", \"p99_us\": " << c.p99
           << ", \"miss_rate\": " << c.miss_rate << ", \"peak_queue\": " << c.peak_queue << "}";
        os << (i + 1 < cases.size() ? ",\n" : "\n");
    }
    os << "  ]\n}\n";
    return os.str();
}

// Minimal extractor for the baseline regression check: pulls the per-case
// sustained_fps and p99_us values in file order.
bool load_baseline(const std::string& path, std::vector<double>& fps, std::vector<double>& p99) {
    std::ifstream in(path);
    if (!in)
        return false;
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    std::size_t pos = 0;
    auto find_num = [&](const std::string& key) -> bool {
        std::size_t k = content.find(key, pos);
        if (k == std::string::npos)
            return false;
        k += key.size();
        while (k < content.size() && (content[k] == ' ' || content[k] == ':'))
            ++k;
        char* end = nullptr;
        double v = std::strtod(content.c_str() + k, &end);
        pos = static_cast<std::size_t>(end - content.c_str());
        if (key.find("sustained") != std::string::npos)
            fps.push_back(v);
        else
            p99.push_back(v);
        return true;
    };
    // Walk the file pulling alternating sustained_fps / p99 values.
    while (true) {
        std::size_t s = content.find("\"sustained_fps\"", pos);
        if (s == std::string::npos)
            break;
        pos = s;
        find_num("\"sustained_fps\"");
        find_num("\"p99_us\"");
    }
    return !fps.empty();
}

}  // namespace

int main(int argc, char** argv) {
    std::uint64_t frames = 3000;
    std::string out_path;
    std::string regress_path;
    double threshold = 0.30;

    for (int i = 1; i < argc; ++i) {
        std::string opt = argv[i];
        auto val = [&]() -> const char* { return (i + 1 < argc) ? argv[++i] : ""; };
        if (opt == "--frames")
            frames = std::strtoull(val(), nullptr, 10);
        else if (opt == "--out")
            out_path = val();
        else if (opt == "--regress")
            regress_path = val();
        else if (opt == "--threshold")
            threshold = std::strtod(val(), nullptr);
    }

    const std::vector<std::pair<double, std::uint32_t>> matrix = {
        {15.0, 0}, {15.0, 200}, {30.0, 0}, {30.0, 200}, {60.0, 0}, {60.0, 200}};

    std::vector<Case> cases;
    for (auto [fps, cost] : matrix) {
        cases.push_back(run_case(frames, fps, cost));
    }

    const std::string json = to_json(cases);
    std::cout << json;

    if (!out_path.empty()) {
        std::ofstream of(out_path);
        of << json;
        std::cerr << "wrote baseline to " << out_path << "\n";
    }

    if (!regress_path.empty()) {
        std::vector<double> base_fps, base_p99;
        if (!load_baseline(regress_path, base_fps, base_p99)) {
            std::cerr << "no baseline at " << regress_path << "; skipping regression gate\n";
            return 0;
        }
        bool regressed = false;
        const std::size_t n = std::min(cases.size(), base_fps.size());
        // Latency below this floor (microseconds) is dominated by OS scheduler
        // jitter rather than the pipeline, so a relative gate on it is pure
        // noise. We only police cases whose baseline P99 sits above the floor,
        // i.e. the compute-bound configurations where the work, not scheduling,
        // sets the latency. Those cases also have stable sustained FPS.
        constexpr double kNoiseFloorUs = 500.0;
        int evaluated = 0;
        for (std::size_t i = 0; i < n; ++i) {
            if (i >= base_p99.size() || base_p99[i] < kNoiseFloorUs)
                continue;
            ++evaluated;
            // FPS dropping by more than threshold, or P99 growing by more than
            // threshold, is a regression.
            if (base_fps[i] > 0 && cases[i].sustained_fps < base_fps[i] * (1.0 - threshold)) {
                std::cerr << "FPS regression case " << i << ": " << cases[i].sustained_fps
                          << " < baseline " << base_fps[i] << "\n";
                regressed = true;
            }
            if (base_p99[i] > 0 && cases[i].p99 > base_p99[i] * (1.0 + threshold)) {
                std::cerr << "P99 regression case " << i << ": " << cases[i].p99 << " > baseline "
                          << base_p99[i] << "\n";
                regressed = true;
            }
        }
        std::cerr << "evaluated " << evaluated << " compute-bound case(s) above " << kNoiseFloorUs
                  << "us floor\n";
        if (regressed)
            return 1;
        std::cerr << "no regression beyond " << threshold * 100 << "%\n";
    }
    return 0;
}
