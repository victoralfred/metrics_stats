#ifndef DISK_STATS_HPP
#define DISK_STATS_HPP

#include <string>
#include <map>
#include <vector>
#include <utility> // For std::move

namespace SystemDiskStats {

    // Structure to hold disk I/O statistics for a single device.
    // The members correspond to fields in /proc/diskstats
    // (relevant fields: major, minor, device name, reads_completed, reads_merged, sectors_read, time_in_queue_read_ms,
    // writes_completed, writes_merged, sectors_written, time_in_queue_write_ms,
    // io_in_progress, time_in_queue_io_ms, weighted_time_in_queue_io_ms)
    // For simplicity, we'll focus on bytes read/written and time spent.
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
        // Retrieves aggregated disk statistics for all physical devices.
        // Returns a DiskStats object representing the sum of all relevant disk activities.
        static DiskStats getDiskStats();

        // Retrieves disk statistics for a specific device.
        // @param device_name The name of the device (e.g., "sda", "nvme0n1").
        // @return A DiskStats object for the specified device.
        // @throws std::runtime_error if the device is not found or data cannot be read.
        static DiskStats getDiskStats(const std::string& device_name);

    private:
        // Helper to parse a line from /proc/diskstats
        // This function will extract the relevant fields to populate a DiskStats object.
        static DiskStats parseDiskStatLine(const std::string& line);
    };

} // namespace SystemDiskStats

#endif // DISK_STATS_HPP