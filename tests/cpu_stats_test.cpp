#include "cpu_stats.hpp"
#include <gtest/gtest.h>
#include <stdexcept>
#include <iostream>
#include <sstream>


TEST(SystemCPUStatsTest, ParsesValidCPUStatsCorrectly) {
    std::string mockStatLine = "cpu  4705 150 2300 160000 123 45 67 8 9 10\n";
    std::istringstream mockStream(mockStatLine);

    auto stats = SystemCPUStats::CPUStatsReader::getCPUStats(mockStream);

    EXPECT_DOUBLE_EQ(stats.user, 4705);
    EXPECT_DOUBLE_EQ(stats.nice, 150);
    EXPECT_DOUBLE_EQ(stats.system, 2300);
    EXPECT_DOUBLE_EQ(stats.idle, 160000);
    EXPECT_DOUBLE_EQ(stats.iowait, 123);
    EXPECT_DOUBLE_EQ(stats.irq, 45);
    EXPECT_DOUBLE_EQ(stats.softirq, 67);
    EXPECT_DOUBLE_EQ(stats.steal, 8);
    EXPECT_DOUBLE_EQ(stats.guest, 9);
    EXPECT_DOUBLE_EQ(stats.guest_nice, 10);
}

TEST(SystemCPUStatsTest, ThrowsIfNotEnoughFields) {
    std::string incompleteLine = "cpu  100 200\n";
    std::istringstream mockStream(incompleteLine);

    EXPECT_THROW(
        SystemCPUStats::CPUStatsReader::getCPUStats(mockStream),
        std::runtime_error
    );
}

TEST(SystemCPUStatsTest, ThrowsOnEmptyInput) {
    std::istringstream emptyStream("");

    EXPECT_THROW(
         SystemCPUStats::CPUStatsReader::getCPUStats(emptyStream),
        std::runtime_error
    );
}
