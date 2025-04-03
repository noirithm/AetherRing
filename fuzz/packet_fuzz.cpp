// libFuzzer harness for parse_udp_packet (proto/packet_header.hpp).
//
// Built SEPARATELY from the main CMake project, directly with clang:
//   clang-19 -std=c++23 -O1 -g -fsanitize=fuzzer,address -I. \
//       -o /tmp/aetherring-packet-fuzz fuzz/packet_fuzz.cpp
//   /tmp/aetherring-packet-fuzz -max_total_time=30
//
// libFuzzer calls LLVMFuzzerTestOneInput repeatedly with mutated byte
// buffers, and AddressSanitizer catches any out-of-bounds read the parser
// makes along the way -- this is what actually exercises the bounds
// checks far more thoroughly than the hand-written truncation test in
// tests/proto_test.cpp can.
#include "proto/packet_header.hpp"
#include <cstdint>
#include <cstddef>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, std::size_t size) {
    auto result = aether::parse_udp_packet(std::span<const uint8_t>(data, size));
    (void)result;
    return 0;
}
