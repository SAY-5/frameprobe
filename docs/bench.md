# Benchmark and regression gate

`throughput_bench` runs the full pipeline over a matrix of target frame rates
(15/30/60 FPS) crossed with stub compute costs (0 and 200 us per frame) and
reports, per case: sustained FPS, latency P50/P95/P99, deadline-miss rate, and
peak queue depth. Output is JSON on stdout; `--out <file>` writes a baseline.

```sh
make bench           # N=3000 frames, writes bench/results/bench_local.json
make bench-regress   # N=300 frames, compares against the committed baseline
```

## What the numbers mean

The zero-compute cases are decode-bound: the stub returns immediately, so the
pipeline runs as fast as the synthetic source and queues can feed it, and
per-frame latency is microseconds (dominated by thread handoff and the queue).
The 200 us cases are inference-bound: each frame costs the detector ~200 us, so
sustained throughput settles near 1e6 / 200 = 5000 FPS and P99 latency sits just
above 200 us plus pipeline overhead. A representative local run:

| target FPS | compute us | sustained FPS | P99 us |
|------------|------------|---------------|--------|
| 15         | 0          | ~113000       | ~80    |
| 15         | 200        | ~4900         | ~3700  |
| 30         | 0          | ~110000       | ~11    |
| 30         | 200        | ~4900         | ~3700  |
| 60         | 0          | ~115000       | ~15    |
| 60         | 200        | ~4900         | ~3700  |

(The compute-bound P99 here reflects this machine running six pipelines back to
back; an idle runner sits closer to 200 us. The gate is relative, so absolute
host speed does not matter.)

## Regression gate

`--regress <baseline> --threshold 0.30` fails the build if, for any
*compute-bound* case, sustained FPS drops by more than 30 percent or P99 grows
by more than 30 percent versus the baseline.

Cases whose baseline P99 is below a 500 us noise floor are skipped: at the
microsecond scale, latency is dominated by OS scheduler jitter rather than the
pipeline, so a relative gate on it would be pure noise (a single scheduling
hiccup can double a 10 us measurement). The compute-bound cases, where the work
sets the latency, are stable across runs and are what the gate polices. CI runs
a 300-frame smoke plus a self-consistent regression check on the runner.
