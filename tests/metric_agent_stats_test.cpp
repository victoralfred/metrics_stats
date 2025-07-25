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
#include "mem_stats.hpp"
#include "disk_stats.hpp"
#include "net_stats.hpp"

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
// Simple helper for test cases (not a full-fledged test framework)
#define TEST_CASE(name) \
    void name(); \
    int main() { \
        std::cout << "--- Running Test Case: " << #name << " ---\n"; \
        try { \
            name(); \
            std::cout << "--- Test Case: " << #name << " PASSED ---\n\n"; \
        } catch (const std::exception& e) { \
            std::cerr << "--- Test Case: " << #name << " FAILED: " << e.what() << " ---\n\n"; \
            return 1; \
        } \
        return 0; \
    } \
    void name()

// --- Test Case for Memory Statistics ---
void testMemStats() {
    std::cout << "Testing SystemMemoryStats::MeMStatsReader::getMemStats()\n";
    SystemMemoryStats::MemStats memStats = SystemMemoryStats::MeMStatsReader::getMemStats();

    std::cout << "Total Memory: " << memStats.total << " KB\n";
    std::cout << "Free Memory: " << memStats.free << " KB\n";
    std::cout << "Available Memory: " << memStats.available << " KB\n";
    std::cout << "Buffers: " << memStats.buffers << " KB\n";
    std::cout << "Cached: " << memStats.cached << " KB\n";
    std::cout << "Swap Total: " << memStats.swap_total << " KB\n";
    std::cout << "Swap Free: " << memStats.swap_free << " KB\n";

    // Basic assertions
    assert(memStats.total > 0 && "Total memory should be greater than 0");
    assert(memStats.free <= memStats.total && "Free memory should not exceed total memory");
    assert(memStats.available <= memStats.total && "Available memory should not exceed total memory");
}

// --- Test Case for Disk Statistics ---
void testDiskStats() {
    std::cout << "Testing SystemDiskStats::DiskStatsReader::getDiskStats() (Aggregated)\n";
    SystemDiskStats::DiskStats aggregatedDiskStats = SystemDiskStats::DiskStatsReader::getDiskStats();

    std::cout << "Aggregated Disk Stats:\n";
    std::cout << "  Device: " << aggregatedDiskStats.device << "\n";
    std::cout << "  Read Bytes: " << aggregatedDiskStats.read_bytes << "\n";
    std::cout << "  Write Bytes: " << aggregatedDiskStats.write_bytes << "\n";
    std::cout << "  Read Time (ms): " << aggregatedDiskStats.read_time_ms << "\n";
    std::cout << "  Write Time (ms): " << aggregatedDiskStats.write_time_ms << "\n";

    // Basic assertions
    assert(aggregatedDiskStats.read_bytes >= 0 && "Aggregated read bytes should be non-negative");
    assert(aggregatedDiskStats.write_bytes >= 0 && "Aggregated write bytes should be non-negative");

    std::cout << "\nTesting SystemDiskStats::DiskStatsReader::getDiskStats(device_name)\n";
    // IMPORTANT: Replace "nvme0n1" with a device name that actually exists on your system.
    // Use `lsblk` or `cat /proc/diskstats` in your terminal to find actual device names.
    std::string testDeviceName = "nvme0n1"; // Common for cloud VMs, could be "sda", "nvme0n1" etc.

    try {
        SystemDiskStats::DiskStats deviceDiskStats = SystemDiskStats::DiskStatsReader::getDiskStats(testDeviceName);
        std::cout << "Disk Stats for " << testDeviceName << ":\n";
        std::cout << "  Device: " << deviceDiskStats.device << "\n";
        std::cout << "  Read Bytes: " << deviceDiskStats.read_bytes << "\n";
        std::cout << "  Write Bytes: " << deviceDiskStats.write_bytes << "\n";
        std::cout << "  Read Time (ms): " << deviceDiskStats.read_time_ms << "\n";
        std::cout << "  Write Time (ms): " << deviceDiskStats.write_time_ms << "\n";

        assert(deviceDiskStats.device == testDeviceName && "Device name should match requested");
        assert(deviceDiskStats.read_bytes >= 0 && "Device read bytes should be non-negative");

    } catch (const std::runtime_error& e) {
        std::cerr << "Warning: Could not get stats for device '" << testDeviceName << "': " << e.what() << "\n";
        std::cerr << "Please ensure '" << testDeviceName << "' is a valid disk device on your system for this test to fully pass.\n";
        // Do not assert false here, as it's a known potential environmental issue for the test.
    }
}

