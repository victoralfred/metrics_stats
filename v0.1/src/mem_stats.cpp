#include "mem_stats.hpp"

namespace SystemMemoryStats {

#ifdef __linux__
MemStats MeMStatsReader::getMemStats(std::istream& input) {
    if (!input) {
        throw std::runtime_error("Could not open /proc/meminfo");
    }
    
    std::string line;
    std::printf("Reading memory stats from /proc/meminfo\n");
    
    // Initialize the MemStats structure
    MemStats stats = {0, 0, 0, 0, 0};

    while (std::getline(input, line)) {
        std::istringstream ss(line);
        std::string key;
        unsigned long value;
        std::string unit;

        if (ss >> key >> value >> unit) {
            if (unit == "kB") {
                if (key == "MemTotal:") {
                    stats.total = value;
                } else if (key == "MemFree:") {
                    stats.free = value;
                } else if (key == "MemAvailable:") {
                    stats.available = value;
                } else if (key == "Buffers:") {
                    stats.buffers = value;
                } else if (key == "Cached:") {
                    stats.cached = value;
                }
            }
        }
    }
    if (stats.total == 0) {
        throw std::runtime_error("Failed to read memory stats from /proc/meminfo");
        return stats; // Return empty stats if no data was read
    }
    return stats; // Return the populated stats
}

MemStats MeMStatsReader::getMemStats() {
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) {
        throw std::runtime_error("Could not open /proc/meminfo");
    }
    return MeMStatsReader::getMemStats(file);
}

#elif _WIN64
#include <windows.h>
MemStats MeMStatsReader::getMemStats() {
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    if (!GlobalMemoryStatusEx(&memStatus)) {
        throw std::runtime_error("GlobalMemoryStatusEx failed");
    }
    MemStats mem{};
    mem.total = memStatus.ullTotalPhys / 1024;
    mem.free = memStatus.ullAvailPhys / 1024;
    mem.available = mem.free;
    mem.buffers = 0;
    mem.cached = 0;
    return mem; 
}
#elif _WIN32
#include <windows.h>
MemStats MeMStatsReader::getMemStats() {
    throw std::runtime_error("Memory stats not supported on _WIN32");
}
#else
MemStats MeMStatsReader::getMemStats() {
    throw std::runtime_error("Memory stats not supported on this platform");
}
#endif // __linux__
} // namespace SystemMemoryStats
