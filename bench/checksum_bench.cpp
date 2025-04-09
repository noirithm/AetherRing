// Scalar vs AVX2 checksum throughput across realistic packet sizes.
//
// Run:   ./build/bench/checksum_bench
// JSON:  ./build/bench/checksum_bench --benchmark_format=json > checksum.json
//
// Sizes: 64 B (min Ethernet frame), 512 B, 1500 B (standard MTU),
// 9000 B (jumbo frame). Throughput is reported as bytes/second.
#include "simd/checksum.hpp"
#include <benchmark/benchmark.h>
#include <cstdint>
#include <random>
#include <vector>

using namespace aether;

namespace {

std::vector<uint8_t> make_buffer(std::size_t size) {
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist(0, 255);
    std::vector<uint8_t> buf(size);
    for (auto& b : buf) b = static_cast<uint8_t>(dist(rng));
    return buf;
}

void BM_ChecksumScalar(benchmark::State& state) {
    auto buf = make_buffer(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        benchmark::DoNotOptimize(checksum_scalar(buf));
    }
    state.SetBytesProcessed(state.iterations() * state.range(0));
}

#if defined(__x86_64__)
void BM_ChecksumAvx2(benchmark::State& state) {
    if (!__builtin_cpu_supports("avx2")) {
        state.SkipWithError("CPU does not support AVX2");
        return;
    }
    auto buf = make_buffer(static_cast<std::size_t>(state.range(0)));
    for (auto _ : state) {
        benchmark::DoNotOptimize(checksum_avx2(buf));
    }
    state.SetBytesProcessed(state.iterations() * state.range(0));
}
#endif

} // namespace

BENCHMARK(BM_ChecksumScalar)->Arg(64)->Arg(512)->Arg(1500)->Arg(9000);
#if defined(__x86_64__)
BENCHMARK(BM_ChecksumAvx2)->Arg(64)->Arg(512)->Arg(1500)->Arg(9000);
#endif

BENCHMARK_MAIN();
