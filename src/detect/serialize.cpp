#include "detect/serialize.h"

#include <cstring>

namespace frameprobe::wire {

namespace {

constexpr std::uint32_t kMaxDetections = 1u << 20;  // hard cap to bound parsing

template <typename T>
void put(std::vector<std::uint8_t>& out, T v) {
    auto* p = reinterpret_cast<const std::uint8_t*>(&v);
    out.insert(out.end(), p, p + sizeof(T));
}

template <typename T>
bool take(const std::uint8_t* data, std::size_t size, std::size_t& off, T& out) {
    if (off + sizeof(T) > size)
        return false;
    std::memcpy(&out, data + off, sizeof(T));
    off += sizeof(T);
    return true;
}

}  // namespace

std::vector<std::uint8_t> serialize(const DetectionResult& result) {
    std::vector<std::uint8_t> out;
    put<std::uint32_t>(out, kMagic);
    put<std::uint64_t>(out, result.frame_index);
    put<std::uint32_t>(out, static_cast<std::uint32_t>(result.detections.size()));
    for (const auto& d : result.detections) {
        put<std::int32_t>(out, d.x);
        put<std::int32_t>(out, d.y);
        put<std::int32_t>(out, d.w);
        put<std::int32_t>(out, d.h);
        put<std::int32_t>(out, d.class_id);
        put<float>(out, d.confidence);
    }
    return out;
}

std::optional<DetectionResult> deserialize(const std::uint8_t* data, std::size_t size) {
    if (data == nullptr)
        return std::nullopt;
    std::size_t off = 0;
    std::uint32_t magic = 0;
    if (!take(data, size, off, magic) || magic != kMagic)
        return std::nullopt;

    DetectionResult result;
    if (!take(data, size, off, result.frame_index))
        return std::nullopt;

    std::uint32_t count = 0;
    if (!take(data, size, off, count))
        return std::nullopt;
    if (count > kMaxDetections)
        return std::nullopt;

    // Reject before reserving so a huge count cannot trigger a large allocation
    // when the buffer cannot possibly contain that many records.
    constexpr std::size_t kRecordSize = 5 * sizeof(std::int32_t) + sizeof(float);
    if (static_cast<std::size_t>(count) * kRecordSize > size - off)
        return std::nullopt;

    result.detections.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        Detection d;
        if (!take(data, size, off, d.x) || !take(data, size, off, d.y) ||
            !take(data, size, off, d.w) || !take(data, size, off, d.h) ||
            !take(data, size, off, d.class_id) || !take(data, size, off, d.confidence)) {
            return std::nullopt;
        }
        result.detections.push_back(d);
    }
    return result;
}

}  // namespace frameprobe::wire
