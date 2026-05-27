#include <cstddef>
#include <cstdint>

#include "detect/serialize.h"

// libFuzzer entry point. The first byte selects the parse target so a single
// corpus exercises both the detection-result wire parser and the frame-metadata
// header parser. Neither parser may crash, over-read, or over-allocate on
// malformed input; any value that parses must round-trip through its serializer.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (size == 0)
        return 0;
    const std::uint8_t selector = data[0];
    const std::uint8_t* body = data + 1;
    const std::size_t body_size = size - 1;

    if ((selector & 1u) == 0) {
        auto parsed = frameprobe::wire::deserialize(body, body_size);
        if (parsed) {
            auto reencoded = frameprobe::wire::serialize(*parsed);
            auto reparsed = frameprobe::wire::deserialize(reencoded);
            if (!reparsed || reparsed->frame_index != parsed->frame_index ||
                reparsed->detections.size() != parsed->detections.size()) {
                __builtin_trap();
            }
        }
    } else {
        auto parsed = frameprobe::wire::parse_header(body, body_size);
        if (parsed) {
            auto reencoded = frameprobe::wire::serialize_header(*parsed);
            auto reparsed = frameprobe::wire::parse_header(reencoded);
            if (!reparsed || reparsed->index != parsed->index || reparsed->width != parsed->width ||
                reparsed->height != parsed->height || reparsed->channels != parsed->channels) {
                __builtin_trap();
            }
        }
    }
    return 0;
}
