#include "ring/spsc_ring.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace aether;

TEST(SpscRing, PushPopSingleThreaded) {
    SpscRing<int, 8> ring;

    EXPECT_TRUE(ring.try_push(42));
    EXPECT_TRUE(ring.try_push(99));

    auto a = ring.try_pop();
    ASSERT_TRUE(a.has_value());
    EXPECT_EQ(*a, 42);

    auto b = ring.try_pop();
    ASSERT_TRUE(b.has_value());
    EXPECT_EQ(*b, 99);

    auto c = ring.try_pop();
    EXPECT_FALSE(c.has_value());  // buffer should be empty now
}

TEST(SpscRing, FullBufferRejectsStackPush) {
    SpscRing<int, 4> ring;  // capacity 4 means only 3 usable slots (1 always kept empty to distinguish full/empty)

    EXPECT_TRUE(ring.try_push(1));
    EXPECT_TRUE(ring.try_push(2));
    EXPECT_TRUE(ring.try_push(3));
    EXPECT_FALSE(ring.try_push(4));  // should be full now
}

TEST(SpscRing, ConcurrentProducerConsumerNoLossOrDuplication) {
    SpscRing<int, 1024> ring;
    constexpr int ITEMS = 1'000'000;
    std::vector<int> received;
    received.reserve(ITEMS);

    std::thread producer([&ring]() {
        for (int i = 0; i < ITEMS; i++) {
            while (!ring.try_push(i)) { /* spin */ }
        }
    });

    std::thread consumer([&ring, &received]() {
        for (int i = 0; i < ITEMS; i++) {
            std::optional<int> val;
            while (!(val = ring.try_pop())) { /* spin */ }
            received.push_back(*val);
        }
    });

    producer.join();
    consumer.join();

    ASSERT_EQ(received.size(), static_cast<size_t>(ITEMS));
    for (int i = 0; i < ITEMS; i++) {
        EXPECT_EQ(received[static_cast<size_t>(i)], i) << "mismatch at index " << i;
    }
}
