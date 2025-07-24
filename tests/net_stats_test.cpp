#include <gtest/gtest.h>
#include <sstream>
#include "net_stats.hpp"

TEST(NetworkStatsReaderTest, ParsesValidNetStats) {
    std::string mockNetDev =
        "Inter-|   Receive                                                |  Transmit\n"
        " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
        "  lo: 12345678  100     0    0    0    0     0          0        9876543  100     0    0    0    0     0       0\n"
        "eth0: 22334455  234     0    0    0    0     0          0        99887766  321     0    0    0    0     0       0\n";

    std::istringstream input(mockNetDev);
    auto stats = SystemNetworkStats::NetworkStatsReader::getNetStats(input);

    EXPECT_EQ(stats.rx_bytes, 22334455ULL);
    EXPECT_EQ(stats.tx_bytes, 99887766ULL);
    EXPECT_DOUBLE_EQ(stats.rx_rate, 0.0); // As per placeholder
    EXPECT_DOUBLE_EQ(stats.tx_rate, 0.0); // As per placeholder
}

TEST(NetworkStatsReaderTest, ThrowsIfStreamInvalid) {
    std::istringstream input;
    input.setstate(std::ios::failbit);  // Simulate bad stream

    EXPECT_THROW({
        SystemNetworkStats::NetworkStatsReader::getNetStats(input);
    }, std::runtime_error);
}

TEST(NetworkStatsReaderTest, ThrowsIfInterfaceNotFound) {
    std::string mockNetDev =
        "Inter-|   Receive                                                |  Transmit\n"
        " face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed\n"
        "wlan0: 11223344  150     0    0    0    0     0          0        55667788  200     0    0    0    0     0       0\n";

    std::istringstream input(mockNetDev);
    EXPECT_THROW({
        SystemNetworkStats::NetworkStatsReader::getNetStats(input);
    }, std::runtime_error);
}
TEST(NetworkStatsReaderTest, HandlesEmptyStream) {
    std::istringstream input;
    EXPECT_THROW({
        SystemNetworkStats::NetworkStatsReader::getNetStats(input);
    }, std::runtime_error);
}