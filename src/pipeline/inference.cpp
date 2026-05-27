#include "pipeline/inference.h"

namespace frameprobe {

void InferenceWorker::run() {
    while (true) {
        auto env = in_.pop();
        if (!env)
            break;  // queue closed and drained

        ResultEnvelope out;
        out.arrival = env->frame.arrival;
        out.skipped = env->skip;
        if (env->skip) {
            out.result.frame_index = env->frame.index;
        } else {
            out.result = detector_.detect(env->frame);
        }
        out.inference_end = Clock::now();
        out_.push(std::move(out));
    }
}

}  // namespace frameprobe
