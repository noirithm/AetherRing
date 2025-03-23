#pragma once
#include <linux/if_xdp.h>
#include <sys/mman.h>
#include <cstring>
#include <stdexcept>
#include <atomic>

namespace aether {

// Wraps one AF_XDP ring (fill, completion, rx, or tx). Each ring is a
// lock-free single-producer/single-consumer queue between kernel and
// userspace, communicated via mmap'd shared memory and atomic producer/
// consumer index pointers -- conceptually the same SPSC pattern as your
// own ring buffer, just implemented by the kernel instead of by you.
struct XdpRing {
    void* map = nullptr;
    std::size_t map_size = 0;
    std::atomic<uint32_t>* producer = nullptr;
    std::atomic<uint32_t>* consumer = nullptr;
    uint32_t* ring = nullptr;     // for fill/completion rings: array of uint64_t frame addrs
                                    // for rx/tx rings: array of xdp_desc structs (cast separately)
    uint32_t mask = 0;
};

inline void map_ring(int sock_fd, XdpRing& r, int64_t off, uint32_t off_type,
                      uint32_t ring_size, off_t mmap_offset) {
    r.map_size = off + ring_size * sizeof(struct xdp_desc);
    r.map = mmap(nullptr, r.map_size, PROT_READ | PROT_WRITE,
                 MAP_SHARED | MAP_POPULATE, sock_fd, mmap_offset);
    if (r.map == MAP_FAILED) {
        throw std::runtime_error("mmap of AF_XDP ring failed");
    }
    r.mask = ring_size - 1;
    (void)off_type;
}

} // namespace aether
