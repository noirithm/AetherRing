#include "ring/backoff.hpp"
#include <gtest/gtest.h>

using namespace aether;

TEST(SpinBackoff, StageAdvancesThenCaps) {
    SpinBackoff b;
    EXPECT_EQ(b.current_stage(), 0u);
    for (int i = 0; i < 100; i++) b.spin();
    unsigned capped = b.current_stage();
    b.spin();
    EXPECT_EQ(b.current_stage(), capped); // stays capped, doesn't grow unbounded
}

TEST(SpinBackoff, ResetReturnsToInitialStage) {
    SpinBackoff b;
    for (int i = 0; i < 10; i++) b.spin();
    EXPECT_GT(b.current_stage(), 0u);
    b.reset();
    EXPECT_EQ(b.current_stage(), 0u);
}
