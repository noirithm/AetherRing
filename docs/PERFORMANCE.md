# AetherRing — Performance Notes

Numbers below are from this machine; absolute values aren't portable
across hardware, but the relative before/after delta from each tuning
step is the useful signal. Each section is appended by the weeks 9-10
autoplan script, in order, so this file doubles as a dated log.

## Baseline (pre-tuning)

Captured 2026-10-14T04:45:07Z.

```

 Performance counter stats for './build/bench/ring_bench':

     1,005,654,398      cpu_atom/cycles/u                                                       (5.33%)
     4,112,826,634      cpu_core/cycles/u                                                       (97.57%)
       859,635,791      cpu_atom/instructions/u                                                 (5.33%)
     5,812,041,193      cpu_core/instructions/u                                                 (97.57%)
         7,810,561      cpu_atom/cache-references/u                                             (5.33%)
        24,544,606      cpu_core/cache-references/u                                             (97.57%)
             2,064      cpu_atom/cache-misses/u                                                 (5.33%)
            15,098      cpu_core/cache-misses/u                                                 (97.57%)
           510,040      cpu_atom/branch-misses/u                                                (5.33%)
         2,884,767      cpu_core/branch-misses/u                                                (97.57%)

       0.857245160 seconds time elapsed

       1.691515000 seconds user
       0.007960000 seconds sys


```

## Hotspots (perf record -g, ring_bench)

```
# To display the perf.data header info, please use --header/--header-only options.
#
#
# Total Lost Samples: 0
#
# Samples: 198  of event 'cpu_atom/cycles/Pu'
# Event count (approx.): 63992909
#
#     Self  Symbol                                                                                                                                                                                                                                            IPC   [IPC Coverage]
# ........  ................................................................................................................................................................................................................................................  ....................
#
    29.73%  [.] aether::SpscRing<int, 8ul>::try_pop()                                                                                                                                                                                                         -      -            
            |          
            |--23.90%--_start
            |          __libc_start_main
            |          0x7fabb5827781
            |          main
            |          benchmark::RunSpecifiedBenchmarks()
            |          benchmark::RunSpecifiedBenchmarks(benchmark::BenchmarkReporter*, benchmark::BenchmarkReporter*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)
            |          benchmark::internal::(anonymous namespace)::RunBenchmarks(std::vector<benchmark::internal::BenchmarkInstance, std::allocator<benchmark::internal::BenchmarkInstance> > const&, benchmark::BenchmarkReporter*, benchmark::BenchmarkReporter*)
            |          benchmark::internal::BenchmarkRunner::DoOneRepetition()
            |          benchmark::internal::BenchmarkRunner::DoNIterations()
            |          benchmark::internal::(anonymous namespace)::RunInThread(benchmark::internal::BenchmarkInstance const*, long, int, benchmark::internal::ThreadManager*, benchmark::internal::PerfCountersMeasurement*, benchmark::ProfilerManager*)
            |          benchmark::internal::BenchmarkInstance::Run(long, int, benchmark::internal::ThreadTimer*, benchmark::internal::ThreadManager*, benchmark::internal::PerfCountersMeasurement*, benchmark::ProfilerManager*) const
            |          benchmark::internal::FunctionBenchmark::Run(benchmark::State&)
            |          |          
            |          |--22.39%--BM_RingPingPongLatency(benchmark::State&)
            |          |          |          
            |          |           --22.03%--aether::SpscRing<int, 8ul>::try_pop()
            |          |          
```

## After backoff tuning (SpinBackoff in ping-pong spin loops)

Captured 2026-10-14T04:45:54Z.

```

 Performance counter stats for './build/bench/ring_bench':

       863,562,289      cpu_atom/cycles/u                                                       (5.18%)
     3,600,754,994      cpu_core/cycles/u                                                       (97.71%)
       748,612,897      cpu_atom/instructions/u                                                 (5.18%)
     3,711,970,797      cpu_core/instructions/u                                                 (97.71%)
         7,281,706      cpu_atom/cache-references/u                                             (5.18%)
        23,609,435      cpu_core/cache-references/u                                             (97.71%)
             1,795      cpu_atom/cache-misses/u                                                 (5.18%)
            12,153      cpu_core/cache-misses/u                                                 (97.71%)
           383,415      cpu_atom/branch-misses/u                                                (5.18%)
         2,320,887      cpu_core/branch-misses/u                                                (97.71%)

       0.758851333 seconds time elapsed

       1.505647000 seconds user
       0.001983000 seconds sys


```
