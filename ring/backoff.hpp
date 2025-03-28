#pragma once
#include <thread>
#include <chrono>

#if defined(__x86_64__)
#include <immintrin.h>
#endif

namespace aether {

// Progressive backoff for spin-wait loops (see docs/PROGRESS.md,
// "spin-wait-cost"): tight hardware-pause spins first, since most waits
// in a ring buffer resolve almost immediately and a pure spin is cheapest
// for that common case; then yields the thread to the scheduler; then
// falls back to short, doubling sleeps for a wait that is genuinely
// taking a while, so a long-idle consumer doesn't peg a full core.
//
// Call spin() once per failed attempt in a retry loop. Call reset() once
// the operation succeeds, so the next wait starts back at the cheap stage.
class SpinBackoff {
public:
    void spin() {
        if (stage_ < kPauseSpins) {
            pause_once();
        } else if (stage_ < kPauseSpins + kYieldSpins) {
            std::this_thread::yield();
        } else {
            std::this_thread::sleep_for(std::chrono::microseconds(sleep_us_));
            if (sleep_us_ < kMaxSleepUs) sleep_us_ *= 2;
        }
        if (stage_ < kMaxStage) stage_++;
    }

    void reset() {
        stage_ = 0;
        sleep_us_ = kInitialSleepUs;
    }

    unsigned current_stage() const { return stage_; }

private:
    static void pause_once() {
#if defined(__x86_64__)
        _mm_pause();
#else
        std::this_thread::yield();
#endif
    }

    static constexpr unsigned kPauseSpins = 32;
    static constexpr unsigned kYieldSpins = 16;
    static constexpr unsigned kMaxStage = kPauseSpins + kYieldSpins + 8;
    static constexpr unsigned kInitialSleepUs = 10;
    static constexpr unsigned kMaxSleepUs = 1000;

    unsigned stage_ = 0;
    unsigned sleep_us_ = kInitialSleepUs;
};

} // namespace aether
