#define _WIN32_WINNT 0x0501 // Required for MEMORYSTATUSEX and GlobalMemoryStatusEx for Windows

#include "mem_stats.hpp" // Ensure this is the correct, latest header
#include <stdexcept>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <cctype> // For std::isalnum

// Platform-specific headers
#if defined(_WIN32) || defined(__WIN64__)
    #include <windows.h>
#elif defined(__APPLE__)
    #include <sys/sysctl.h>
    #include <mach/mach_host.h>
    #include <mach/vm_statistics.h>
#elif defined(__linux__)
    // Headers already included by <fstream>, <string>, <sstream> etc.
#endif

namespace SystemMemoryStats {

// Anonymous namespace for Linux-only internal helpers
#if defined(__linux__)
namespace {
// Helper to parse a line from /proc/meminfo (e.g., "MemTotal:        8000000 kB")
// Extracts the numerical value for a given key.
unsigned long long parseMeminfoLineInternal(const std::string& line, const std::string& key) {
    if (line.empty()) { // Handle empty line specifically
        throw std::runtime_error("Cannot parse empty line for key '" + key + "'.");
    }

    size_t key_pos = line.find(key);
    // Check if key is found AND it's at the beginning or if not, ensure it's not a partial match (e.g., "Slab" matching "SwapTotal")
    if (key_pos == std::string::npos || (key_pos != 0 && std::isalnum(line[key_pos - 1]))) {
        throw std::runtime_error("Key '" + key + "' not found at the beginning of the line or is a partial match: " + line);
    }

    size_t colon_pos = line.find(':', key_pos);
    // Missing colon after finding the key
    if (colon_pos == std::string::npos) {
        throw std::runtime_error("Malformed meminfo line (missing colon after key '" + key + "'): " + line);
    }

    // Extract the part after the colon
    std::string value_and_unit_str = line.substr(colon_pos + 1);
    std::stringstream ss(value_and_unit_str);
    unsigned long long value;
    std::string unit;

    // Try to read the number
    ss >> value;

    // If reading the number failed, it's malformed
    if (ss.fail()) {
        throw std::runtime_error("Failed to parse numerical value from meminfo line for key '" + key + "': " + line);
    }

    // Try to read the unit (should be "kB")
    ss >> unit;

    // If unit is not "kB" OR there's more data after "kB" (not just whitespace till EOF)
    if (unit != "kB" || !ss.eof()) {
        throw std::runtime_error("Malformed meminfo line (invalid unit or extra data after value for key '" + key + "'): " + line);
    }

    return value;
}
} // anonymous namespace
#endif // __linux__

// --- Private Platform-Specific Helper Definitions (static member functions) ---
// These functions are *defined* for all platforms, but their *implementation logic*
// is conditionally compiled based on the platform.

MemStats MeMStatsReader::getRawWindowsMemStats() {
#if defined(_WIN32) || defined(__WIN64__)
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (!GlobalMemoryStatusEx(&memInfo)) {
        throw std::runtime_error("Failed to get Windows memory status.");
    }

    MemStats stats;
    // Values are in bytes, convert to KB
    stats.total = memInfo.ullTotalPhys / 1024;
    stats.free = memInfo.ullAvailPhys / 1024;
    stats.available = memInfo.ullAvailPhys / 1024; // Approximation for available
    stats.swap_total = memInfo.ullTotalPageFile / 1024;
    stats.swap_free = memInfo.ullAvailPageFile / 1024;
    // Windows API doesn't directly provide equivalents for buffers/cached in a simple way
    stats.buffers = 0;
    stats.cached = 0;

    return stats; // Return a single MemStats object
#else
    throw std::runtime_error("Windows memory statistics not supported on this platform.");
#endif
}

MemStats MeMStatsReader::getRawLinuxMemStats() {
#if defined(__linux__)
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) {
        throw std::runtime_error("Could not open /proc/meminfo.");
    }

    MemStats stats;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        if (line.rfind("MemTotal:", 0) == 0) stats.total = parseMeminfoLineInternal(line, "MemTotal:");
        else if (line.rfind("MemFree:", 0) == 0) stats.free = parseMeminfoLineInternal(line, "MemFree:");
        else if (line.rfind("MemAvailable:", 0) == 0) stats.available = parseMeminfoLineInternal(line, "MemAvailable:");
        else if (line.rfind("Buffers:", 0) == 0) stats.buffers = parseMeminfoLineInternal(line, "Buffers:");
        else if (line.rfind("Cached:", 0) == 0) stats.cached = parseMeminfoLineInternal(line, "Cached:");
        else if (line.rfind("SwapTotal:", 0) == 0) stats.swap_total = parseMeminfoLineInternal(line, "SwapTotal:");
        else if (line.rfind("SwapFree:", 0) == 0) stats.swap_free = parseMeminfoLineInternal(line, "SwapFree:");
    }
    return stats; // Return a single MemStats object
