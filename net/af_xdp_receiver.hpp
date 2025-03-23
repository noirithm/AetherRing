#pragma once
#include "af_xdp_socket.hpp"
#include "af_xdp_rings.hpp"
#include <linux/if_xdp.h>
#include <sys/mman.h>
#include <functional>
#include <iostream>
#include <cstring>

namespace aether {

class AfXdpReceiver {
public:
    AfXdpReceiver(const char* ifname, uint32_t queue_id,
                  std::size_t num_frames = 2048, std::size_t frame_size = 2048)
        : socket_(ifname, num_frames * frame_size, frame_size),
          num_frames_(num_frames), frame_size_(frame_size) {

        socket_.bind_to_queue(queue_id);
        map_all_rings();
        populate_fill_ring();
    }

    // Polls the RX ring for available packets, calling on_packet for each.
    // Returns the number of packets actually processed (may be 0, this
    // is non-blocking -- caller decides whether/how to wait or retry).
    std::size_t poll_once(const std::function<void(const uint8_t*, std::size_t)>& on_packet) {
        uint32_t rx_cons = rx_.consumer->load(std::memory_order_acquire);
        uint32_t rx_prod = rx_.producer->load(std::memory_order_acquire);

        std::size_t processed = 0;
        auto* descs = reinterpret_cast<struct xdp_desc*>(rx_.ring);

        while (rx_cons != rx_prod) {
            struct xdp_desc& desc = descs[rx_cons & rx_.mask];
            auto* frame = static_cast<uint8_t*>(umem_base_) + desc.addr;

            on_packet(frame, desc.len);

            // return this frame to the fill ring so the kernel can reuse it
            uint32_t fill_prod = fill_.producer->load(std::memory_order_relaxed);
            auto* fill_ring = reinterpret_cast<uint64_t*>(fill_.ring);
            fill_ring[fill_prod & fill_.mask] = desc.addr;
            fill_.producer->store(fill_prod + 1, std::memory_order_release);

            rx_cons++;
            processed++;
        }

        rx_.consumer->store(rx_cons, std::memory_order_release);
        return processed;
    }

private:
    void map_all_rings() {
        struct xdp_mmap_offsets offsets{};
        socklen_t len = sizeof(offsets);
        if (getsockopt(socket_.fd(), SOL_XDP, XDP_MMAP_OFFSETS, &offsets, &len) < 0) {
            throw std::runtime_error(std::string("XDP_MMAP_OFFSETS failed: ") + strerror(errno));
        }

        constexpr uint32_t RING_SIZE = 2048;

        // fill ring
        fill_.map_size = offsets.fr.desc + RING_SIZE * sizeof(uint64_t);
        fill_.map = mmap(nullptr, fill_.map_size, PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_POPULATE, socket_.fd(), XDP_UMEM_PGOFF_FILL_RING);
        if (fill_.map == MAP_FAILED) throw std::runtime_error("mmap fill ring failed");
        fill_.producer = reinterpret_cast<std::atomic<uint32_t>*>(
            static_cast<uint8_t*>(fill_.map) + offsets.fr.producer);
        fill_.consumer = reinterpret_cast<std::atomic<uint32_t>*>(
            static_cast<uint8_t*>(fill_.map) + offsets.fr.consumer);
        fill_.ring = reinterpret_cast<uint32_t*>(
            static_cast<uint8_t*>(fill_.map) + offsets.fr.desc);
        fill_.mask = RING_SIZE - 1;

        // completion ring
        comp_.map_size = offsets.cr.desc + RING_SIZE * sizeof(uint64_t);
        comp_.map = mmap(nullptr, comp_.map_size, PROT_READ | PROT_WRITE,
                          MAP_SHARED | MAP_POPULATE, socket_.fd(), XDP_UMEM_PGOFF_COMPLETION_RING);
        if (comp_.map == MAP_FAILED) throw std::runtime_error("mmap completion ring failed");
        comp_.producer = reinterpret_cast<std::atomic<uint32_t>*>(
            static_cast<uint8_t*>(comp_.map) + offsets.cr.producer);
        comp_.consumer = reinterpret_cast<std::atomic<uint32_t>*>(
            static_cast<uint8_t*>(comp_.map) + offsets.cr.consumer);
        comp_.ring = reinterpret_cast<uint32_t*>(
            static_cast<uint8_t*>(comp_.map) + offsets.cr.desc);
        comp_.mask = RING_SIZE - 1;

        // rx ring
        rx_.map_size = offsets.rx.desc + RING_SIZE * sizeof(struct xdp_desc);
        rx_.map = mmap(nullptr, rx_.map_size, PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_POPULATE, socket_.fd(), XDP_PGOFF_RX_RING);
        if (rx_.map == MAP_FAILED) throw std::runtime_error("mmap rx ring failed");
        rx_.producer = reinterpret_cast<std::atomic<uint32_t>*>(
            static_cast<uint8_t*>(rx_.map) + offsets.rx.producer);
        rx_.consumer = reinterpret_cast<std::atomic<uint32_t>*>(
            static_cast<uint8_t*>(rx_.map) + offsets.rx.consumer);
        rx_.ring = reinterpret_cast<uint32_t*>(
            static_cast<uint8_t*>(rx_.map) + offsets.rx.desc);
        rx_.mask = RING_SIZE - 1;

        umem_base_ = socket_.umem_base();
    }

    void populate_fill_ring() {
        uint32_t prod = fill_.producer->load(std::memory_order_relaxed);
        auto* fill_ring = reinterpret_cast<uint64_t*>(fill_.ring);

        for (std::size_t i = 0; i < num_frames_; i++) {
            fill_ring[(prod + i) & fill_.mask] = i * frame_size_;
        }
        fill_.producer->store(static_cast<uint32_t>(prod + num_frames_), std::memory_order_release);
    }

    AfXdpSocket socket_;
    XdpRing fill_, comp_, rx_, tx_;
    void* umem_base_ = nullptr;
    std::size_t num_frames_;
    std::size_t frame_size_;
};

} // namespace aether
