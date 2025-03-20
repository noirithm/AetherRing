#pragma once
#include <liburing.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <functional>

namespace aether {

class UringMultishotReceiver {
public:
    UringMultishotReceiver(uint16_t port, std::size_t buf_count = 64, std::size_t buf_size = 2048)
        : buf_count_(buf_count), buf_size_(buf_size) {

        int ret = io_uring_queue_init(256, &ring_, 0);
        if (ret < 0) {
            throw std::runtime_error(std::string("queue_init failed: ") + strerror(-ret));
        }

        sock_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_fd_ < 0) {
            throw std::runtime_error(std::string("socket() failed: ") + strerror(errno));
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(sock_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            throw std::runtime_error(std::string("bind() failed: ") + strerror(errno));
        }

        setup_buffer_ring();
    }

    ~UringMultishotReceiver() {
        if (sock_fd_ >= 0) close(sock_fd_);
        io_uring_queue_exit(&ring_);
    }

    UringMultishotReceiver(const UringMultishotReceiver&) = delete;
    UringMultishotReceiver& operator=(const UringMultishotReceiver&) = delete;

    void start() {
        struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
        if (!sqe) {
            throw std::runtime_error("failed to get sqe for multishot recv");
        }

        io_uring_prep_recv_multishot(sqe, sock_fd_, nullptr, 0, 0);
        sqe->flags |= IOSQE_BUFFER_SELECT;
        sqe->buf_group = buf_group_id_;

        int ret = io_uring_submit(&ring_);
        if (ret < 0) {
            throw std::runtime_error(std::string("submit failed: ") + strerror(-ret));
        }
    }

    // calls `on_packet(data, length)` for each packet received, up to `max_packets` times
    void receive_loop(std::size_t max_packets,
                       const std::function<void(const uint8_t*, std::size_t)>& on_packet) {
        std::size_t received = 0;

        while (received < max_packets) {
            struct io_uring_cqe* cqe;
            int ret = io_uring_wait_cqe(&ring_, &cqe);
            if (ret < 0) {
                throw std::runtime_error(std::string("wait_cqe failed: ") + strerror(-ret));
            }

            if (cqe->res < 0) {
                std::cerr << "recv error: " << strerror(-cqe->res) << "\n";
                io_uring_cqe_seen(&ring_, cqe);
                continue;
            }

            int buf_id = cqe->flags >> IORING_CQE_BUFFER_SHIFT;
            uint8_t* data = buffer_base_ + (static_cast<std::size_t>(buf_id) * buf_size_);
            on_packet(data, static_cast<std::size_t>(cqe->res));

            // return the buffer to the ring for reuse
            io_uring_buf_ring_add(buf_ring_, data, static_cast<unsigned>(buf_size_),
                                   buf_id, io_uring_buf_ring_mask(static_cast<unsigned>(buf_count_)), 0);
            io_uring_buf_ring_advance(buf_ring_, 1);

            io_uring_cqe_seen(&ring_, cqe);
            received++;

            // IORING_CQE_F_MORE means the kernel will keep sending completions
            // without needing a new submission (that's the "multishot" part)
            if (!(cqe->flags & IORING_CQE_F_MORE)) {
                start(); // multishot ended unexpectedly, resubmit
            }
        }
    }

    int port_fd() const { return sock_fd_; }

private:
    void setup_buffer_ring() {
        buffer_base_ = static_cast<uint8_t*>(
            std::aligned_alloc(4096, buf_count_ * buf_size_));
        if (!buffer_base_) {
            throw std::runtime_error("failed to allocate buffer ring memory");
        }

        int ret;
        buf_ring_ = io_uring_setup_buf_ring(&ring_, static_cast<unsigned>(buf_count_),
                                             buf_group_id_, 0, &ret);
        if (!buf_ring_) {
            throw std::runtime_error(std::string("setup_buf_ring failed: ") + strerror(-ret));
        }

        unsigned mask = io_uring_buf_ring_mask(static_cast<unsigned>(buf_count_));
        for (std::size_t i = 0; i < buf_count_; i++) {
            io_uring_buf_ring_add(buf_ring_, buffer_base_ + (i * buf_size_),
                                   static_cast<unsigned>(buf_size_),
                                   static_cast<int>(i), mask, static_cast<int>(i));
        }
        io_uring_buf_ring_advance(buf_ring_, static_cast<int>(buf_count_));
    }

    struct io_uring ring_;
    int sock_fd_ = -1;
    struct io_uring_buf_ring* buf_ring_ = nullptr;
    uint8_t* buffer_base_ = nullptr;
    std::size_t buf_count_;
    std::size_t buf_size_;
    static constexpr int buf_group_id_ = 1;
};

} // namespace aether
