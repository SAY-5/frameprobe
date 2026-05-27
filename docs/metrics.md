# Metrics

## LatencyHist

Records per-frame end-to-end latency samples in microseconds and computes
percentiles by sorting on demand. Runs here are a few thousand frames, so an
exact sort is cheap and keeps percentile assertions exact (no histogram
bucketing error). Exposes P50/P95/P99, mean, min, and max.

## FrameStats

Aggregates the per-frame outcomes observed at the sink and produces the run
report:

| field             | meaning                                            |
|-------------------|----------------------------------------------------|
| frames_processed  | frames run through the detector                    |
| frames_dropped    | frames dropped by the queue in drop mode           |
| frames_skipped    | frames skipped by the adaptive controller (v4)     |
| sustained_fps     | processed frames / wall-clock seconds              |
| deadline_us       | per-frame budget from the target FPS               |
| deadline_misses   | processed frames whose latency exceeded the budget |
| miss_rate         | deadline_misses / processed                        |
| latency_p50/95/99 | latency percentiles, microseconds                  |
| mean_confidence   | mean max-confidence across processed frames        |
| peak_queue_depth  | high-water mark of the frame queue                 |

The report is a stable `key=value` block, which doubles as a smoke-check target
in the e2e tests and a machine-readable summary on stdout.
