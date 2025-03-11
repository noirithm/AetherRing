add_executable(ring_bench
    ring_bench.cpp
)

target_include_directories(ring_bench PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(ring_bench PRIVATE
    benchmark::benchmark
)
