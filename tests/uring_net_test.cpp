#include "net/uring_multishot_recv.hpp"
#include "net/uring_zerocopy_send.hpp"
#include <gtest/gtest.h>
#include <thread>
#include <string>
#include <vector>
#include <chrono>

using namespace aether;

TEST(UringNet, SendAndReceiveRoundTrip) {
    constexpr uint16_t PORT = 9876;
    const std::string message = "round trip test message";

    std::vector<uint8_t> received_data;
    bool received_flag = false;

    // receiver runs on its own thread, since receive_loop() blocks
    std::thread receiver_thread([&]() {
        UringMultishotReceiver receiver(PORT);
        receiver.start();

        receiver.receive_loop(1, [&](const uint8_t* data, std::size_t len) {
            received_data.assign(data, data + len);
            received_flag = true;
        });
    });

    // give the receiver a moment to bind and start listening
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    UringZeroCopySender sender;
    sender.send_zc(reinterpret_cast<const uint8_t*>(message.data()), message.size(),
                    "127.0.0.1", PORT);

    receiver_thread.join();

    ASSERT_TRUE(received_flag);
    std::string received_str(received_data.begin(), received_data.end());
    EXPECT_EQ(received_str, message);
}

TEST(UringNet, ReceiverGetsCorrectByteLength) {
    constexpr uint16_t PORT = 9877;
    const std::string message = "x";  // single byte, edge case

    std::size_t received_len = 0;
    bool received_flag = false;

    std::thread receiver_thread([&]() {
        UringMultishotReceiver receiver(PORT);
        receiver.start();

        receiver.receive_loop(1, [&](const uint8_t*, std::size_t len) {
            received_len = len;
            received_flag = true;
        });
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    UringZeroCopySender sender;
    sender.send_zc(reinterpret_cast<const uint8_t*>(message.data()), message.size(),
                    "127.0.0.1", PORT);

    receiver_thread.join();

    ASSERT_TRUE(received_flag);
    EXPECT_EQ(received_len, message.size());
}
