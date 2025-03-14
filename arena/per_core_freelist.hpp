#pragma once
#include <cstddef>
#include <vector>
#include <stdexcept>

namespace aether {

template <std::size_t ChunkSize>
class PerCoreFreelist {
public:
    PerCoreFreelist(void* arena_base, std::size_t arena_size) {
        std::size_t chunk_count = arena_size / ChunkSize;
        free_chunks_.reserve(chunk_count);

        auto* base_bytes = static_cast<std::byte*>(arena_base);
        for (std::size_t i = 0; i < chunk_count; i++) {
            free_chunks_.push_back(base_bytes + (i * ChunkSize));
        }
    }

    void* allocate() {
        if (free_chunks_.empty()) {
            return nullptr;
        }
        void* chunk = free_chunks_.back();
        free_chunks_.pop_back();
        return chunk;
    }

    void deallocate(void* chunk) {
        free_chunks_.push_back(chunk);
    }

    std::size_t available() const { return free_chunks_.size(); }

private:
    std::vector<void*> free_chunks_;
};

} // namespace aether
