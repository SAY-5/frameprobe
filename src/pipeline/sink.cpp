#include "pipeline/sink.h"

#include <chrono>

namespace frameprobe {

void Sink::flush_ready() {
    // Emit every contiguous frame starting at next_expected_ that has arrived.
    auto it = pending_.find(next_expected_);
    while (it != pending_.end()) {
        const ResultEnvelope& env = it->second;
        if (env.skipped) {
            stats_.record_skipped();
        } else {
            const double latency_us =
                std::chrono::duration<double, std::micro>(env.inference_end - env.arrival).count();
            stats_.record_processed(latency_us, env.result.max_confidence());
        }
        if (emit_)
            emit_(env);
        pending_.erase(it);
        ++next_expected_;
        it = pending_.find(next_expected_);
    }
}

void Sink::run() {
    while (true) {
        auto env = in_.pop();
        if (!env)
            break;
        if (depth_probe_)
            stats_.observe_queue_depth(depth_probe_());
        const std::uint64_t idx = env->result.frame_index;
        pending_.emplace(idx, std::move(*env));
        flush_ready();
    }
    // Drain anything left (e.g. if a frame index was dropped upstream we still
    // emit the remaining buffered frames in order).
    while (!pending_.empty()) {
        next_expected_ = pending_.begin()->first;
        flush_ready();
    }
}

}  // namespace frameprobe
