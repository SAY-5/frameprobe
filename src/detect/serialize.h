#ifndef FRAMEPROBE_DETECT_SERIALIZE_H
#define FRAMEPROBE_DETECT_SERIALIZE_H

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "detect/detector.h"

namespace frameprobe {

// A small, self-describing binary wire format for a DetectionResult. Used by the
// sink for optional logging and exercised directly by the libFuzzer target, so
// the parser must reject malformed input without crashing or over-reading.
//
// Layout (all little-endian):
//   u32 magic = 0x46505244 ('FPRD')
//   u64 frame_index
//   u32 count
//   count * { i32 x, i32 y, i32 w, i32 h, i32 class_id, f32 confidence }
namespace wire {

constexpr std::uint32_t kMagic = 0x46505244u;

std::vector<std::uint8_t> serialize(const DetectionResult& result);

// Returns nullopt on any malformed input (bad magic, truncation, absurd count).
std::optional<DetectionResult> deserialize(const std::uint8_t* data, std::size_t size);

inline std::optional<DetectionResult> deserialize(const std::vector<std::uint8_t>& buf) {
    return deserialize(buf.data(), buf.size());
}

}  // namespace wire
}  // namespace frameprobe

#endif  // FRAMEPROBE_DETECT_SERIALIZE_H
