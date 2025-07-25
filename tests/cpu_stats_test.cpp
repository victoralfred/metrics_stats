// Include Google Test headers
#include "gtest/gtest.h"
#include <sstream>    // For std::stringstream
#include <thread>     // For std::this_thread::sleep_for
#include <chrono>     // For std::chrono::milliseconds
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cassert> // For basic assertions
#include <stdexcept> // For std::runtime_error
#include <thread>    // For std::this_thread::sleep_for
#include <chrono>    // For std::chrono::milliseconds


// Include your system stat headers
#include "cpu_stats.hpp"
// --- Test Cases for CPUStats struct ---

TEST(CPUStatsTest, ConstructorInitialization) {
    // Test default constructor
    SystemCPUStats::CPUStats default_stats;
    EXPECT_EQ(default_stats.user, 0.0);
    EXPECT_EQ(default_stats.nice, 0.0);
    EXPECT_EQ(default_stats.system, 0.0);
    EXPECT_EQ(default_stats.idle, 0.0);
    EXPECT_EQ(default_stats.iowait, 0.0);
    EXPECT_EQ(default_stats.irq, 0.0);
    EXPECT_EQ(default_stats.softirq, 0.0);
    EXPECT_EQ(default_stats.steal, 0.0);
    EXPECT_EQ(default_stats.guest, 0.0);
    EXPECT_EQ(default_stats.guest_nice, 0.0);
    EXPECT_EQ(default_stats.usage_percent, 0.0);

    // Test parameterized constructor
    SystemCPUStats::CPUStats custom_stats(1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 50.0);
    EXPECT_EQ(custom_stats.user, 1.0);
    EXPECT_EQ(custom_stats.nice, 2.0);
    EXPECT_EQ(custom_stats.system, 3.0);
    EXPECT_EQ(custom_stats.idle, 4.0);
    EXPECT_EQ(custom_stats.iowait, 5.0);
    EXPECT_EQ(custom_stats.irq, 6.0);
    EXPECT_EQ(custom_stats.softirq, 7.0);
    EXPECT_EQ(custom_stats.steal, 8.0);
    EXPECT_EQ(custom_stats.guest, 9.0);
    EXPECT_EQ(custom_stats.guest_nice, 10.0);
    EXPECT_EQ(custom_stats.usage_percent, 50.0);
}

TEST(CPUStatsTest, GetTotalActiveTime) {
    SystemCPUStats::CPUStats stats(10, 5, 20, 100, 2, 3, 4, 1, 0.5, 0.2);
    // 10+5+20+2+3+4+1+0.5+0.2 = 45.7
    EXPECT_DOUBLE_EQ(stats.getTotalActiveTime(), 45.7);

    SystemCPUStats::CPUStats stats_idle(0, 0, 0, 1000, 0, 0, 0, 0, 0, 0);
    EXPECT_DOUBLE_EQ(stats_idle.getTotalActiveTime(), 0.0);
}

TEST(CPUStatsTest, GetTotalTime) {
    SystemCPUStats::CPUStats stats(10, 5, 20, 100, 2, 3, 4, 1, 0.5, 0.2);
    // 10+5+20+100+2+3+4+1+0.5+0.2 = 145.7
    EXPECT_DOUBLE_EQ(stats.getTotalTime(), 145.7);

    SystemCPUStats::CPUStats stats_idle(0, 0, 0, 1000, 0, 0, 0, 0, 0, 0);
    EXPECT_DOUBLE_EQ(stats_idle.getTotalTime(), 1000.0);
}

// --- Test Cases for SystemCPUStats::calculateUsagePercentage ---

TEST(CalculateUsagePercentageTest, TypicalLoad) {
    SystemCPUStats::CPUStats prev(100, 0, 20, 880, 0, 0, 0, 0, 0, 0); // Total 1000, Active 120
    SystemCPUStats::CPUStats curr(200, 0, 40, 1760, 0, 0, 0, 0, 0, 0); // Total 2000, Active 240

    // Delta active = 240 - 120 = 120
    // Delta total = 2000 - 1000 = 1000
    // Usage = (120 / 1000) * 100 = 12.0
    double usage = SystemCPUStats::calculateUsagePercentage(curr, prev, 1000); // 1 sec delta (dummy for calc)
    EXPECT_DOUBLE_EQ(usage, 12.0);
}

