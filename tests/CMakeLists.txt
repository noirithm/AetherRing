add_executable(ring_tests
    ring_test.cpp
)

target_include_directories(ring_tests PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(ring_tests PRIVATE
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(ring_tests)
