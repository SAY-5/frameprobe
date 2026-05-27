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

// A fixed frame-metadata header that can prefix a raw frame on the wire (e.g.
// when a decoder feeds frames over a socket). Parsing untrusted headers is a
// classic source of integer-overflow bugs, so this is a second fuzz target.
//
// Layout (little-endian): u32 magic('FPFR') u64 index, u16 width, u16 height,
//                         u8 channels, u8 reserved
struct FrameHeader {
    std::uint64_t index = 0;
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    std::uint8_t channels = 0;
};

constexpr std::uint32_t kFrameMagic = 0x46504652u;  // 'FPFR'

std::vector<std::uint8_t> serialize_header(const FrameHeader& h);

// Rejects bad magic, truncation, zero dimensions, channels outside 1..4, and any
// header whose width*height*channels would overflow a 32-bit byte count.
std::optional<FrameHeader> parse_header(const std::uint8_t* data, std::size_t size);

inline std::optional<FrameHeader> parse_header(const std::vector<std::uint8_t>& buf) {
    return parse_header(buf.data(), buf.size());
}

}  // namespace wire
}  // namespace frameprobe

#endif  // FRAMEPROBE_DETECT_SERIALIZE_H
