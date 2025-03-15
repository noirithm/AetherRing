add_executable(ring_tests
    ring_test.cpp
)

add_executable(arena_tests
    arena_test.cpp
)

foreach(target ring_tests arena_tests)
    target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(${target} PRIVATE GTest::gtest_main)
endforeach()

include(GoogleTest)
gtest_discover_tests(ring_tests)
gtest_discover_tests(arena_tests)
