#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include "disk_stats.hpp"  // Assuming this is where getDiskStats is declared

class DiskStatsReaderTest : public ::testing::Test {
protected:
    std::string tempFilePath;

    void SetUp() override {
        tempFilePath = "test_diskstats.txt";
    }

    void TearDown() override {
        std::remove(tempFilePath.c_str());
    }

    void writeMockDiskStats(const std::string& content) {
        std::ofstream out(tempFilePath);
        out << content;
        out.close();
    }
    void writeMockDiskStats(const std::string& filePath, const std::string& content) {
    std::ofstream out(filePath);
    if (!out.is_open()) throw std::runtime_error("Failed to open file for writing");
    out << content;
}
};

TEST_F(DiskStatsReaderTest, ParsesCorrectlyFromMockData) {
    std::string mockData =
        "   8       0 sda 157698 5002 7654321 3020 198123 6543 5432109 4980 0 0 0 0\n";
    
    writeMockDiskStats(mockData);
    std::ifstream file(tempFilePath);
    if (!file.is_open()) {
    // handle error
        throw std::runtime_error("Could not open mock disk stats file");
    }
    auto stats = SystemDiskStats::DiskStatsReader::getDiskStats(file);  // Pass file path instead of "sda"

    // 7654321 and 5432109 sectors * 512 bytes
    EXPECT_EQ(stats.read_bytes, 7654321ULL * 512);
    EXPECT_EQ(stats.write_bytes, 5432109ULL * 512);
}

TEST_F(DiskStatsReaderTest, ThrowsOnEmptyDevicePath) {
    std::string empty = "";
    std::ifstream file(empty);
    if (!file.is_open()) {
    // handle error
        throw std::runtime_error("Could not open mock disk stats file");
    }
    EXPECT_THROW({
        SystemDiskStats::DiskStatsReader::getDiskStats(file);
    }, std::runtime_error);
}

TEST_F(DiskStatsReaderTest, ThrowsOnMissingFile) {
    std::string missingFile = "nonexistent_file.txt";
    std::ifstream file(missingFile);
    if (!file.is_open()) {
    // handle error
        throw std::runtime_error("Could not open mock disk stats file");
    }
    EXPECT_THROW({
        SystemDiskStats::DiskStatsReader::getDiskStats(file);
    }, std::runtime_error);
}
TEST_F(DiskStatsReaderTest, ThrowsOnInvalidFormat) {
    // Write invalid content to your mock disk stats file
    writeMockDiskStats(tempFilePath, "Invalid format line");

    std::ifstream file(tempFilePath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open mock disk stats file");
    }

    EXPECT_THROW({
        SystemDiskStats::DiskStatsReader::getDiskStats(file);
    }, std::runtime_error);
}