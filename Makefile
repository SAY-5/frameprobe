BUILD_DIR ?= build
CMAKE ?= cmake
CMAKE_FLAGS ?=

.PHONY: all build configure test asan tsan fuzz bench bench-regress format format-check clean

all: build

configure:
	$(CMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release $(CMAKE_FLAGS)

build: configure
	$(CMAKE) --build $(BUILD_DIR) -j

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

asan:
	$(CMAKE) -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DFRAMEPROBE_ASAN=ON $(CMAKE_FLAGS)
	$(CMAKE) --build build-asan -j
	ctest --test-dir build-asan --output-on-failure

tsan:
	$(CMAKE) -S . -B build-tsan -DCMAKE_BUILD_TYPE=Debug -DFRAMEPROBE_TSAN=ON $(CMAKE_FLAGS)
	$(CMAKE) --build build-tsan -j
	ctest --test-dir build-tsan --output-on-failure -R "concurrency|pipeline|reorder|adaptive"

fuzz:
	$(CMAKE) -S . -B build-fuzz -DCMAKE_BUILD_TYPE=Debug -DFRAMEPROBE_BUILD_FUZZ=ON -DCMAKE_CXX_COMPILER=clang++ $(CMAKE_FLAGS)
	$(CMAKE) --build build-fuzz -j --target fuzz_detection

bench: build
	$(BUILD_DIR)/throughput_bench --frames 3000 --out bench/results/bench_local.json

bench-regress: build
	$(BUILD_DIR)/throughput_bench --frames 300 --regress bench/results/bench_local.json --threshold 0.30

format:
	@find src tests bench -name '*.cpp' -o -name '*.h' | xargs clang-format -i

format-check:
	@find src tests bench -name '*.cpp' -o -name '*.h' | xargs clang-format --dry-run --Werror

clean:
	$(CMAKE) -E rm -rf $(BUILD_DIR) build-asan build-tsan build-fuzz
