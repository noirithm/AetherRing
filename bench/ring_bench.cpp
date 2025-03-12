#include "ring/spsc_ring.hpp"
#include <benchmark/benchmark.h>
#include <thread>
#include <atomic>

using namespace aether;

// Measures raw push/pop throughput: how fast can one thread push,
// one thread pop, continuously, for the benchmark's duration.
static void BM_RingThroughput(benchmark::State& state) {
    for (auto _ : state) {
        SpscRing<int, 1024> ring;
        std::atomic<bool> stop{false};

        std::thread producer([&]() {
            int i = 0;
            while (!stop.load(std::memory_order_relaxed)) {
                if (ring.try_push(i)) i++;
            }
        });

        std::thread consumer([&]() {
            int count = 0;
            while (count < 100000) {
                if (ring.try_pop()) count++;
            }
            stop.store(true, std::memory_order_relaxed);
        });

        producer.join();
        consumer.join();
    }
}
BENCHMARK(BM_RingThroughput)->Unit(benchmark::kMillisecond);

// Measures round-trip latency: thread A pushes to ring_a2b, waits for
// thread B to pop it and push a reply on ring_b2a, then A pops that reply.
// This is the "ping-pong" pattern - one full round trip per iteration.
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
        while (!ring_a2b.try_push(1)) { /* spin */ }
        while (!ring_b2a.try_pop()) { /* spin */ }
    }

    stop.store(true, std::memory_order_relaxed);
    responder.join();
}
BENCHMARK(BM_RingPingPongLatency)->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();