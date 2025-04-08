# Fuzzing

The packet parser (`proto/packet_header.hpp`) is fuzzed with libFuzzer, AddressSanitizer and UBSan.

## Build and run

```sh
clang++-19 -std=c++23 -O1 -g -fsanitize=fuzzer,address,undefined -I. \
    -o packet_fuzz fuzz/packet_fuzz.cpp
./packet_fuzz fuzz/corpus -runs=2000000 -print_final_stats=1
```

For a time-bounded run use `-max_total_time=30` instead of `-runs`.

## What the harness checks

- Every input goes through `parse_udp_packet`, and the result is consumed so the compiler cannot optimize the call away.
- On a successful parse, the payload must lie entirely inside the input buffer, and every payload byte is read so ASan checks the full range.

## Latest result (clang-19, x86-64)

| Metric | Value |
|---|---|
| Executions | 2,000,000 in 3 s (about 660k exec/s) |
| Coverage | `cov: 37` of 64 inline counters, `ft: 63` |
| Corpus | 20 inputs, 931 bytes |
| Crashes / sanitizer findings | none |

A longer 30 s run (20.4M executions) found no new coverage, so the parser's reachable paths are saturated for this input size. Coverage reads `cov: 1` only if the harness is being optimized away, which was a bug in an earlier version of the harness.

## Corpus

`fuzz/corpus/` holds two hand-built seed packets (a plain Ethernet/IPv4/UDP packet and one with IPv4 options, IHL=6) plus inputs the fuzzer found. To shrink it to the minimal set with the same coverage:

```sh
mkdir -p fuzz/corpus_min
./packet_fuzz -merge=1 fuzz/corpus_min fuzz/corpus
rm -rf fuzz/corpus && mv fuzz/corpus_min fuzz/corpus
```
