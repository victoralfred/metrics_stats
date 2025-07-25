#include "cpu_stats.hpp" // Include the redesigned header

#include <fstream>     // For std::ifstream
#include <sstream>     // For std::stringstream
#include <limits>      // For std::numeric_limits<double>::epsilon()
#include <chrono>      // For std::chrono::steady_clock

// Platform-specific includes
#ifdef _WIN32
#include <windows.h>
#elif __linux__
#include <unistd.h> // For sysconf(_SC_CLK_TCK), though not used for percentage
#elif __APPLE__
#include <mach/mach.h>
#include <mach/mach_host.h>
#endif

namespace SystemCPUStats {

// --- Implementation of the standalone calculateUsagePercentage function ---
double calculateUsagePercentage(const CPUStats& curr, const CPUStats& prev, long long time_delta_ms) {
    // If time_delta_ms is zero, or if the total CPU time hasn't changed, return 0.0
    // This prevents division by zero and handles cases where no activity change is observed.
    if (time_delta_ms <= 0) {
        return 0.0;
    }

    double prev_total_cpu_time = prev.getTotalTime();
    double current_total_cpu_time = curr.getTotalTime();

    double prev_active_cpu_time = prev.getTotalActiveTime();
    double current_active_cpu_time = curr.getTotalActiveTime();

    double total_cpu_time_delta = current_total_cpu_time - prev_total_cpu_time;
    double active_cpu_time_delta = current_active_cpu_time - prev_active_cpu_time;

    // Avoid division by zero if total_cpu_time_delta is zero (e.g., stats didn't change at all)
    if (total_cpu_time_delta <= std::numeric_limits<double>::epsilon()) {
        return 0.0;
    }

    return (active_cpu_time_delta / total_cpu_time_delta) * 100.0;
}


// --- CPUStatsReader Implementations ---

// Private helper to parse /proc/stat-like input (used by Linux/Docker and getCPUStats(istream))
CPUStats CPUStatsReader::parseProcStatLine(std::istream& input_stream) {
    std::string line;
    if (!std::getline(input_stream, line)) {
        throw std::runtime_error("Failed to read line from CPU stats stream.");
    }

    std::stringstream ss(line);
    std::string cpu_label;
    ss >> cpu_label; // Read "cpu" or "cpu0", "cpu1", etc.

    if (cpu_label.substr(0, 3) != "cpu") {
        throw std::runtime_error("Invalid CPU stats line format: expected 'cpu' label.");
    }

    CPUStats stats;
    // Read the values into their respective fields.
    // Order: user, nice, system, idle, iowait, irq, softirq, steal, guest, guest_nice
    if (!(ss >> stats.user >> stats.nice >> stats.system >> stats.idle >>
            stats.iowait >> stats.irq >> stats.softirq >> stats.steal >>
            stats.guest >> stats.guest_nice)) {
        throw std::runtime_error("Failed to parse CPU time fields from stream.");
    }
    stats.usage_percent = 0.0; // Not calculated by parsing raw data
    return stats;
}


// --- Platform-specific raw CPUStats retrieval ---

#ifdef _WIN32
CPUStats CPUStatsReader::getRawWindowsCpuStats() {
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        throw std::runtime_error("Failed to get system times on Windows.");
    }

    ULARGE_INTEGER uIdle, uKernel, uUser;
    uIdle.LowPart = idleTime.dwLowDateTime; uIdle.HighPart = idleTime.dwHighDateTime;
    uKernel.LowPart = kernelTime.dwLowDateTime; uKernel.HighPart = kernelTime.dwHighDateTime;
    uUser.LowPart = userTime.dwLowDateTime; uUser.HighPart = userTime.dwHighDateTime;

    CPUStats currentStats;
    currentStats.idle = static_cast<double>(uIdle.QuadPart);
    currentStats.user = static_cast<double>(uUser.QuadPart);
    currentStats.system = static_cast<double>(uKernel.QuadPart - uIdle.QuadPart); // System (kernel) time excluding idle

    // Other fields are not directly available or have different meanings in GetSystemTimes
    currentStats.nice = 0.0; currentStats.iowait = 0.0; currentStats.irq = 0.0;
    currentStats.softirq = 0.0; currentStats.steal = 0.0; currentStats.guest = 0.0;
    currentStats.guest_nice = 0.0;
    currentStats.usage_percent = 0.0; // Not calculated by this raw data retrieval
    return currentStats;
}

#elif __linux__
CPUStats CPUStatsReader::getRawLinuxCpuStats() {
    std::ifstream proc_stat("/proc/stat");
    if (!proc_stat.is_open()) {
        throw std::runtime_error("Failed to open /proc/stat.");
    }
    CPUStats stats = parseProcStatLine(proc_stat); // Reuses the parsing logic
    proc_stat.close();
    stats.usage_percent = 0.0; // Not calculated by this raw data retrieval
    return stats;
}

#elif __APPLE__
CPUStats CPUStatsReader::getRawMacCpuStats() {
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    host_cpu_load_info_data_t cpu_info;

    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpu_info, &count) != KERN_SUCCESS) {
        throw std::runtime_error("Failed to get CPU load info on macOS.");
    }

    CPUStats currentStats;
    // Mach provides: user, system, idle, nice
    currentStats.user = static_cast<double>(cpu_info.cpu_ticks[CPU_STATE_USER]);
    currentStats.system = static_cast<double>(cpu_info.cpu_ticks[CPU_STATE_SYSTEM]);
    currentStats.idle = static_cast<double>(cpu_info.cpu_ticks[CPU_STATE_IDLE]);
    currentStats.nice = static_cast<double>(cpu_info.cpu_ticks[CPU_STATE_NICE]);

    currentStats.iowait = 0.0; currentStats.irq = 0.0; currentStats.softirq = 0.0;
    currentStats.steal = 0.0; currentStats.guest = 0.0; currentStats.guest_nice = 0.0;
    currentStats.usage_percent = 0.0; // Not calculated by this raw data retrieval
    return currentStats;
}
#else
// Fallback for unsupported platforms
CPUStats CPUStatsReader::getRawUnsupportedCpuStats() {
    throw std::runtime_error("CPUStatsReader::getCPUStats() is not implemented for this operating system.");
}
#endif

// --- Public API Implementations ---

CPUStats CPUStatsReader::getCPUStats() {
#ifdef _WIN32
    return getRawWindowsCpuStats();
#elif __linux__
    return getRawLinuxCpuStats();
#elif __APPLE__
    return getRawMacCpuStats();
#else
    return getRawUnsupportedCpuStats();
#endif
}

CPUStats CPUStatsReader::getCPUStats(std::istream& input) {
    return parseProcStatLine(input);
}

} // namespace SystemCPUStats