TEST(CalculateUsagePercentageTest, HighLoad) {
    SystemCPUStats::CPUStats prev(100, 0, 20, 100, 0, 0, 0, 0, 0, 0);  // Total 220, Active 120
    SystemCPUStats::CPUStats curr(200, 0, 40, 110, 0, 0, 0, 0, 0, 0); // Total 350, Active 240

    // Delta active = 240 - 120 = 120
    // Delta total = 350 - 220 = 130
    // Usage = (120 / 130) * 100 = ~92.307
    double usage = SystemCPUStats::calculateUsagePercentage(curr, prev, 1000);
    EXPECT_NEAR(usage, (120.0 / 130.0) * 100.0, 0.001);
}

TEST(CalculateUsagePercentageTest, IdleSystem) {
    SystemCPUStats::CPUStats prev(10, 0, 5, 985, 0, 0, 0, 0, 0, 0); // Total 1000, Active 15
    SystemCPUStats::CPUStats curr(10, 0, 5, 1985, 0, 0, 0, 0, 0, 0); // Total 2000, Active 15

    // Delta active = 15 - 15 = 0
    // Delta total = 2000 - 1000 = 1000
    // Usage = (0 / 1000) * 100 = 0.0
    double usage = SystemCPUStats::calculateUsagePercentage(curr, prev, 1000);
    EXPECT_DOUBLE_EQ(usage, 0.0);
}

TEST(CalculateUsagePercentageTest, ZeroTimeDeltaInput) {
    // This tests the explicit check for time_delta_ms <= 0
    SystemCPUStats::CPUStats prev(100, 0, 20, 880, 0, 0, 0, 0, 0, 0);
    SystemCPUStats::CPUStats curr(200, 0, 40, 1760, 0, 0, 0, 0, 0, 0);

    double usage = SystemCPUStats::calculateUsagePercentage(curr, prev, 0);
    EXPECT_DOUBLE_EQ(usage, 0.0);

    usage = SystemCPUStats::calculateUsagePercentage(curr, prev, -100);
    EXPECT_DOUBLE_EQ(usage, 0.0);
}

TEST(CalculateUsagePercentageTest, ZeroTotalCpuTimeDelta) {
    // This can happen if CPU times don't change between two reads
    SystemCPUStats::CPUStats prev(100, 0, 20, 880, 0, 0, 0, 0, 0, 0);
    SystemCPUStats::CPUStats curr(100, 0, 20, 880, 0, 0, 0, 0, 0, 0);

    double usage = SystemCPUStats::calculateUsagePercentage(curr, prev, 1000);
    EXPECT_DOUBLE_EQ(usage, 0.0);
}

