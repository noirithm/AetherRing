#pragma once
#include <liburing.h>
#include <stdexcept>
#include <cstring>
#include <iostream>

namespace aether {

inline void uring_hello_world() {
    struct io_uring ring;

    int ret = io_uring_queue_init(8, &ring, 0);
    if (ret < 0) {
        throw std::runtime_error(std::string("io_uring_queue_init failed: ") + strerror(-ret));
    }

    struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
    if (!sqe) {
        throw std::runtime_error("failed to get sqe");
    }

    io_uring_prep_nop(sqe);

    ret = io_uring_submit(&ring);
    if (ret < 0) {
        throw std::runtime_error(std::string("io_uring_submit failed: ") + strerror(-ret));
    }

    struct io_uring_cqe* cqe;
    ret = io_uring_wait_cqe(&ring, &cqe);
    if (ret < 0) {
        throw std::runtime_error(std::string("io_uring_wait_cqe failed: ") + strerror(-ret));
    }

    std::cout << "completion received, result: " << cqe->res << "\n";

    io_uring_cqe_seen(&ring, cqe);
    io_uring_queue_exit(&ring);
}

} // namespace aether
