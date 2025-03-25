#include "simd/checksum.hpp"
#include <gtest/gtest.h>
#include <vector>
#include <random>

using namespace aether;

TEST(Checksum, EmptyInput) {
    std::vector<uint8_t> data;
    EXPECT_EQ(checksum_scalar(data), 0u);
}

TEST(Checksum, SingleByte) {
    std::vector<uint8_t> data = {0xAB};
    uint32_t result = checksum_scalar(data);
    EXPECT_EQ(result, 0xAB00u);
}

TEST(Checksum, KnownValue) {
    std::vector<uint8_t> data = {0x00, 0x01, 0x00, 0x02, 0x00, 0x03};
    EXPECT_EQ(checksum_scalar(data), 0x0006u);
}

TEST(Checksum, ScalarAndDispatchAgreeOnVariousSizes) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> byte_dist(0, 255);

    for (std::size_t size : {0, 1, 2, 31, 32, 33, 63, 64, 65, 1500, 9000}) {
        std::vector<uint8_t> data(size);
        for (auto& b : data) b = static_cast<uint8_t>(byte_dist(rng));

        uint32_t scalar_result = checksum_scalar(data);
        uint32_t dispatch_result = checksum(data);

        EXPECT_EQ(scalar_result, dispatch_result)
            << "mismatch at size " << size;
    }
}

#if defined(__x86_64__)
TEST(Checksum, Avx2MatchesScalarDirectly) {
    if (!__builtin_cpu_supports("avx2")) {
        GTEST_SKIP() << "CPU does not support AVX2";
    }

    std::mt19937 rng(7);
    std::uniform_int_distribution<int> byte_dist(0, 255);

    for (std::size_t size : {32, 64, 96, 1500, 9000}) {
        std::vector<uint8_t> data(size);
        for (auto& b : data) b = static_cast<uint8_t>(byte_dist(rng));

        EXPECT_EQ(checksum_scalar(data), checksum_avx2(data))
            << "mismatch at size " << size;
    }
}
#endif
