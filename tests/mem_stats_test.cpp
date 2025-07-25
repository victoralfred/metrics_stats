#include <gtest/gtest.h>
#include "mem_stats.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream> // For std::cerr (used in some test scenarios for debug/info)

// Use the namespace for convenience
using namespace SystemMemoryStats;

// Test fixture for MeMStatsReader. This allows setting up common resources
// before each test and cleaning up after.
class MemStatsReaderTest : public ::testing::Test {
protected:
    // No specific setup or teardown needed for these tests, but a fixture
    // is good practice with GTest.
};

// --- Tests for getMemStats() (Platform-dependent behavior) ---

TEST_F(MemStatsReaderTest, GetMemStats_ReturnsValidDataOrThrowsUnsupported) {
    MeMStatsReader reader;
    try {
        MemStats stats = reader.getMemStats();
        
        // On a supported platform, we expect total memory to be greater than 0.
        // Other specific values are hard to assert as they are dynamic and system-dependent.
        EXPECT_GT(stats.total, 0ULL) << "Total memory should be greater than 0 on a supported platform.";
        
        // Basic sanity checks: free and available should not exceed total.
        EXPECT_LE(stats.free, stats.total);
        EXPECT_LE(stats.available, stats.total);
        
        // Swap stats might be 0 if no swap is configured, so we don't strictly check GT 0.
        EXPECT_LE(stats.swap_free, stats.swap_total);

        std::cout << "Successfully retrieved memory stats:"
                  << " Total: " << stats.total << " KB"
                  << ", Free: " << stats.free << " KB"
                  << ", Available: " << stats.available << " KB"
                  << ", Swap Total: " << stats.swap_total << " KB"
                  << ", Swap Free: " << stats.swap_free << " KB" << std::endl;

    } catch (const std::runtime_error& e) {
        // This is expected on unsupported platforms, or if the underlying system call fails.
        // We check for the specific "unsupported" message to pass the test gracefully
        // in environments where disk stats are not available.
        std::string errorMessage = e.what();
        if (errorMessage == "Memory statistics are not supported on this platform.") {
            SUCCEED() << "Caught expected runtime error (platform unsupported): " << errorMessage;
        } else {
            // If it throws for another reason (e.g., permission denied on Linux),
            // it indicates a potential issue or setup problem.
            FAIL() << "Caught unexpected runtime error: " << errorMessage;
        }
    }
}

// --- Tests for parseMeminfoLine (Linux-specific behavior) ---
#if defined(__linux__)
TEST_F(MemStatsReaderTest, ParseMeminfoLine_ValidInput) {
    // Test a standard line
    EXPECT_EQ(MeMStatsReader::parseMeminfoLine("MemTotal:        8000000 kB", "MemTotal"), 8000000ULL);
    EXPECT_EQ(MeMStatsReader::parseMeminfoLine("MemFree:          123456 kB", "MemFree"), 123456ULL);
    EXPECT_EQ(MeMStatsReader::parseMeminfoLine("Buffers:           7890 kB", "Buffers"), 7890ULL);
    EXPECT_EQ(MeMStatsReader::parseMeminfoLine("Cached:         9876543 kB", "Cached"), 9876543ULL);
    EXPECT_EQ(MeMStatsReader::parseMeminfoLine("SwapTotal:       1048576 kB", "SwapTotal"), 1048576ULL);
    EXPECT_EQ(MeMStatsReader::parseMeminfoLine("SwapFree:         524288 kB", "SwapFree"), 524288ULL);
    EXPECT_EQ(MeMStatsReader::parseMeminfoLine("MemAvailable:    7890123 kB", "MemAvailable"), 7890123ULL);

    // Test with different spacing
    EXPECT_EQ(MeMStatsReader::parseMeminfoLine("MemTotal:8000000 kB", "MemTotal"), 8000000ULL);
    EXPECT_EQ(MeMStatsReader::parseMeminfoLine("MemFree:   123 kB", "MemFree"), 123ULL);

    // Test with keys that contain spaces (though /proc/meminfo keys typically don't)
    // This highlights the importance of the initial `iss >> key_str` getting the whole string up to ':'
    // and then removing the ':'.
    // Note: If a key *actually* contained spaces before the colon (e.g. "Some Key:"),
    // `iss >> key_str` would only read "Some". The current implementation expects keys without spaces
    // before the colon. This is consistent with /proc/meminfo format.
}

TEST_F(MemStatsReaderTest, ParseMeminfoLine_MalformedInput) {
    // Malformed line - not enough fields
    EXPECT_THROW(MeMStatsReader::parseMeminfoLine("MemTotal:", "MemTotal"), std::runtime_error);
    EXPECT_THROW(MeMStatsReader::parseMeminfoLine("MemTotal: 8000000", "MemTotal"), std::runtime_error); // Missing unit
    EXPECT_THROW(MeMStatsReader::parseMeminfoLine("Just a string", "MemTotal"), std::runtime_error);

    // Incorrect key
    EXPECT_THROW(MeMStatsReader::parseMeminfoLine("MemTotal:        8000000 kB", "WrongKey"), std::runtime_error);

    // Empty line
    EXPECT_THROW(MeMStatsReader::parseMeminfoLine("", "MemTotal"), std::runtime_error);

    // Invalid value (non-numeric)
    EXPECT_THROW(MeMStatsReader::parseMeminfoLine("MemTotal:        ABCDEF kB", "MemTotal"), std::runtime_error);
}

#else // Not Linux: parseMeminfoLine should throw std::logic_error
TEST_F(MemStatsReaderTest, ParseMeminfoLine_ThrowsLogicErrorOnNonLinux) {
    std::string line = "any line content"; // The content doesn't matter on non-Linux
    EXPECT_THROW(MeMStatsReader::parseMeminfoLine(line, "MemTotal"), std::logic_error);
}
#endif