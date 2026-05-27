FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        git \
        ca-certificates \
        libopencv-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B build \
        -DCMAKE_BUILD_TYPE=Release \
        -DFRAMEPROBE_WITH_OPENCV=ON \
        -DFRAMEPROBE_BUILD_TESTS=OFF \
    && cmake --build build -j

ENTRYPOINT ["/src/build/frameprobe"]
CMD ["--source", "synthetic", "--fps", "30", "--frames", "300"]
