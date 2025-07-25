#ifndef DISK_STATS_HPP
#define DISK_STATS_HPP

#include <string>
#include <map>
#include <vector>
#include <utility> // For std::move

namespace SystemDiskStats {
    // Structure to hold disk I/O statistics for a single device.
    struct DiskStats {
        std::string device;           // Device name (e.g., "sda", "nvme0n1")
        unsigned long long read_bytes;    // Total bytes read
        unsigned long long write_bytes;   // Total bytes written
        unsigned long long read_time_ms;  // Milliseconds spent reading (from field 6)
        unsigned long long write_time_ms; // Milliseconds spent writing (from field 10)

        // Constructor matching the members expected by pybind11 and Python test.
        DiskStats(std::string dev = "", unsigned long long rb = 0, unsigned long long wb = 0,
                  unsigned long long rt = 0, unsigned long long wt = 0)
            : device(std::move(dev)), read_bytes(rb), write_bytes(wb), read_time_ms(rt), write_time_ms(wt) {}
    };
    // Class to read and provide disk statistics from the system.
    class DiskStatsReader {
    public:
        /// @brief Retrieves aggregated disk statistics for all physical devices.
        /// @return Returns a DiskStats object representing the sum of all relevant disk activities.
        static DiskStats getDiskStats();
        /// @brief Retrieves disk statistics for a specific device.
        /// @param device_name The name of the device (e.g., "sda", "nvme0n1").
        /// @return A DiskStats object for the specified device.
        /// @throws std::runtime_error if the device is not found or data cannot be read.
        static DiskStats getDiskStats(const std::string& device_name);
        /// @brief Helper to parse a line from /proc/diskstats
        /// @returns  the relevant fields to populate a DiskStats object.
        static DiskStats parseDiskStatLine(const std::string& line);
#if defined(__WIN32__) || defined(__WIN64__)
        /// @brief Windows-specific implementation to retrieve disk statistics.
        /// @return A DiskStats object containing the disk statistics for Windows.
        static DiskStats getRawWindowsDiskStats();
#elif defined( __linux__)
        /// @brief Linux-specific implementation to retrieve disk statistics.
        /// @return A DiskStats object containing the disk statistics for Linux.
        static DiskStats getRawLinuxDiskStats();
#elif defined( __APPLE__)
        /// @brief macOS-specific implementation to retrieve disk statistics.
        /// @return A DiskStats object containing the disk statistics for macOS.
        static DiskStats getRawMacDiskStats();
#else
        /// @brief Fallback for unsupported platforms.
        /// @return A DiskStats object with default values for unsupported platforms.
        static DiskStats getRawUnsupportedDiskStats();
#endif
        // Prevent instantiation of this utility class.
        DiskStatsReader() = delete;
        ~DiskStatsReader() = delete;
        // Prevent copying and assignment of this utility class.
        DiskStatsReader(const DiskStatsReader&) = delete;
        DiskStatsReader& operator=(const DiskStatsReader&) = delete;
    };

} // namespace SystemDiskStats

#endif // DISK_STATS_HPP