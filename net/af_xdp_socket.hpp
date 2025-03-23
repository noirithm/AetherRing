#pragma once
#include <linux/if_xdp.h>
#include <linux/if_link.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <net/if.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace aether {

// Minimal AF_XDP socket wrapper: sets up UMEM (the shared packet buffer
// region) and binds an AF_XDP socket to a network interface. This is
// stage 1 -- proves the kernel accepts the setup before building the
// real fill/completion/rx/tx ring packet path on top of it.
class AfXdpSocket {
public:
    AfXdpSocket(const char* ifname, std::size_t umem_size = 4 * 1024 * 1024,
                std::size_t frame_size = 2048) {
        ifindex_ = if_nametoindex(ifname);
        if (ifindex_ == 0) {
            throw std::runtime_error(std::string("if_nametoindex failed for ") + ifname);
        }

        umem_area_ = mmap(nullptr, umem_size, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (umem_area_ == MAP_FAILED) {
            throw std::runtime_error(std::string("mmap for UMEM failed: ") + strerror(errno));
        }
        umem_size_ = umem_size;
        frame_size_ = frame_size;

        sock_fd_ = socket(AF_XDP, SOCK_RAW, 0);
        if (sock_fd_ < 0) {
            throw std::runtime_error(std::string("socket(AF_XDP) failed: ") + strerror(errno));
        }

        struct xdp_umem_reg umem_reg{};
        umem_reg.addr = reinterpret_cast<uint64_t>(umem_area_);
        umem_reg.len = umem_size;
        umem_reg.chunk_size = static_cast<uint32_t>(frame_size);
        umem_reg.headroom = 0;

        if (setsockopt(sock_fd_, SOL_XDP, XDP_UMEM_REG, &umem_reg, sizeof(umem_reg)) < 0) {
            throw std::runtime_error(std::string("XDP_UMEM_REG failed: ") + strerror(errno));
        }

        int fill_ring_size = 2048;
        int comp_ring_size = 2048;
        if (setsockopt(sock_fd_, SOL_XDP, XDP_UMEM_FILL_RING, &fill_ring_size, sizeof(int)) < 0) {
            throw std::runtime_error(std::string("XDP_UMEM_FILL_RING failed: ") + strerror(errno));
        }
        if (setsockopt(sock_fd_, SOL_XDP, XDP_UMEM_COMPLETION_RING, &comp_ring_size, sizeof(int)) < 0) {
            throw std::runtime_error(std::string("XDP_UMEM_COMPLETION_RING failed: ") + strerror(errno));
        }

        int rx_ring_size = 2048;
        int tx_ring_size = 2048;
        if (setsockopt(sock_fd_, SOL_XDP, XDP_RX_RING, &rx_ring_size, sizeof(int)) < 0) {
            throw std::runtime_error(std::string("XDP_RX_RING failed: ") + strerror(errno));
        }
        if (setsockopt(sock_fd_, SOL_XDP, XDP_TX_RING, &tx_ring_size, sizeof(int)) < 0) {
            throw std::runtime_error(std::string("XDP_TX_RING failed: ") + strerror(errno));
        }

        std::cout << "AF_XDP socket created and UMEM registered for interface "
                  << ifname << " (ifindex " << ifindex_ << ")\n";
    }

    ~AfXdpSocket() {
        if (sock_fd_ >= 0) close(sock_fd_);
        if (umem_area_ != MAP_FAILED && umem_area_ != nullptr) {
            munmap(umem_area_, umem_size_);
        }
    }

    AfXdpSocket(const AfXdpSocket&) = delete;
    AfXdpSocket& operator=(const AfXdpSocket&) = delete;

    void bind_to_queue(uint32_t queue_id) {
        struct sockaddr_xdp sxdp{};
        sxdp.sxdp_family = AF_XDP;
        sxdp.sxdp_ifindex = ifindex_;
        sxdp.sxdp_queue_id = queue_id;
        sxdp.sxdp_flags = XDP_COPY; // start with copy mode; zero-copy needs driver support

        if (bind(sock_fd_, reinterpret_cast<sockaddr*>(&sxdp), sizeof(sxdp)) < 0) {
            throw std::runtime_error(std::string("AF_XDP bind failed: ") + strerror(errno));
        }

        std::cout << "AF_XDP socket bound to queue " << queue_id << " (copy mode)\n";
    }

    int fd() const { return sock_fd_; }

private:
    int sock_fd_ = -1;
    void* umem_area_ = MAP_FAILED;
    std::size_t umem_size_ = 0;
    std::size_t frame_size_ = 0;
    unsigned ifindex_ = 0;
};

} // namespace aether
