#include "disk_stats.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include <algorithm> // For std::find_if
#include <cctype>    // For std::isdigit

namespace SystemDiskStats {

    // Helper function to convert sectors to bytes (assuming 512 bytes per sector)
    static unsigned long long sectorsToBytes(unsigned long long sectors) {
        return sectors * 512;
    }

    DiskStats DiskStatsReader::parseDiskStatLine(const std::string& line) {
        std::istringstream iss(line);
        DiskStats stats;
        std::string temp_dev_name; // Use a temporary to read device name

        // Fields from /proc/diskstats:
        // 0: major number
        // 1: minor number
        // 2: device name
        // 3: reads completed successfully (rd_ios)
        // 4: reads merged (rd_merges)
        // 5: sectors read (rd_sectors)
        // 6: time spent reading (rd_ticks) (ms)
        // 7: writes completed (wr_ios)
        // 8: writes merged (wr_merges)
        // 9: sectors written (wr_sectors)
        // 10: time spent writing (wr_ticks) (ms)
        // 11: I/Os currently in progress (ios_pgr)
        // 12: time spent doing I/Os (tot_ticks) (ms)
        // 13: weighted time spent doing I/Os (rq_ticks) (ms)

        unsigned long long dummy_ull; // For skipping unused fields
        unsigned long long sectors_read_val, time_read_ms_val;
        unsigned long long sectors_written_val, time_write_ms_val;

        if (!(iss >> dummy_ull // major
                 >> dummy_ull // minor
                 >> temp_dev_name // device name
                 >> dummy_ull // rd_ios
                 >> dummy_ull // rd_merges
                 >> sectors_read_val // rd_sectors
                 >> time_read_ms_val // rd_ticks
                 >> dummy_ull // wr_ios
                 >> dummy_ull // wr_merges
                 >> sectors_written_val // wr_sectors
                 >> time_write_ms_val)) // wr_ticks
        {
            throw std::runtime_error("Failed to parse essential fields from diskstat line: " + line);
        }

        stats.device = temp_dev_name;
        stats.read_bytes = sectorsToBytes(sectors_read_val);
        stats.write_bytes = sectorsToBytes(sectors_written_val);
        stats.read_time_ms = time_read_ms_val;
        stats.write_time_ms = time_write_ms_val;

        return stats;
    }

    DiskStats DiskStatsReader::getDiskStats() {
        std::ifstream file("/proc/diskstats");
        if (!file.is_open()) {
            throw std::runtime_error("Could not open /proc/diskstats for reading disk statistics.");
        }

        DiskStats aggregated_stats; // All members initialized to 0 by default constructor

        std::string line;
        while (std::getline(file, line)) {
            try {
                std::istringstream iss(line);
                unsigned long long major, minor;
                std::string device_name;
                iss >> major >> minor >> device_name;

                // Simple heuristic: Filter out partitions (e.g., sda1, sdb2) and loop devices.
                // A device name ending with a digit is likely a partition.
                if (!device_name.empty() && std::isdigit(device_name.back())) {
                    continue; // Skip partitions
                }
                if (device_name.rfind("loop", 0) == 0 || device_name.rfind("ram", 0) == 0 || device_name.rfind("dm-", 0) == 0) {
                     continue; // Skip loop, ram, and device-mapper devices
                }

                // We need to re-parse the full line to get all stats for the current device
                DiskStats current_device_stats = parseDiskStatLine(line);

                aggregated_stats.read_bytes += current_device_stats.read_bytes;
                aggregated_stats.write_bytes += current_device_stats.write_bytes;
                aggregated_stats.read_time_ms += current_device_stats.read_time_ms;
                aggregated_stats.write_time_ms += current_device_stats.write_time_ms;

            } catch (const std::runtime_error& e) {
                // Log or ignore lines that cannot be parsed fully or don't match expected format
                std::cerr << "Warning: Skipping diskstat line due to parsing error: " << e.what() << std::endl;
            }
        }

        aggregated_stats.device = "aggregated"; // Set a placeholder device name for aggregated stats
        return aggregated_stats;
    }

    DiskStats DiskStatsReader::getDiskStats(const std::string& device_name) {
        std::ifstream file("/proc/diskstats");
        if (!file.is_open()) {
            throw std::runtime_error("Could not open /proc/diskstats for reading disk statistics.");
        }

        std::string line;
        while (std::getline(file, line)) {
            // Check if the line contains the specified device name.
            // Using a space before and after the device_name to prevent partial matches.
            // Note: /proc/diskstats format has variable whitespace, so careful matching is needed.
            // A more robust check would involve parsing the device name field explicitly.
            std::istringstream iss(line);
            unsigned long long major, minor;
            std::string current_dev_name;
            if (iss >> major >> minor >> current_dev_name && current_dev_name == device_name) {
                return parseDiskStatLine(line);
            }
        }
        throw std::runtime_error("Device '" + device_name + "' not found in /proc/diskstats.");
    }

} // namespace SystemDiskStats
