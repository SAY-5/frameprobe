# Detector

The `Detector` interface is one call:

```cpp
DetectionResult detect(const Frame& frame);
```

`DetectionResult` is a frame index plus a list of `Detection` boxes
(`x, y, w, h, class_id, confidence`).

## StubDetector (default)

`StubDetector` returns exactly the boxes the `SyntheticSource` drew for each
frame, with a confidence derived deterministically from the frame index. Its
output is a pure function of the frame index, which makes the pipeline testable:
detections are bit-identical across runs and thread counts.

It also takes a `compute_us` budget and burns that many microseconds on a
side-effecting spin loop per frame. Spinning (rather than sleeping) keeps the
stage CPU-bound like real inference and visible to the scheduler, so latency and
deadline-miss behaviour are realistic and tunable without a model.

## DnnDetector (opt-in)

`DnnDetector` loads an ONNX model through OpenCV's DNN module and runs a forward
pass per frame, decoding a MobileNet-SSD style `[1,1,N,7]` output. It is built
only with `-DFRAMEPROBE_WITH_OPENCV=ON` and needs a model path supplied at
runtime (`--model`). See `models/README.md`. The CI default stays on the stub so
the build is hermetic and the determinism guarantees hold.