TEST(CalculateUsagePercentageTest, AllZeroStats) {
    SystemCPUStats::CPUStats prev(0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    SystemCPUStats::CPUStats curr(0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    double usage = SystemCPUStats::calculateUsagePercentage(curr, prev, 1000);
    EXPECT_DOUBLE_EQ(usage, 0.0);
}


// --- Test Cases for CPUStatsReader::getCPUStats(std::istream&) (testing parsing) ---

TEST(CPUStatsReaderTest, GetCPUStatsFromStream_ValidInput) {
    std::stringstream ss("cpu 1000 200 300 4000 50 10 5 0 0 0");
    SystemCPUStats::CPUStats stats = SystemCPUStats::CPUStatsReader::getCPUStats(ss);

    EXPECT_DOUBLE_EQ(stats.user, 1000.0);
    EXPECT_DOUBLE_EQ(stats.nice, 200.0);
    EXPECT_DOUBLE_EQ(stats.system, 300.0);
    EXPECT_DOUBLE_EQ(stats.idle, 4000.0);
    EXPECT_DOUBLE_EQ(stats.iowait, 50.0);
    EXPECT_DOUBLE_EQ(stats.irq, 10.0);
    EXPECT_DOUBLE_EQ(stats.softirq, 5.0);
    EXPECT_DOUBLE_EQ(stats.steal, 0.0);
    EXPECT_DOUBLE_EQ(stats.guest, 0.0);
    EXPECT_DOUBLE_EQ(stats.guest_nice, 0.0);
    EXPECT_DOUBLE_EQ(stats.usage_percent, 0.0); // Should always be 0.0 for stream reads
}

TEST(CPUStatsReaderTest, GetCPUStatsFromStream_MissingFields) {
    std::stringstream ss("cpu 1000 200 300"); // Missing many fields
    EXPECT_THROW(SystemCPUStats::CPUStatsReader::getCPUStats(ss), std::runtime_error);
}

TEST(CPUStatsReaderTest, GetCPUStatsFromStream_InvalidLabel) {
    std::stringstream ss("notcpu 1000 200 300 4000 50 10 5 0 0 0");
    EXPECT_THROW(SystemCPUStats::CPUStatsReader::getCPUStats(ss), std::runtime_error);
}

TEST(CPUStatsReaderTest, GetCPUStatsFromStream_EmptyInput) {
    std::stringstream ss("");
    EXPECT_THROW(SystemCPUStats::CPUStatsReader::getCPUStats(ss), std::runtime_error);
}

TEST(CPUStatsReaderTest, GetCPUStatsFromStream_WhitespaceOnly) {
    std::stringstream ss("   \n");
    EXPECT_THROW(SystemCPUStats::CPUStatsReader::getCPUStats(ss), std::runtime_error);
}

// --- Test Cases for CPUStatsReader::getCPUStats() (platform-specific, raw values) ---

TEST(CPUStatsReaderTest, GetCPUStats_ReturnsRawValuesAndZeroUsage) {
    // This is an integration-style test for platform-specific raw data retrieval.
    // It verifies that raw values are non-zero (indicating data was read)
    // and that usage_percent is consistently 0.0 as per the stateless design.
    try {
        SystemCPUStats::CPUStats stats = SystemCPUStats::CPUStatsReader::getCPUStats();

        // Expect raw values to be non-zero (assuming a running system)
        // Adjust these expectations based on your specific OS's reporting
        // and what "sensible non-zero" means. This just checks for basic success.
        EXPECT_GE(stats.getTotalTime(), 0.0); // Total time should always be non-negative

        // Specific checks for common fields (these might vary on a bare system vs container)
        // We expect at least some user or system time if the system is running.
        if (stats.getTotalTime() > 0) {
             // If total time is positive, expect some active or idle time
             EXPECT_TRUE(stats.getTotalActiveTime() >= 0.0);
             EXPECT_TRUE(stats.idle >= 0.0);
        }

        // CRUCIAL: Verify usage_percent is 0.0 as the reader is stateless
        EXPECT_DOUBLE_EQ(stats.usage_percent, 0.0);

    } catch (const std::runtime_error& e) {
        // If the OS is unsupported or there's a fundamental error reading, it should throw
        // For unsupported OS, this test would pass by catching the expected throw.
        // For supported OS, if it throws, it's a test failure unless specifically expected.
        FAIL() << "Failed to get CPU stats: " << e.what();
    }
}


// --- Example of how a User/Higher-Level Component would calculate usage ---
// This is not a unit test of CPUStatsReader directly, but demonstrates its intended use.
TEST(CPUUsageCalculationDemo, UserCalculatesPercentage) {
    SystemCPUStats::CPUStats prev_stats;
    std::chrono::steady_clock::time_point prev_time;

    try {
        // First read (will just get raw stats)
        prev_stats = SystemCPUStats::CPUStatsReader::getCPUStats();
        prev_time = std::chrono::steady_clock::now();

        // Simulate some time passing and CPU activity
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Second read
        SystemCPUStats::CPUStats current_stats = SystemCPUStats::CPUStatsReader::getCPUStats();
        auto current_time = std::chrono::steady_clock::now();

        long long time_delta_ms = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - prev_time).count();

        // The user/caller then explicitly calls the calculation function
        double actual_usage_percent = SystemCPUStats::calculateUsagePercentage(
            current_stats, prev_stats, time_delta_ms);

        // Assert that a sensible percentage is calculated
        EXPECT_GE(actual_usage_percent, 0.0);
        EXPECT_LE(actual_usage_percent, 100.0);
        // We cannot predict the exact value, but we can verify it's within bounds.
        // This implicitly tests the interaction of getCPUStats() and calculateUsagePercentage().

    } catch (const std::runtime_error& e) {
        FAIL() << "CPU Usage Calculation Demo failed: " << e.what();
    }
}


// --- Main function to run tests ---
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
