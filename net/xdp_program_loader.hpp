#pragma once
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <net/if.h>
#include <stdexcept>
#include <string>
#include <cstring>
#include <iostream>

namespace aether {

// Loads the compiled xdp_redirect.o eBPF object, attaches it to the
// given interface, and exposes the xsks_map so AF_XDP socket fds can
// be registered into it per queue.
class XdpProgramLoader {
public:
    XdpProgramLoader(const char* obj_path, const char* ifname) {
        ifindex_ = if_nametoindex(ifname);
        if (ifindex_ == 0) {
            throw std::runtime_error(std::string("if_nametoindex failed for ") + ifname);
        }

        obj_ = bpf_object__open_file(obj_path, nullptr);
        if (!obj_) {
            throw std::runtime_error("bpf_object__open_file failed");
        }

        if (bpf_object__load(obj_)) {
            throw std::runtime_error("bpf_object__load failed");
        }

        struct bpf_program* prog = bpf_object__find_program_by_name(obj_, "xdp_redirect_prog");
        if (!prog) {
            throw std::runtime_error("could not find xdp_redirect_prog in object file");
        }
        prog_fd_ = bpf_program__fd(prog);

        struct bpf_map* map = bpf_object__find_map_by_name(obj_, "xsks_map");
        if (!map) {
            throw std::runtime_error("could not find xsks_map in object file");
        }
        xsks_map_fd_ = bpf_map__fd(map);

        // attach the program to the interface (generic XDP mode, since
        // native/driver XDP support is NIC-driver-dependent and we're
        // on WiFi, which generally only supports generic mode)
        int ret = bpf_xdp_attach(static_cast<int>(ifindex_), prog_fd_,
                                  XDP_FLAGS_SKB_MODE, nullptr);
        if (ret < 0) {
            throw std::runtime_error(std::string("bpf_xdp_attach failed: ") + strerror(-ret));
        }

        std::cout << "XDP program attached to interface (ifindex " << ifindex_ << ")\n";
    }

    ~XdpProgramLoader() {
        if (ifindex_ != 0) {
            bpf_xdp_detach(static_cast<int>(ifindex_), XDP_FLAGS_SKB_MODE, nullptr);
        }
        if (obj_) {
            bpf_object__close(obj_);
        }
    }

    XdpProgramLoader(const XdpProgramLoader&) = delete;
    XdpProgramLoader& operator=(const XdpProgramLoader&) = delete;

    // registers an AF_XDP socket's fd into the xsks_map at the given queue index
    void register_socket(uint32_t queue_id, int sock_fd) {
        if (bpf_map_update_elem(xsks_map_fd_, &queue_id, &sock_fd, 0) < 0) {
            throw std::runtime_error(std::string("bpf_map_update_elem failed: ") + strerror(errno));
        }
    }

private:
    struct bpf_object* obj_ = nullptr;
    int prog_fd_ = -1;
    int xsks_map_fd_ = -1;
    unsigned ifindex_ = 0;
};

} // namespace aether