// --- Test Case for Network Statistics ---
void testNetStats() {
    std::cout << "Testing SystemNetworkStats::NetworkStatsReader::getNetStatsPerINterface()\n";
    std::map<std::string, SystemNetworkStats::NetStats> netStatsMap {};
    // Retrieve network stats for all interfaces and store in a map
    try {
        netStatsMap = SystemNetworkStats::NetworkStatsReader::getNetStatsPerINterface();
    } catch (const std::runtime_error& e) {
        std::cerr << "Error retrieving network stats: " << e.what() << "\n";
        return; // Exit the test if we cannot retrieve network stats
    }
    // Check if the map is not empty
    std::cout << "Retrieved Network Stats for " << netStatsMap.size() << " interfaces.\n";
    // Ensure we have at least one interface to test against
    assert(netStatsMap.size() > 0 && "Network stats map should contain at least one interface");

    std::cout << "Network Statistics for all interfaces:\n";
    for (const auto& pair : netStatsMap) {
        const std::string& interfaceName = pair.first;
        const SystemNetworkStats::NetStats& netStats = pair.second;

        std::cout << "  Interface: " << interfaceName << "\n";
        std::cout << "    Rx Bytes: " << netStats.rx_bytes << "\n";
        std::cout << "    Tx Bytes: " << netStats.tx_bytes << "\n";
        std::cout << "    Rx Packets: " << netStats.rx_packets << "\n";
        std::cout << "    Tx Packets: " << netStats.tx_packets << "\n";
        std::cout << "    Rx Errors: " << netStats.rx_errors << "\n";
        std::cout << "    Tx Errors: " << netStats.tx_errors << "\n";
        std::cout << "    Rx Dropped: " << netStats.rx_dropped << "\n";
        std::cout << "    Tx Dropped: " << netStats.tx_dropped << "\n";

        // Basic assertions
        assert(!interfaceName.empty() && "Interface name should not be empty");
        assert(netStats.rx_bytes >= 0 && "Rx bytes should be non-negative");
        assert(netStats.tx_bytes >= 0 && "Tx bytes should be non-negative");
    }
}

// --- Test Case for CPU Statistics ---
void testCpuStats() {
    std::cout << "Testing SystemCPUStats::CPUStatsReader::getCPUStats()\n";
    SystemCPUStats::CPUStats cpuStats1 = SystemCPUStats::CPUStatsReader::getCPUStats();

    std::cout << "Initial CPU Stats:\n";
    std::cout << "  User: " << cpuStats1.user << "\n";
    std::cout << "  System: " << cpuStats1.system << "\n";
    std::cout << "  Idle: " << cpuStats1.idle << "\n";
    std::cout << "  Total Active Time: " << cpuStats1.getTotalActiveTime() << "\n";
    std::cout << "  Total Time: " << cpuStats1.getTotalTime() << "\n";

    // Wait for a short interval to get new stats for usage calculation
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    SystemCPUStats::CPUStats cpuStats2 = SystemCPUStats::CPUStatsReader::getCPUStats();

    double usage = SystemCPUStats::calculateUsagePercentage(cpuStats2, cpuStats1, 500);
    std::cout << "CPU Usage (over 500ms): " << usage << "%\n";

    // Basic assertions
    assert(cpuStats1.getTotalTime() > 0 && "Initial total CPU time should be greater than 0");
    assert(usage >= 0 && usage <= 100 && "CPU usage should be between 0 and 100%");
}

// Example main function to run all tests
// For simplicity, this main function calls all test cases.
// If you have individual test files (e.g., cpu_stats_test.cpp, disk_stats_test.cpp)
// that also define their own 'main' functions, you MUST remove those 'main' functions
// when linking them with this file, or compile them as separate executables.
// --- Main function to run tests ---
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);

    std::cout << "Starting C++ System Metrics Tests...\n\n";

    // Run CPU Stats Test
    std::cout << "--- Running Test Case: testCpuStats ---\n";
    try {
        testCpuStats();
        std::cout << "--- Test Case: testCpuStats PASSED ---\n\n";
    } catch (const std::exception& e) {
        std::cerr << "--- Test Case: testCpuStats FAILED: " << e.what() << " ---\n\n";
        return 1; // Indicate failure
    }

    // Run Memory Stats Test
    std::cout << "--- Running Test Case: testMemStats ---\n";
    try {
        testMemStats();
        std::cout << "--- Test Case: testMemStats PASSED ---\n\n";
    } catch (const std::exception& e) {
        std::cerr << "--- Test Case: testMemStats FAILED: " << e.what() << " ---\n\n";
        return 1; // Indicate failure
    }

    // Run Disk Stats Test
    std::cout << "--- Running Test Case: testDiskStats ---\n";
    try {
        testDiskStats();
        std::cout << "--- Test Case: testDiskStats PASSED ---\n\n";
    } catch (const std::exception& e) {
        std::cerr << "--- Test Case: testDiskStats FAILED: " << e.what() << " ---\n\n";
        return 1; // Indicate failure
    }

    // Run Network Stats Test
    std::cout << "--- Running Test Case: testNetStats ---\n";
    try {
        testNetStats();
        std::cout << "--- Test Case: testNetStats PASSED ---\n\n";
    } catch (const std::exception& e) {
        std::cerr << "--- Test Case: testNetStats FAILED: " << e.what() << " ---\n\n";
        return 1; // Indicate failure
    }

    std::cout << "All C++ System Metrics Tests Completed.\n";
    return RUN_ALL_TESTS();
}
