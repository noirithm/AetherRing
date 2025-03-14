#pragma once
#include <pthread.h>
#include <sched.h>
#include <thread>
#include <stdexcept>
#include <cstring>

namespace aether {

inline void pin_current_thread(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    int result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (result != 0) {
        throw std::runtime_error(std::string("pthread_setaffinity_np failed: ") + strerror(result));
    }
}

} // namespace aether
