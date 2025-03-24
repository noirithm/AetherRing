#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

// XSKMAP: a special map type holding AF_XDP socket file descriptors,
// indexed by queue ID. The userspace loader populates this map with
// the fd of the AF_XDP socket it created for each queue.
struct {
    __uint(type, BPF_MAP_TYPE_XSKMAP);
    __uint(max_entries, 64);
    __uint(key_size, sizeof(int));
    __uint(value_size, sizeof(int));
} xsks_map SEC(".maps");

SEC("xdp")
int xdp_redirect_prog(struct xdp_md *ctx) {
    int index = ctx->rx_queue_index;

    // bpf_redirect_map: redirects this packet into the AF_XDP socket
    // registered in xsks_map at this queue's index, instead of letting
    // it continue through the normal kernel network stack.
    if (bpf_map_lookup_elem(&xsks_map, &index)) {
        return bpf_redirect_map(&xsks_map, index, 0);
    }

    // no AF_XDP socket registered for this queue -- let the packet
    // pass through normally
    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
