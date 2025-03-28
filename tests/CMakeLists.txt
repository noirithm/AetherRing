add_executable(ring_tests
    ring_test.cpp
)

add_executable(arena_tests
    arena_test.cpp
)

add_executable(uring_net_tests
    uring_net_test.cpp
)

add_executable(checksum_tests
    checksum_test.cpp
)

add_executable(backoff_tests
    backoff_test.cpp
)

foreach(target ring_tests arena_tests uring_net_tests checksum_tests backoff_tests)
    target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(${target} PRIVATE GTest::gtest_main)
endforeach()

target_link_libraries(uring_net_tests PRIVATE PkgConfig::URING)
target_compile_options(checksum_tests PRIVATE -march=x86-64-v3)

include(GoogleTest)
gtest_discover_tests(ring_tests)
gtest_discover_tests(arena_tests)
gtest_discover_tests(uring_net_tests)
gtest_discover_tests(checksum_tests)
gtest_discover_tests(backoff_tests)
