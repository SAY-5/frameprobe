# FrameProbe

A C++20 multithreaded real-time vision inference harness. FrameProbe decodes a
video stream with OpenCV, runs a pretrained object detector over each frame,
and measures how the model holds up under a soft real-time frame-rate budget.

The pipeline is three threads connected by bounded queues:

```
decoder  ->  [frame queue]  ->  inference  ->  [result queue]  ->  sink/metrics
```

Each frame carries an arrival timestamp. A target FPS sets a per-frame deadline
(30 FPS = 33.3 ms). The sink records per-frame latency (decode to result),
detection confidence, and whether the result landed inside the deadline, then
reports sustained FPS, latency P50/P95/P99, deadline-miss rate, and mean
confidence.

When inference falls behind, the bounded frame queue applies back-pressure
(block) or drops frames (configurable). v4 adds an adaptive controller that skips
frames proportionally to how far behind the pipeline is, holding the deadline for
processed frames and resuming full rate on recovery.

## Why a stub detector

To keep CI hermetic and deterministic the default detector is `StubDetector`:
it returns scripted boxes with a configurable per-frame compute cost, so
detection output is bit-identical across runs and thread counts while latency
stays a real timing measurement. The real OpenCV-DNN path is behind a build flag
(`-DFRAMEPROBE_WITH_OPENCV=ON`) and a documented model URL (see `models/`).

## Build

```sh
make build      # core + stub detector + tests, no OpenCV
make test       # ctest
make asan       # AddressSanitizer + UBSan
make tsan       # ThreadSanitizer (pipeline stress)
make bench      # throughput benchmark -> bench/results/bench_local.json
```

OpenCV-backed video source and DNN detector:

```sh
cmake -S . -B build -DFRAMEPROBE_WITH_OPENCV=ON && cmake --build build -j
```

## Run

```sh
frameprobe --source synthetic --fps 30 --frames 300
frameprobe --source synthetic --fps 60 --frames 600 --queue 8 --backpressure drop
frameprobe --source synthetic --fps 30 --frames 900 --workers 4   # v3
frameprobe --source synthetic --fps 60 --frames 900 --adaptive    # v4
```

## Layout

```
src/source     synthetic + OpenCV video frame producers
src/detect     detector interface, StubDetector, DNN detector
src/pipeline    bounded queue, decoder/inference/sink threads, harness
src/metrics    latency histogram + per-frame stats
bench           throughput benchmark + regression gate
docs            pipeline / soft-realtime / detector / metrics notes
```

## License

MIT. See [LICENSE](LICENSE).
