#pragma once
#include <array>
#include <optional>
#include "padded_index.hpp"

namespace aether {

template <typename T, std::size_t Capacity>
class SpscRing {
public:
    bool try_push(const T& item) {
        std::size_t current_head = head_.value.load(std::memory_order_relaxed);
        std::size_t next_head = (current_head + 1) % Capacity;

        if (next_head == tail_.value.load(std::memory_order_acquire)) {
            return false;
        }

        buffer_[current_head] = item;
        head_.value.store(next_head, std::memory_order_release);
        return true;
    }

    std::optional<T> try_pop() {
        std::size_t current_tail = tail_.value.load(std::memory_order_relaxed);

        if (current_tail == head_.value.load(std::memory_order_acquire)) {
            return std::nullopt;
        }

        T item = buffer_[current_tail];
        tail_.value.store((current_tail + 1) % Capacity, std::memory_order_release);
        return item;
    }

private:
    std::array<T, Capacity> buffer_;
    PaddedIndex head_;
    PaddedIndex tail_;
};

} // namespace aether
