#include "disk_stats.hpp"

namespace SystemDiskStats {
#ifdef __linux__
DiskStats DiskStatsReader::getDiskStats(std::ifstream& file) {
    std::string device = "sda"; // Default device, can be parameterized
    // This function reads disk stats from /proc/diskstats for a specific device
    if (file.fail() || !file.is_open()) {
        throw std::runtime_error("File name cannot be empty");
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(device) != std::string::npos) {
            std::istringstream iss(line);
            int major, minor;
            std::string dev;
            unsigned long rd_ios, rd_merges, rd_sectors, rd_ticks;
            unsigned long wr_ios, wr_merges, wr_sectors, wr_ticks;
            iss >> major >> minor >> dev >> rd_ios >> rd_merges >> rd_sectors >> rd_ticks
                >> wr_ios >> wr_merges >> wr_sectors >> wr_ticks;
            stats.read_bytes = rd_sectors * 512ULL;
            stats.write_bytes = wr_sectors * 512ULL;
            break;
        }
    }
    return stats;
}
DiskStats DiskStatsReader::getDiskStats() {
    std::ifstream file("/proc/diskstats");
    if (!file.is_open()) {
        throw std::runtime_error("Could not open /proc/diskstats");
    }
    return DiskStatsReader::getDiskStats(file);
}
#elif _WIN64
DiskStats DiskStatsReader::getDiskStats(std::ifstream& device) {
    throw std::runtime_error("Disk stats not supported on WIN64 platform");
}
DiskStats DiskStatsReader::getDiskStats() {
    // Windows implementation would go here, but is not provided in this example
    throw std::runtime_error("Disk stats not supported on WIN64 platform");
}
#else
DiskStats DiskStatsReader::getDiskStats(std::ifstream& device) {
    throw std::runtime_error("Disk stats not supported on this platform");
}
DiskStats DiskStatsReader::getDiskStats() {
    throw std::runtime_error("Disk stats not supported on this platform");
}
#endif // __linux__
}