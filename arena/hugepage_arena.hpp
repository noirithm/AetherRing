#pragma once
#include <sys/mman.h>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace aether {

class HugepageArena {
public:
    explicit HugepageArena(std::size_t size_bytes) : size_(size_bytes) {
        base_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);

        if (base_ == MAP_FAILED) {
            base_ = mmap(nullptr, size_, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            if (base_ == MAP_FAILED) {
                throw std::runtime_error(std::string("mmap failed: ") + strerror(errno));
            }
            used_hugepages_ = false;
        } else {
            used_hugepages_ = true;
        }
    }

    ~HugepageArena() {
        if (base_ != MAP_FAILED && base_ != nullptr) {
            munmap(base_, size_);
        }
    }

    HugepageArena(const HugepageArena&) = delete;
    HugepageArena& operator=(const HugepageArena&) = delete;

    void* base() const { return base_; }
    std::size_t size() const { return size_; }
    bool used_hugepages() const { return used_hugepages_; }

private:
    void* base_;
    std::size_t size_;
    bool used_hugepages_;
};

} // namespace aether
