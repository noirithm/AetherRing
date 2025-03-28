#include "ring/spsc_ring.hpp"
#include "ring/backoff.hpp"
#include <benchmark/benchmark.h>
#include <thread>
#include <atomic>
#include <chrono>

using namespace aether;

static void BM_RingThroughput(benchmark::State& state) {
    for (auto _ : state) {
        SpscRing<int, 1024> ring;
        std::atomic<bool> stop{false};
        std::atomic<long> produced{0};

        std::thread producer([&]() {
            int i = 0;
            while (!stop.load(std::memory_order_relaxed)) {
                if (ring.try_push(i)) {
                    i++;
                    produced.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });

        std::thread consumer([&]() {
            while (!stop.load(std::memory_order_relaxed)) {
                ring.try_pop();
            }
        });

        auto start = std::chrono::high_resolution_clock::now();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        stop.store(true, std::memory_order_relaxed);
        producer.join();
        consumer.join();
        auto end = std::chrono::high_resolution_clock::now();

        double elapsed_seconds = std::chrono::duration<double>(end - start).count();
        state.SetIterationTime(elapsed_seconds);
        state.counters["items_per_second"] =
            benchmark::Counter(static_cast<double>(produced.load()), benchmark::Counter::kIsRate);
    }
}
BENCHMARK(BM_RingThroughput)->Unit(benchmark::kMillisecond)->UseManualTime()->Iterations(1);

static void BM_RingPingPongLatency(benchmark::State& state) {
    SpscRing<int, 8> ring_a2b;
    SpscRing<int, 8> ring_b2a;
    std::atomic<bool> stop{false};

    std::thread responder([&]() {
        while (!stop.load(std::memory_order_relaxed)) {
            if (auto val = ring_a2b.try_pop()) {
                while (!ring_b2a.try_push(*val)) { /* spin */ }
            }
        }
    });

    for (auto _ : state) {
        { aether::SpinBackoff bo; while (!ring_a2b.try_push(1)) bo.spin(); }
        { aether::SpinBackoff bo; while (!ring_b2a.try_pop()) bo.spin(); }
    }

    stop.store(true, std::memory_order_relaxed);
    responder.join();
}
BENCHMARK(BM_RingPingPongLatency)->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();