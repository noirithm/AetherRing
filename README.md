# AetherRing

A lock-free, user-space packet engine written in C++23.

## What this is

AetherRing explores high-performance, lock-free systems programming:
a single-producer/single-consumer ring buffer, a hugepage-backed memory
arena with per-core freelists, and two networking backends (io_uring
and AF_XDP) for moving packets with minimal copying and minimal syscall
overhead.

This is a learning project — built incrementally, with design notes
and correctness tests alongside the implementation.

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Status

Early development — SPSC ring buffer in progress.

## License

TBD
