add_executable(ring_bench ring_bench.cpp)
add_executable(checksum_bench checksum_bench.cpp)

foreach(target ring_bench checksum_bench)
    target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(${target} PRIVATE benchmark::benchmark)
endforeach()
