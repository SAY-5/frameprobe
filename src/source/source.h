#ifndef FRAMEPROBE_SOURCE_SOURCE_H
#define FRAMEPROBE_SOURCE_SOURCE_H

#include <optional>

#include "source/frame.h"

namespace frameprobe {

// A frame producer. next() returns the next frame or nullopt at end of stream.
class FrameSource {
  public:
    virtual ~FrameSource() = default;
    virtual std::optional<Frame> next() = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
};

}  // namespace frameprobe

#endif  // FRAMEPROBE_SOURCE_SOURCE_H