#else
    throw std::runtime_error("Linux memory statistics not supported on this platform.");
#endif
}

MemStats MeMStatsReader::getRawMacMemStats() {
#if defined(__APPLE__)
    MemStats stats;
    int mib[2];
    size_t len;

    // Get total physical memory
    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    unsigned long long total_phys_bytes;
    len = sizeof(total_phys_bytes);
    if (sysctl(mib, 2, &total_phys_bytes, &len, NULL, 0) == -1) {
        throw std::runtime_error("Failed to get total physical memory (sysctl HW_MEMSIZE).");
    }
    stats.total = total_phys_bytes / 1024; // Convert to KB

    // Get VM statistics for free, active, inactive, wired, speculative
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    vm_statistics64_data_t vm_stats;
    if (host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vm_stats, &count) != KERN_SUCCESS) {
        throw std::runtime_error("Failed to get VM statistics (host_statistics64).");
    }

    // Get page size
    vm_size_t page_size;
    if (host_page_size(mach_host_self(), &page_size) != KERN_SUCCESS) {
        throw std::runtime_error("Failed to get page size (host_page_size).");
    }

    // Calculations based on vm_statistics64_data_t
    // free: Pages not in use
    stats.free = (vm_stats.free_count * page_size) / 1024; // Convert to KB

    // available: free + inactive + speculative pages (approx. for applications)
    stats.available = ((vm_stats.free_count + vm_stats.inactive_count + vm_stats.speculative_count) * page_size) / 1024; // Convert to KB

    // cached: For macOS, this is often approximated by inactive pages.
    stats.cached = (vm_stats.inactive_count * page_size) / 1024; // Convert to KB
    stats.buffers = 0; // No direct equivalent on macOS

    // Get swap statistics
    xsw_usage swap_usage;
    len = sizeof(swap_usage);
    if (sysctlbyname("vm.swapusage", &swap_usage, &len, NULL, 0) == -1) {
        // This sysctl can fail if swap isn't active, don't throw, just set to 0
        stats.swap_total = 0;
        stats.swap_free = 0;
    } else {
        stats.swap_total = swap_usage.xsu_total / 1024; // Convert to KB
        stats.swap_free = swap_usage.xsu_avail / 1024; // Convert to KB
    }

    return stats; // Return a single MemStats object
#else
    throw std::runtime_error("macOS memory statistics not supported on this platform.");
#endif
}

MemStats MeMStatsReader::getRawUnsupportedMemStats() {
    throw std::runtime_error("Memory statistics are not supported on this platform.");
}

// Definition for MeMStatsReader's static member function to dispatch to raw platform stats
MemStats MeMStatsReader::getPlatformMemStats() {
#if defined(_WIN32) || defined(__WIN64__)
    return getRawWindowsMemStats();
#elif defined(__linux__)
    return getRawLinuxMemStats();
#elif defined(__APPLE__)
    return getRawMacMemStats();
#else
    return getRawUnsupportedMemStats();
#endif
}

// --- Public API Implementation ---

// Definition of the static member function.
// IMPORTANT: No 'static' keyword here, but the 'MeMStatsReader::' scope ensures it's a member.
// The compiler knows it's static from the mem_stats.hpp declaration.
MemStats MeMStatsReader::getMemStats() {
    // Call the static helper function (which is now correctly defined as a member)
    // getPlatformMemStats now returns a single MemStats object directly.
    return getPlatformMemStats();
}

#if defined(__linux__)
// Public parser implementation for Linux
unsigned long long MeMStatsReader::parseMeminfoLine(const std::string& line, const std::string& key) {
    return parseMeminfoLineInternal(line, key);
}
#else
// On non-Linux platforms, this function is not applicable.
unsigned long long MeMStatsReader::parseMeminfoLine(const std::string& line, const std::string& key) {
    (void)line; (void)key; // Avoid unused parameter warning
    throw std::logic_error("parseMeminfoLine is only available on Linux.");
}
#endif


} // namespace SystemMemoryStats