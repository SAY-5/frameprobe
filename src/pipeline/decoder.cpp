#include "pipeline/decoder.h"

namespace frameprobe {

void Decoder::run() {
    while (true) {
        auto frame = source_.next();
        if (!frame)
            break;
        FrameEnvelope env;
        env.skip = skip_decider_ ? skip_decider_(frame->index) : false;
        env.frame = std::move(*frame);
        out_.push(std::move(env));
    }
    out_.close();
}

}  // namespace frameprobe
