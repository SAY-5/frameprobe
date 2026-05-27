cmake_minimum_required(VERSION 3.16)
project(frameprobe LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(FRAMEPROBE_WITH_OPENCV "Build the OpenCV-backed video source and DNN detector" OFF)
option(FRAMEPROBE_BUILD_TESTS "Build unit/e2e tests with GoogleTest" ON)
option(FRAMEPROBE_BUILD_FUZZ "Build libFuzzer targets (requires clang)" OFF)
option(FRAMEPROBE_BUILD_BENCH "Build the throughput benchmark" ON)
option(FRAMEPROBE_ASAN "Enable AddressSanitizer + UBSan" OFF)
option(FRAMEPROBE_TSAN "Enable ThreadSanitizer" OFF)
option(FRAMEPROBE_COVERAGE "Enable coverage instrumentation" OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

if(FRAMEPROBE_ASAN)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()
if(FRAMEPROBE_TSAN)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=thread)
endif()
if(FRAMEPROBE_COVERAGE)
    add_compile_options(--coverage -O0 -g)
    add_link_options(--coverage)
endif()

set(FRAMEPROBE_CORE_SOURCES
    src/source/synthetic_source.cpp
    src/detect/stub_detector.cpp
    src/detect/serialize.cpp
    src/pipeline/decoder.cpp
    src/pipeline/inference.cpp
    src/pipeline/sink.cpp
    src/pipeline/harness.cpp
    src/metrics/latency_hist.cpp
    src/metrics/frame_stats.cpp
)

add_library(frameprobe_core ${FRAMEPROBE_CORE_SOURCES})
target_include_directories(frameprobe_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
find_package(Threads REQUIRED)
target_link_libraries(frameprobe_core PUBLIC Threads::Threads)

if(FRAMEPROBE_WITH_OPENCV)
    find_package(OpenCV REQUIRED)
    target_sources(frameprobe_core PRIVATE
        src/source/video_source.cpp
        src/detect/dnn_detector.cpp)
    target_include_directories(frameprobe_core PUBLIC ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(frameprobe_core PUBLIC ${OpenCV_LIBS})
    target_compile_definitions(frameprobe_core PUBLIC FRAMEPROBE_WITH_OPENCV=1)
endif()

add_executable(frameprobe src/main.cpp)
target_link_libraries(frameprobe PRIVATE frameprobe_core)

if(FRAMEPROBE_BUILD_BENCH)
    add_executable(throughput_bench bench/throughput_bench.cpp)
    target_link_libraries(throughput_bench PRIVATE frameprobe_core)
endif()

if(FRAMEPROBE_BUILD_TESTS)
    enable_testing()
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    file(GLOB FRAMEPROBE_UNIT_TESTS CONFIGURE_DEPENDS tests/unit/*.cpp)
    file(GLOB FRAMEPROBE_E2E_TESTS CONFIGURE_DEPENDS tests/e2e/*.cpp)
    add_executable(frameprobe_tests ${FRAMEPROBE_UNIT_TESTS} ${FRAMEPROBE_E2E_TESTS})
    target_link_libraries(frameprobe_tests PRIVATE frameprobe_core GTest::gtest_main)
    include(GoogleTest)
    # Bump the discovery probe timeout: with OpenCV linked, loading the shared
    # libraries at startup can exceed the 5s default on a cold cache.
    gtest_discover_tests(frameprobe_tests DISCOVERY_TIMEOUT 60)
endif()

if(FRAMEPROBE_BUILD_FUZZ)
    add_executable(fuzz_detection tests/fuzz/fuzz_detection.cpp)
    target_link_libraries(fuzz_detection PRIVATE frameprobe_core)
    target_compile_options(fuzz_detection PRIVATE -fsanitize=fuzzer,address)
    target_link_options(fuzz_detection PRIVATE -fsanitize=fuzzer,address)
endif()
