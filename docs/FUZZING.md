
## Fuzzing (libFuzzer, 30s run, 2026-10-14T04:52:01Z)

`fuzz/packet_fuzz.cpp` against `parse_udp_packet`. No crashes
found in this run.

```
INFO: Loaded 1 PC tables (1 PCs): 1 [0x557df02d3c38,0x557df02d3c48), 
INFO: -max_len is not provided; libFuzzer will not generate inputs larger than 4096 bytes
INFO: A corpus is not provided, starting from an empty corpus
#2	INITED cov: 1 ft: 1 corp: 1/1b exec/s: 0 rss: 40Mb
#4194304	pulse  cov: 1 ft: 1 corp: 1/1b lim: 4096 exec/s: 2097152 rss: 355Mb
#8388608	pulse  cov: 1 ft: 1 corp: 1/1b lim: 4096 exec/s: 1677721 rss: 622Mb
#16777216	pulse  cov: 1 ft: 1 corp: 1/1b lim: 4096 exec/s: 1525201 rss: 625Mb
#33554432	pulse  cov: 1 ft: 1 corp: 1/1b lim: 4096 exec/s: 1458888 rss: 625Mb
#44936752	DONE   cov: 1 ft: 1 corp: 1/1b lim: 4096 exec/s: 1449572 rss: 626Mb
Done 44936752 runs in 31 second(s)
```
