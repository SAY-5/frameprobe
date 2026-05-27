# Soft real-time budgeting

A target frame rate sets a per-frame deadline:

```
deadline = 1 / target_fps        (30 FPS -> 33.33 ms)
```

For each frame the harness tracks:

- `arrival`        the decoder produced the frame
- `inference_end`  the detector finished
- `latency`        `inference_end - arrival`, the end-to-end time
- deadline outcome `latency <= deadline ? met : missed`

The sink aggregates these into the run report: sustained FPS (processed frames
over wall time), latency P50/P95/P99, deadline-miss rate, mean confidence, and
peak queue depth.

## Bounded queue and back-pressure

The frame queue is a `BoundedQueue<T>` with a fixed capacity and one of two
overflow policies, which is the core soft-real-time knob:

- **block** (back-pressure): when the queue is full the decoder blocks until the
  inference stage drains an item. No frame is ever lost; the producer slows to
  the consumer's rate.
- **drop**: when the queue is full the decoder's push is rejected and a drop is
  counted. The pipeline keeps up with real time at the cost of skipped frames.

The condition variables are paired (`not_full` / `not_empty`) and `close()`
wakes every blocked thread so shutdown never hangs. This is the part most prone
to races, so it is covered by dedicated ThreadSanitizer tests.

## What is deterministic

With the `StubDetector`, a fixed synthetic stream, and a fixed seed, the
detection output for each frame is a pure function of the frame index, so it is
bit-identical across runs and across thread counts. Latency is a real timing
measurement and is therefore not deterministic, so tests assert detection
content exactly and latency only statistically (percentile ordering, bounds).
