# Adaptive frame-skipping

Under overload the pipeline falls behind the real-time budget: the frame queue
fills and processed frames sit in the queue long enough to blow their deadline.
With `--adaptive` the harness runs a controller that skips frames to catch up,
then resumes full processing once it recovers.

## The controller

`AdaptiveController` is a proportional controller keyed on the live frame-queue
depth. The decoder consults it for every frame:

- At or below a high-watermark (default `capacity / 4`), the skip stride is 1:
  process every frame.
- Past the watermark the stride grows linearly with how far the depth has pushed
  toward capacity, clamped to `max_skip` (default 8). At stride K the controller
  processes one frame and skips the next K-1.
- When the queue drains back below the watermark the stride returns to 1. That
  is the recovery: full processing resumes automatically.

A skipped frame is still decoded and forwarded, but the inference stage does not
run the detector on it (it passes straight through), so skipping genuinely frees
inference capacity and lets the kept frames drain the queue. Skipped frames are
counted and reported separately (`frames_skipped`) from processed ones.

## The trade-off

Skipping trades detection coverage for latency. The kept frames meet their
deadline; the skipped frames produce no detection at all. The controller is
deliberately depth-driven rather than accuracy-driven: it spends exactly as much
skipping as the backlog demands and no more, so on recovery it gives the full
frame rate back.

The effect is measured in `Adaptive.HoldsDeadlineOnProcessedFramesUnderOverload`.
Feeding 800 frames at a 2 ms deadline with a 300 us detector and a 16-deep queue:

| mode         | processed | skipped | miss rate (processed) |
|--------------|-----------|---------|-----------------------|
| non-adaptive | 800       | 0       | ~0.99                 |
| adaptive     | ~180      | ~620    | <0.20                 |

Without adaptation the queue wait alone (up to 16 x 300 us = 4.8 ms) exceeds the
2 ms deadline, so almost every processed frame misses. With adaptation the queue
stays shallow, so the frames that are processed comfortably meet the deadline,
at the cost of dropping detection on the skipped majority. When the input slows
(`Adaptive.NoSkipWhenInputArrivesSlowly`) the queue never builds, the stride
stays at 1, and nothing is skipped.

## Run it

```sh
frameprobe --source synthetic --frames 800 --fps 500 --queue 16 --compute-us 300 --adaptive
```
