#ifndef MEM_STATS_HPP
#define MEM_STATS_HPP

#include <string>
#include <map>

namespace SystemMemoryStats {

    // Structure to hold memory statistics.
    // The members correspond to relevant fields in /proc/meminfo.
    struct MemStats {
        unsigned long long total;       // Total physical memory in KB
        unsigned long long free;        // Free physical memory in KB
        unsigned long long available;   // Available memory for applications in KB (more accurate than 'free')
        unsigned long long buffers;     // Memory used by kernel buffers in KB
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
    class MeMStatsReader { // Corrected from original 'MeMStatsReader' if it was a typo, otherwise keeping it.
    public:
        // Retrieves current memory statistics from /proc/meminfo.
        // @return A MemStats object containing the memory information.
        // @throws std::runtime_error if /proc/meminfo cannot be opened or parsed.
        static MemStats getMemStats();

    private:
        // Helper to parse a line from /proc/meminfo (e.g., "MemTotal:        8000000 kB")
        // Extracts the numerical value for a given key.
        static unsigned long long parseMeminfoLine(const std::string& line, const std::string& key);
    };

} // namespace SystemMemoryStats

#endif // MEM_STATS_HPP