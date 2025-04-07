# AetherRing

A lock-free, zero-copy packet engine written in C++23, exploring systems programming through atomics, memory ordering, cache efficiency, Linux networking, SIMD, and packet parsing.

## What's in here

- **`ring/`** — A lock-free single-producer/single-consumer (SPSC) ring buffer with cache-line-separated indices and acquire/release memory ordering, plus a progressive spin/yield/sleep backoff helper.
- **`arena/`** — A hugepage-backed memory arena with independent freelists to reduce allocation contention.
- **`net/`** — Two networking backends:
  - **io_uring:** Multishot UDP receive with a registered buffer ring, and zero-copy send handling for send and notification completions.
  - **AF_XDP:** UMEM-backed packet sockets with fill, completion, and RX ring handling, alongside an eBPF program in `ebpf/` for redirecting network traffic.
- **`simd/`** — An AVX2 checksum implementation with runtime CPU feature detection and a scalar fallback.
- **`proto/`** — A bounds-checked Ethernet/IPv4/UDP packet parser, tested using a `libFuzzer` harness in `fuzz/`.

## Building

```bash
mkdir build
cd build
cmake ..
cmake --build . -j$(nproc)
```

Dependencies include GoogleTest, Google Benchmark, liburing, and libbpf. These can be discovered through `pkg-config` or fetched through CMake `FetchContent`, depending on the project configuration.

## Testing

```bash
cd build
ctest --output-on-failure
```

Test suites cover the ring buffer, memory arena, io_uring networking, checksums, backoff logic, and packet parsing.

Some networking tests require elevated privileges and appropriate kernel support. AF_XDP testing also requires a compatible network interface and suitable driver support.

## Benchmarks

```bash
./build/bench/ring_bench
```

The benchmark suite measures ring-buffer performance and helps evaluate implementation changes.

## Fuzzing

The packet parser (`proto/packet_header.hpp`) has a libFuzzer harness at `fuzz/packet_fuzz.cpp`, built separately with Clang:

```bash
clang++ -std=c++23 -O1 -g \
  -fsanitize=fuzzer,address -I. \
  -o packet_fuzz fuzz/packet_fuzz.cpp

./packet_fuzz -max_total_time=30
```

## Project structure

```text
aetherring/
├── ring/       # SPSC ring buffer and backoff helper
├── arena/      # Memory arena and freelists
├── net/        # io_uring and AF_XDP packet paths
├── ebpf/       # XDP redirect program
├── simd/       # AVX2 checksum implementation
├── proto/      # Packet header parser
├── fuzz/       # libFuzzer harness
├── tests/      # GoogleTest suites
├── bench/      # Google Benchmark suites
├── docker/     # Build environment
└── docs/       # Project documentation
```

## Status

AetherRing brings together lock-free data structures, memory management, Linux networking, SIMD optimization, and packet parsing in a single systems-programming project.

The implementation includes an SPSC ring buffer, a memory arena, io_uring receive and send paths, AF_XDP packet reception with an eBPF redirect program, an AVX2 checksum implementation, and a fuzz-tested packet parser.
