#pragma once
#include <liburing.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <iostream>

namespace aether {

class UringZeroCopySender {
public:
    UringZeroCopySender() {
        int ret = io_uring_queue_init(256, &ring_, 0);
        if (ret < 0) {
            throw std::runtime_error(std::string("queue_init failed: ") + strerror(-ret));
        }

        sock_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_fd_ < 0) {
            throw std::runtime_error(std::string("socket() failed: ") + strerror(errno));
        }
    }

    ~UringZeroCopySender() {
        if (sock_fd_ >= 0) close(sock_fd_);
        io_uring_queue_exit(&ring_);
    }

    UringZeroCopySender(const UringZeroCopySender&) = delete;
    UringZeroCopySender& operator=(const UringZeroCopySender&) = delete;

    // Sends `data` to the given address using IORING_OP_SEND_ZC (zero-copy send).
    // Blocks until both the SEND completion and the (separate) notification
    // completion arrive, confirming the kernel is done with the buffer.
    void send_zc(const uint8_t* data, std::size_t len, const char* dest_ip, uint16_t dest_port) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(dest_port);
        inet_pton(AF_INET, dest_ip, &addr.sin_addr);

        struct io_uring_sqe* sqe = io_uring_get_sqe(&ring_);
        if (!sqe) {
            throw std::runtime_error("failed to get sqe for send_zc");
        }

        io_uring_prep_send_zc(sqe, sock_fd_, data, len, 0, 0);
        io_uring_sqe_set_data(sqe, nullptr);

        // send_zc actually needs the destination wired via sendto-style prep;
        // liburing's prep_send_zc doesn't take a dest addr directly, so we
        // connect() once up front instead for UDP "destination" semantics.
        if (connect(sock_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            throw std::runtime_error(std::string("connect() failed: ") + strerror(errno));
        }

        int ret = io_uring_submit(&ring_);
        if (ret < 0) {
            throw std::runtime_error(std::string("submit failed: ") + strerror(-ret));
        }

        // IORING_OP_SEND_ZC produces TWO completions:
        // 1. the initial completion (data handed to kernel)
        // 2. a notification completion (IORING_CQE_F_NOTIF) once the kernel
        //    is fully done referencing the buffer (safe to reuse/free now)
        bool got_send_completion = false;
        bool got_notif_completion = false;

        while (!got_send_completion || !got_notif_completion) {
            struct io_uring_cqe* cqe;
            int wret = io_uring_wait_cqe(&ring_, &cqe);
            if (wret < 0) {
                throw std::runtime_error(std::string("wait_cqe failed: ") + strerror(-wret));
            }

            if (cqe->flags & IORING_CQE_F_NOTIF) {
                got_notif_completion = true;
            } else {
                if (cqe->res < 0) {
                    throw std::runtime_error(std::string("send_zc failed: ") + strerror(-cqe->res));
                }
                got_send_completion = true;
            }

            io_uring_cqe_seen(&ring_, cqe);
        }
    }

private:
    struct io_uring ring_;
    int sock_fd_ = -1;
};

} // namespace aether
