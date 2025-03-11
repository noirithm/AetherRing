#include <benchmark/benchmark.h>

static void BM_Placeholder(benchmark::State& state) {
    for (auto _ : state) {
        int x = 1 + 1;
        benchmark::DoNotOptimize(x);
    }
}
BENCHMARK(BM_Placeholder);

BENCHMARK_MAIN();
