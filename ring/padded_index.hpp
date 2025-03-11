#pragma once
#include <atomic>
#include <cstddef>

namespace aether {

struct alignas(64) PaddedIndex {
    std::atomic<std::size_t> value{0};
    char padding[64 - sizeof(std::atomic<std::size_t>)];
};

} // namespace aether
