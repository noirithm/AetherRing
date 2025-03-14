#pragma once
#include "hugepage_arena.hpp"
#include "per_core_freelist.hpp"
#include "pinned_worker.hpp"
#include <vector>
#include <memory>
#include <stdexcept>

namespace aether {

template <std::size_t ChunkSize>
class CoreArenaPool {
public:
    CoreArenaPool(std::size_t num_cores, std::size_t per_core_arena_size)
        : num_cores_(num_cores) {
        arenas_.reserve(num_cores);
        freelists_.reserve(num_cores);

        for (std::size_t i = 0; i < num_cores; i++) {
            arenas_.push_back(std::make_unique<HugepageArena>(per_core_arena_size));
            freelists_.push_back(
                std::make_unique<PerCoreFreelist<ChunkSize>>(
                    arenas_[i]->base(), arenas_[i]->size()));
        }
    }

    void pin_and_bind(std::size_t core_id) {
        if (core_id >= num_cores_) {
            throw std::out_of_range("core_id exceeds pool size");
        }
        pin_current_thread(static_cast<int>(core_id));
    }

    PerCoreFreelist<ChunkSize>& freelist_for(std::size_t core_id) {
        if (core_id >= num_cores_) {
            throw std::out_of_range("core_id exceeds pool size");
        }
        return *freelists_[core_id];
    }

    std::size_t num_cores() const { return num_cores_; }

private:
    std::size_t num_cores_;
    std::vector<std::unique_ptr<HugepageArena>> arenas_;
    std::vector<std::unique_ptr<PerCoreFreelist<ChunkSize>>> freelists_;
};

} // namespace aether
