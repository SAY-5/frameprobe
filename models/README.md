# Models

FrameProbe defaults to the `StubDetector` so CI stays hermetic and deterministic
(no model download, scripted boxes, configurable compute cost). The real
detector path is OpenCV's DNN module and is opt-in.

## Enabling the DNN detector

Build with OpenCV and point the harness at an ONNX model:

```sh
cmake -S . -B build -DFRAMEPROBE_WITH_OPENCV=ON && cmake --build build -j
frameprobe --source video --video clip.mp4 --detector dnn --model mobilenet_ssd.onnx
```

## Suggested model

A small MobileNet-SSD ONNX (input 300x300, output `[1,1,N,7]`) works with
`DnnDetector` as written. One commonly used export:

- MobileNet-SSD v2 ONNX, ~10-25 MB.

The model is intentionally not committed to keep the repository small and the CI
hermetic. Download it out of band and pass `--model`. The output decoding in
`src/detect/dnn_detector.cpp` assumes the SSD `[_, class, conf, x1, y1, x2, y2]`
layout; adjust for a different head.
