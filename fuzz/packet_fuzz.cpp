// libFuzzer harness for parse_udp_packet (proto/packet_header.hpp).
//
// Build separately from the main CMake project, with clang:
//   clang++ -std=c++23 -O1 -g -fsanitize=fuzzer,address,undefined -I. \
//       -o packet_fuzz fuzz/packet_fuzz.cpp
//   ./packet_fuzz fuzz/corpus -max_total_time=60
//
// Two things make this harness meaningful:
//
// 1. The parse result is CONSUMED (see `sink`). parse_udp_packet is inline
//    and side-effect free, so if the result were discarded the compiler
//    could delete the whole call, leaving libFuzzer with a single
//    instrumented PC ("cov: 1") and nothing to find.
//
// 2. A successful parse is checked against its own contract: the payload
//    must lie entirely inside the input buffer. ASan catches out-of-bounds
//    *reads*; this catches wrong *answers*.
//
// Healthy run: `cov:` should climb to dozens of edges within seconds.
// A seed packet lives in fuzz/corpus/ so the fuzzer starts past the
// Ethernet/IPv4 checks instead of rediscovering them from nothing.
#include "proto/packet_header.hpp"
#include <cstdint>
#include <cstddef>
#include <span>

namespace {
// Opaque use of a value: the compiler must assume it is read.
inline void sink(std::uint64_t v) {
    asm volatile("" : : "r"(v) : "memory");
}
} // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    auto result = aether::parse_udp_packet(std::span<const std::uint8_t>(data, size));

    if (!result) {
        sink(static_cast<std::uint64_t>(result.error()));
        return 0;
    }

    const aether::ParsedPacket& p = *result;

    // Contract: payload is a sub-range of the input.
    const std::uint8_t* pay_begin = p.payload.data();
    const std::uint8_t* pay_end   = pay_begin + p.payload.size();
    if (pay_begin < data || pay_end > data + size) {
        __builtin_trap();
    }

    // Touch every payload byte so ASan checks the whole range.
    std::uint64_t acc = p.ip.src_ip ^ p.ip.dst_ip ^ p.udp.src_port ^ p.udp.dst_port;
    for (std::uint8_t b : p.payload) acc += b;
    sink(acc);
    return 0;
}
