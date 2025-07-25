#include "net_stats.hpp"
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

// Define a platform-specific loopback interface name
#if defined(_WIN32) || defined(__WIN64__)
    const std::string LOOPBACK_INTERFACE_NAME = "Loopback Pseudo-Interface 1"; // Common Windows loopback
#elif defined(__APPLE__)
    const std::string LOOPBACK_INTERFACE_NAME = "lo0"; // Common macOS loopback
#elif defined(__linux__)
    const std::string LOOPBACK_INTERFACE_NAME = "lo"; // Common Linux loopback
#else
    const std::string LOOPBACK_INTERFACE_NAME = "unsupported_loopback"; // Fallback for unsupported platforms
#endif


TEST(NetStatsReaderTest, GetNetStats_Aggregated_NoThrowOnSupportedPlatforms) {
#if defined(_WIN32) || defined(__WIN64__) || defined(__APPLE__) || defined(__linux__)
    // Expect no throw when getting aggregated stats on supported platforms
    EXPECT_NO_THROW({
        SystemNetStats::NetStats stats = SystemNetStats::NetStatsReader::getNetStats();
        // You can add more specific checks here if you want to ensure the data is non-zero etc.
        // For example: EXPECT_GE(stats.bytes_received, 0ULL);
        // This test primarily checks that the function executes without crashing or throwing.
    }) << "Expected not to throw an exception when getting aggregated network stats on a supported platform.";
#else
    // On unsupported platforms, it should throw
    EXPECT_THROW(
        SystemNetStats::NetStatsReader::getNetStats(),
        std::runtime_error
    ) << "Expected std::runtime_error on unsupported platform when getting aggregated network stats.";
#endif
}

TEST(NetStatsReaderTest, GetNetStats_SpecificInterface_ValidAndNotFound) {
    // Test a valid interface (if platform supported)
#if defined(_WIN32) || defined(__WIN64__) || defined(__APPLE__) || defined(__linux__)
    SystemNetStats::NetStats actual_stats;
    EXPECT_NO_THROW({
        actual_stats = SystemNetStats::NetStatsReader::getNetStats(LOOPBACK_INTERFACE_NAME);
    }) << "Expected to find loopback interface '" << LOOPBACK_INTERFACE_NAME << "' but it threw an exception.";

    // Optionally, you can add checks for the retrieved stats if you expect specific values
    EXPECT_EQ(actual_stats.interface_name, LOOPBACK_INTERFACE_NAME);
    // EXPECT_GT(actual_stats.bytes_received, 0); // Loopback usually has some traffic

    // Test a non-existent interface (should always throw std::runtime_error)
    EXPECT_THROW(
        SystemNetStats::NetStatsReader::getNetStats("nonexistent_interface_xyz"),
        std::runtime_error
    ) << "Expected std::runtime_error for a non-existent interface.";

#else
    // On unsupported platforms, both calls should throw
    EXPECT_THROW(
        SystemNetStats::NetStatsReader::getNetStats(LOOPBACK_INTERFACE_NAME),
        std::runtime_error
    ) << "Expected std::runtime_error on unsupported platform for valid interface name.";

    EXPECT_THROW(
        SystemNetStats::NetStatsReader::getNetStats("nonexistent_interface_xyz"),
        std::runtime_error
    ) << "Expected std::runtime_error on unsupported platform for non-existent interface name.";
#endif
}

TEST(NetStatsReaderTest, GetNetStats_SpecificInterface_EmptyName) {
#if defined(_WIN32) || defined(__WIN64__) || defined(__APPLE__) || defined(__linux__)
    // Expect std::invalid_argument when passing an empty interface name
    EXPECT_THROW(
        SystemNetStats::NetStatsReader::getNetStats(""),
        std::invalid_argument // Changed from std::runtime_error to match implementation logic
    ) << "Expected std::invalid_argument when passing an empty interface name.";
#else
    // On unsupported platforms, it should still throw std::runtime_error
    EXPECT_THROW(
        SystemNetStats::NetStatsReader::getNetStats(""),
        std::runtime_error
    ) << "Expected std::runtime_error on unsupported platform for empty interface name.";
#endif
}
