#ifndef MEM_STATS_HPP
#define MEM_STATS_HPP

#include <string>
#define _WIN32_WINNT 0x0501 // Required for MEMORYSTATUSEX and GlobalMemoryStatusEx
// No need for <map> in the header unless MemStats struct or public methods
// directly use it, which they don't in this design.

namespace SystemMemoryStats {

    // Structure to hold memory statistics.
    // The members correspond to relevant fields from system memory APIs across platforms.
    struct MemStats {
        unsigned long long total;       // Total physical memory in KB
        unsigned long long free;        // Free physical memory in KB
        unsigned long long available;   // Available memory for applications in KB (more accurate than 'free')
        unsigned long long buffers;     // Memory used by kernel buffers/system caches in KB
        unsigned long long cached;      // Memory used by the page cache in KB
        unsigned long long swap_total;  // Total swap space in KB
        unsigned long long swap_free;   // Free swap space in KB

        // Default constructor initializes all members to 0
        MemStats(unsigned long long t = 0, unsigned long long f = 0,
                 unsigned long long a = 0, unsigned long long b = 0,
                 unsigned long long c = 0, unsigned long long st = 0,
                 unsigned long long sf = 0)
            : total(t), free(f), available(a), buffers(b), cached(c),
              swap_total(st), swap_free(sf) {}
    };

    // Class to read and provide memory statistics from the system.
    class MeMStatsReader {
    public:
        /// @brief Retrieves current memory statistics for the system.
        /// This function abstracts away platform-specific implementation details.
        /// @return A MemStats object containing the memory information.
        /// @throws std::runtime_error if memory statistics are not supported on the platform or if there's a system error.
        static MemStats getMemStats();

#if defined(__linux__)
        /// @brief Linux-specific helper to parse a line from /proc/meminfo.
        /// Extracts the numerical value for a given key.
        /// This static method is exposed here as it's a specific parsing utility.
        /// @param line The line from /proc/meminfo (e.g., "MemTotal:        8000000 kB").
        /// @param key The key to look for (e.g., "MemTotal", "MemFree").
        /// @return The extracted unsigned long long value in KB.
        /// @throws std::runtime_error if the key is not found or parsing fails.
        static unsigned long long parseMeminfoLine(const std::string& line, const std::string& key);
#endif

    private:
        // --- Private Static Forward Declarations for Platform-Specific Implementations ---
        // These functions will be implemented in the corresponding .cpp files for each platform
        // (e.g., mem_stats_win.cpp, mem_stats_linux.cpp, mem_stats_mac.cpp).
        // Each will return a MemStats object containing the raw memory statistics.
        static MemStats getRawWindowsMemStats();
        static MemStats getRawLinuxMemStats();
        static MemStats getRawMacMemStats();
        static MemStats getRawUnsupportedMemStats();

        /// @brief Private helper to dispatch to the correct platform-specific function.
        /// This centralizes the platform selection logic, keeping the public API clean.
        static MemStats getPlatformMemStats();
    };

} // namespace SystemMemoryStats

#endif // MEM_STATS_HPP