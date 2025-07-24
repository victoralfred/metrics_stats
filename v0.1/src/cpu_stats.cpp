#include "cpu_stats.hpp"

namespace SystemCPUStats {
#ifdef __linux__
CPUStats CPUStatsReader::getCPUStats(std::istream& input) {
    // This function reads CPU stats from an input stream, typically from /proc/stat
    if (!input) {
        throw std::runtime_error("Input stream is not valid");
    }
    std::printf("Opening input stream to read CPU stats\n");
    // Initialize the CPUStats structure
    CPUStats stats = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    std::string line;
    std::printf("Reading CPU stats from input stream\n");

    if (std::getline(input, line)) {
        std::istringstream ss(line);
        std::string cpu;
        ss >> cpu;
        std::vector<double> times;
        double time;
        while (ss >> time) {
            times.push_back(time);
        }
        if (times.size() < 10) {
            throw std::runtime_error("Not enough CPU time fields in input");
        }
        stats.user = times[0];
        stats.nice = times[1];
        stats.system = times[2];
        stats.idle = times[3];
        stats.iowait = times[4];
        stats.irq = times[5];
        stats.softirq = times[6];
        stats.steal = times[7];
        stats.guest = times[8];
        stats.guest_nice = times[9];
        return stats;
    } else {
        throw std::runtime_error("Failed to read from input stream");
    }
}

CPUStats CPUStatsReader ::getCPUStats() {
    std::ifstream file("/proc/stat");
    if (!file.is_open()) {
        throw std::runtime_error("Could not open /proc/stat");
    }
    return CPUStatsReader ::getCPUStats(file);
}
#elif _WIN64
#include <windows.h>
CPUStats CPUStatsReader::getCPUStats(std::istream& input) {
    throw std::runtime_error("CPU stats not supported on WIN64 platform");
}
CPUStats CPUStatsReader::getCPUStats() {
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        throw std::runtime_error("GetSystemTimes failed");
    }
    ULARGE_INTEGER idle, kernel, user;
    idle.LowPart = idleTime.dwLowDateTime; idle.HighPart = idleTime.dwHighDateTime;
    kernel.LowPart = kernelTime.dwLowDateTime; kernel.HighPart = kernelTime.dwHighDateTime;
    user.LowPart = userTime.dwLowDateTime; user.HighPart = userTime.dwHighDateTime;

    CPUStats stats{};
    stats.user = user.QuadPart / 10000;
    stats.system = kernel.QuadPart / 10000;
    stats.idle = idle.QuadPart / 10000;
    stats.nice = stats.iowait = stats.irq = stats.softirq = stats.steal = 0;
    return stats;
}
#else
CPUStats CPUStatsReader::getCPUStats() {
    throw std::runtime_error("CPU stats not supported on this platform");
}
#endif // __linux__
}
