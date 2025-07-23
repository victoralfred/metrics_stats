#include "mem_stats.hpp"
#include <gtest/gtest.h>
#include <stdexcept>
#include <iostream>
#include <sstream>

TEST(SystemMemoryStatsTest, ParsesValidMeminfo) {
    // Sample /proc/meminfo lines (some fields)
    std::string meminfo = R"(
    MemTotal:       16384256 kB
    MemFree:         1234567 kB
    MemAvailable:    2345678 kB
    Buffers:          345678 kB
    Cached:          4567890 kB
    )";
    // Create an input stream from the string
    std::istringstream input(meminfo);

    auto stats =  SystemMemoryStats::MeMStatsReader::getMemStats(input);

    EXPECT_EQ(stats.total, 16384256u);
    EXPECT_EQ(stats.free, 1234567u);
    EXPECT_EQ(stats.available, 2345678u);
    EXPECT_EQ(stats.buffers, 345678u);
    EXPECT_EQ(stats.cached, 4567890u);
}

TEST(SystemMemoryStatsTest, ThrowsOnEmptyInput) {
    std::istringstream emptyInput("");

    EXPECT_THROW( SystemMemoryStats::MeMStatsReader::getMemStats(emptyInput), std::runtime_error);
}

TEST(SystemMemoryStatsTest, ThrowsOnBadStream) {
    std::istringstream badInput;
    badInput.setstate(std::ios::badbit);  // simulate bad stream

    EXPECT_THROW( SystemMemoryStats::MeMStatsReader::getMemStats(badInput), std::runtime_error);
}