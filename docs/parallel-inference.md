# Parallel inference

The inference stage scales to N worker threads that all pull from the shared,
bounded frame queue and push to the shared result queue. Each worker owns its
own per-frame state, so several can run the detector concurrently. Workers
finish out of order: a frame that happens to take longer can complete after a
later frame, so the order results reach the sink is not the frame order.

## Reorder buffer

The sink restores strict frame order with a reorder buffer keyed on frame index.
It tracks the next expected index and a `std::map` of results that arrived early.
On each result it inserts into the map and then flushes every contiguous frame
starting at the next expected index. Because the sink is single threaded, this
needs no locking and the emit order is exactly `0, 1, 2, ...`.

Correctness is pinned by tests (`Reorder.*`): a shuffled result stream and a
concurrent out-of-order producer both emit in strict frame order, and the full
multi-worker pipeline processes every frame exactly once with no loss.

## Scaling: inference-bound vs decode-bound

Run the worker sweep:

```sh
throughput_bench --scaling --frames 2000 --scaling-compute-us 500
throughput_bench --scaling --frames 2000 --scaling-compute-us 0
```

With an inference-bound cost (500 us per frame), throughput scales close to
linearly until it hits the physical core count, then saturates:

| workers | sustained FPS | speedup |
|---------|---------------|---------|
| 1       | ~1960         | 1.0x    |
| 2       | ~3940         | 2.0x    |
| 4       | ~7890         | 4.0x    |
| 8       | ~7860         | 4.0x    |

With zero compute cost the pipeline is decode-bound: the single decoder thread
feeds frames as fast as it can and extra inference workers add no throughput
(speedup stays around 0.9x, slightly below 1.0 from contention on the shared
queue). This is the expected crossover: parallelism only helps once inference,
not decode, is the bottleneck.
