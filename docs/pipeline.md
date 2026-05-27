# Pipeline

FrameProbe runs three stages on three (or more) threads connected by bounded
queues:

```
 source --> Decoder --> [frame queue] --> InferenceWorker(s) --> [result queue] --> Sink
```

## Decoder

Pulls frames from a `FrameSource` (`SyntheticSource` or, with OpenCV,
`VideoSource`), wraps each in a `FrameEnvelope`, and pushes it onto the frame
queue. When the source is exhausted it closes the queue, which is how the
downstream threads learn to drain and exit. The decoder consults an optional
skip decider (v4) to mark frames the controller wants skipped.

## Inference

One or more `InferenceWorker` threads pop frame envelopes, run the detector
(`StubDetector` by default), stamp `inference_end`, and push a `ResultEnvelope`
carrying the original `arrival` timestamp. Each worker owns no per-frame mutable
state, so several can share the frame and result queues. Workers finish out of
order when there are several of them.

## Sink

A single thread that pops results, reorders them into strict frame order with a
reorder buffer keyed on frame index, and records metrics. Being single-threaded
makes all metric updates race-free without locks on `FrameStats`.

## Threading and shutdown

- The frame and result queues are `BoundedQueue<T>` (see `soft-realtime.md`).
- Shutdown is cooperative: decoder closes the frame queue, workers drain and
  exit, the harness then closes the result queue, and the sink drains and exits.
- The whole path is exercised under ThreadSanitizer in CI
  (`PipelineConcurrency.*`, `BoundedQueue.*`).
