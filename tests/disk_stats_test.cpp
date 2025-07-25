#include <gtest/gtest.h>
#include "disk_stats.hpp"
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream> // For std::cerr

// Use the namespace for convenience
using namespace SystemDiskStats;

// Test fixture for DiskStatsReader. This allows setting up common resources
// before each test and cleaning up after.
class DiskStatsReaderTest : public ::testing::Test {
protected:
    // No specific setup or teardown needed for these tests, but a fixture
    // is good practice with GTest.
};

// --- Tests for parseDiskStatLine (Linux-specific behavior) ---
#if defined(__linux__)
// Test case for valid /proc/diskstats line on Linux
TEST_F(DiskStatsReaderTest, ParseDiskStatLine_ValidInput) {
    // Example line from /proc/diskstats (simplified for key fields)
    // Format: major minor device_name reads_comp reads_merg sectors_read time_read writes_comp writes_merg sectors_write time_write io_in_progress time_io weighted_time_io
    std::string line = "   8       0 sda 100 0 1000 500 200 0 2000 1000 0 0 0 0";
    DiskStats stats = DiskStatsReader::parseDiskStatLine(line);

    EXPECT_EQ(stats.device, "sda");
    // Sectors are typically 512 bytes for /proc/diskstats, so sectors_read * 512
    EXPECT_EQ(stats.read_bytes, 1000ULL * 512ULL); // 512000
    EXPECT_EQ(stats.write_bytes, 2000ULL * 512ULL); // 1024000
    EXPECT_EQ(stats.read_time_ms, 500ULL);
    EXPECT_EQ(stats.write_time_ms, 1000ULL);

    // Another example with different values and a different device
    // Corrected line2: Values 89012 and 34567 are now correctly placed for sectors_written and ms_written
    std::string line2 = "   8    16 sdb 12345 67 890123 4567 0 0 89012 34567 0 0 0";
    DiskStats stats2 = DiskStatsReader::parseDiskStatLine(line2);
    EXPECT_EQ(stats2.device, "sdb");
    EXPECT_EQ(stats2.read_bytes, 890123ULL * 512ULL);
    EXPECT_EQ(stats2.write_bytes, 89012ULL * 512ULL); // This should now pass
    EXPECT_EQ(stats2.read_time_ms, 4567ULL);
    EXPECT_EQ(stats2.write_time_ms, 34567ULL); // This should now pass
}

// Test case for malformed /proc/diskstats line on Linux
TEST_F(DiskStatsReaderTest, ParseDiskStatLine_MalformedInput) {
    std::string line = "invalid line format"; // Not enough numeric fields
    EXPECT_THROW(DiskStatsReader::parseDiskStatLine(line), std::runtime_error);

    std::string line2 = "8 0 sda 100 0 1000 500 missing_fields"; // Missing values
    EXPECT_THROW(DiskStatsReader::parseDiskStatLine(line2), std::runtime_error);

    std::string line3 = ""; // Empty line
    EXPECT_THROW(DiskStatsReader::parseDiskStatLine(line3), std::runtime_error);
    
    std::string line4 = "8 0 sda 1 2 3 4 5 6 7 8 9 10"; // Too few values (missing last 3)
    EXPECT_THROW(DiskStatsReader::parseDiskStatLine(line4), std::runtime_error);
}

#else // Not Linux: parseDiskStatLine should throw std::logic_error
// Test case to ensure parseDiskStatLine throws logic_error on non-Linux platforms
TEST_F(DiskStatsReaderTest, ParseDiskStatLine_ThrowsLogicErrorOnNonLinux) {
    std::string line = "any line content"; // The content doesn't matter on non-Linux
    EXPECT_THROW(DiskStatsReader::parseDiskStatLine(line), std::logic_error);
}
#endif

// --- Tests for getDiskStats() (platform-dependent behavior) ---

// Test case for getDiskStats() (aggregated)
TEST_F(DiskStatsReaderTest, GetDiskStats_Aggregated_ReturnsCorrectlyOrThrowsUnsupported) {
    try {
        DiskStats stats = DiskStatsReader::getDiskStats();
        // If it reaches here, it means it's a supported platform and returned stats.
        // We can at least check if the device name is "aggregated".
        EXPECT_EQ(stats.device, "aggregated");
        // We cannot assert specific numeric values as they depend on the system state.
        // You might add EXPECT_GE(stats.read_bytes, 0) etc., but actual values are dynamic.
    } catch (const std::runtime_error& e) {
        // This is expected on unsupported platforms or if the underlying system call fails.
        // We check for the specific "unsupported" message to pass the test gracefully
        // in environments where disk stats are not available.
        std::string errorMessage = e.what();
        if (errorMessage == "Disk statistics are not supported on this platform.") {
            SUCCEED() << "Caught expected runtime error: " << errorMessage;
        } else {
            // If it throws for another reason (e.g., permission denied on Linux),
            // it indicates a potential issue or setup problem.
            FAIL() << "Caught unexpected runtime error: " << errorMessage;
        }
    }
}

// Test case for getDiskStats(device_name)
TEST_F(DiskStatsReaderTest, GetDiskStats_SpecificDevice_ThrowsNotFoundOrUnsupported) {
    std::string nonExistentDevice = "NonExistentDevice12345";
    try {
        // This call is expected to throw either because the device is not found
        // or because the platform is unsupported.
        EXPECT_THROW(DiskStatsReader::getDiskStats(nonExistentDevice), std::runtime_error);

        // Capture the exception to check its message if needed
        try {
            DiskStatsReader::getDiskStats(nonExistentDevice);
        } catch (const std::runtime_error& e) {
            std::string errorMessage = e.what();
            if (errorMessage.find("not found") != std::string::npos || 
                errorMessage == "Disk statistics are not supported on this platform.") {
                SUCCEED() << "Caught expected runtime error: " << errorMessage;
            } else {
                FAIL() << "Caught unexpected runtime error for specific device: " << errorMessage;
            }
        }
    } catch (const ::testing::AssertionException& e) {
        // Re-throw GTest assertions
        throw e;
    } catch (const std::exception& e) {
        // Catch any other unexpected exceptions that escape EXPECT_THROW
        FAIL() << "An unexpected exception occurred: " << e.what();
    }
}