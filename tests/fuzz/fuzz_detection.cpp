#include <cstddef>
#include <cstdint>

#include "detect/serialize.h"

// libFuzzer entry point. Feeds arbitrary bytes to the detection-result wire
// parser and, on any value that parses, round-trips it back through the
// serializer to confirm a stable encode/decode. The parser must never crash,
// over-read, or over-allocate on malformed input.
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    auto parsed = frameprobe::wire::deserialize(data, size);
    if (parsed) {
        auto reencoded = frameprobe::wire::serialize(*parsed);
        auto reparsed = frameprobe::wire::deserialize(reencoded);
        // A successfully parsed value must re-encode to something that parses
        // back to the same logical result.
        if (!reparsed || reparsed->frame_index != parsed->frame_index ||
            reparsed->detections.size() != parsed->detections.size()) {
            __builtin_trap();
        }
    }
    return 0;
}
