#include "arena/hugepage_arena.hpp"
#include "arena/per_core_freelist.hpp"
#include "arena/core_arena_pool.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <set>

using namespace aether;

TEST(HugepageArena, AllocatesRequestedSize) {
    constexpr std::size_t SIZE = 2 * 1024 * 1024;
    HugepageArena arena(SIZE);

    EXPECT_NE(arena.base(), nullptr);
    EXPECT_EQ(arena.size(), SIZE);
}

TEST(PerCoreFreelist, AllocateAndDeallocateReturnsSameAvailability) {
    constexpr std::size_t CHUNK_SIZE = 2048;
    constexpr std::size_t ARENA_SIZE = 2 * 1024 * 1024;

    HugepageArena arena(ARENA_SIZE);
    PerCoreFreelist<CHUNK_SIZE> freelist(arena.base(), arena.size());

    std::size_t initial = freelist.available();

    void* chunk = freelist.allocate();
    ASSERT_NE(chunk, nullptr);
    EXPECT_EQ(freelist.available(), initial - 1);

    freelist.deallocate(chunk);
    EXPECT_EQ(freelist.available(), initial);
}

TEST(PerCoreFreelist, AllocatedChunksAreUnique) {
    constexpr std::size_t CHUNK_SIZE = 2048;
    constexpr std::size_t ARENA_SIZE = 2 * 1024 * 1024;

    HugepageArena arena(ARENA_SIZE);
    PerCoreFreelist<CHUNK_SIZE> freelist(arena.base(), arena.size());

    std::set<void*> seen;
    for (int i = 0; i < 100; i++) {
        void* chunk = freelist.allocate();
        ASSERT_NE(chunk, nullptr);
        EXPECT_TRUE(seen.insert(chunk).second) << "duplicate chunk pointer returned";
    }
}

TEST(PerCoreFreelist, ReturnsNullptrWhenExhausted) {
    constexpr std::size_t CHUNK_SIZE = 2048;
    constexpr std::size_t ARENA_SIZE = 4096;

    HugepageArena arena(ARENA_SIZE);
    PerCoreFreelist<CHUNK_SIZE> freelist(arena.base(), arena.size());

    ASSERT_EQ(freelist.available(), 2u);

    EXPECT_NE(freelist.allocate(), nullptr);
    EXPECT_NE(freelist.allocate(), nullptr);
    EXPECT_EQ(freelist.allocate(), nullptr);
}

TEST(CoreArenaPool, EachCoreGetsIndependentFreelist) {
    constexpr std::size_t CHUNK_SIZE = 2048;
    constexpr std::size_t ARENA_SIZE = 2 * 1024 * 1024;

    CoreArenaPool<CHUNK_SIZE> pool(4, ARENA_SIZE);
    ASSERT_EQ(pool.num_cores(), 4u);

    std::size_t initial = pool.freelist_for(0).available();

    void* chunk = pool.freelist_for(0).allocate();
    ASSERT_NE(chunk, nullptr);

    EXPECT_EQ(pool.freelist_for(0).available(), initial - 1);
    EXPECT_EQ(pool.freelist_for(1).available(), initial);
}

TEST(CoreArenaPool, PinnedThreadsUseOwnFreelistConcurrently) {
    constexpr std::size_t CHUNK_SIZE = 2048;
    constexpr std::size_t ARENA_SIZE = 2 * 1024 * 1024;
    constexpr std::size_t NUM_CORES = 4;

    CoreArenaPool<CHUNK_SIZE> pool(NUM_CORES, ARENA_SIZE);
    std::size_t initial = pool.freelist_for(0).available();

    std::vector<std::thread> workers;
    for (std::size_t core = 0; core < NUM_CORES; core++) {
        workers.emplace_back([&pool, core]() {
            pool.pin_and_bind(core);
            auto& fl = pool.freelist_for(core);
            void* chunk = fl.allocate();
            ASSERT_NE(chunk, nullptr);
            fl.deallocate(chunk);
        });
    }
    for (auto& t : workers) t.join();

    for (std::size_t core = 0; core < NUM_CORES; core++) {
        EXPECT_EQ(pool.freelist_for(core).available(), initial);
    }
}
