#ifndef SYSTEM_CPU_STATS_H
#define SYSTEM_CPU_STATS_H

#include <string>      // For std::string
#include <vector>      // For std::vector (if needed in future, currently not directly used)
#include <stdexcept>   // For standard exceptions like std::runtime_error
#include <istream>     // For std::istream in the overloaded getCPUStats

namespace SystemCPUStats {
struct CPUStats {
    double user;       ///< User CPU time
    double nice;       ///< Nice CPU time
    double system;     ///< System CPU time
    double idle;       ///< Idle CPU time
    double iowait;     ///< I/O wait CPU time
    double irq;        ///< Hardware interrupt CPU time
    double softirq;    ///< Software interrupt CPU time
    double steal;      ///< Steal time (time spent in other OS or virtualized environment)
    double guest;      ///< Guest CPU time (time spent running a virtual CPU for guest OS)
    double guest_nice; ///< Guest nice CPU time (guest CPU time with a 'nice' value)
    double usage_percent; ///< Derived CPU usage percentage based on the raw times. Will be 0.0 if not explicitly calculated.

    // Constructor to ensure proper initialization of all members.
    CPUStats(double u = 0.0, double n = 0.0, double s = 0.0, double i = 0.0,
             double io = 0.0, double ir = 0.0, double si = 0.0, double st = 0.0,
             double g = 0.0, double gn = 0.0, double up = 0.0)
        : user(u), nice(n), system(s), idle(i), iowait(io), irq(ir),
          softirq(si), steal(st), guest(g), guest_nice(gn), usage_percent(up) {}

    /// @brief Calculates the total non-idle CPU time.
    /// @return Sum of user, nice, system, iowait, irq, softirq, steal, guest, guest_nice.
    double getTotalActiveTime() const {
        return user + nice + system + iowait + irq + softirq + steal + guest + guest_nice;
    }

    /// @brief Calculates the total CPU time including idle.
    /// @return Sum of all CPU time fields.
    double getTotalTime() const {
        return getTotalActiveTime() + idle;
    }
};

/// @brief Calculates the CPU usage percentage based on two CPUStats snapshots.
/// This is a pure static helper function, accessible publicly.
/// @param curr The current CPUStats snapshot.
/// @param prev The previous CPUStats snapshot.
/// @param time_delta_ms The time difference in milliseconds between the two snapshots.
///        This is used to prevent division by zero if CPU times remain unchanged.
/// @return The calculated CPU usage percentage (0.0 - 100.0). Returns 0.0 if total_delta is 0.
/// @note This function requires the caller to manage the previous state and time delta.
double calculateUsagePercentage(const CPUStats& curr, const CPUStats& prev, long long time_delta_ms);

// ---

/// @brief Utility class for reading system CPU statistics.
/// This class is designed to be purely static, providing functions to retrieve CPU data.
/// It cannot be instantiated and does not hold any internal state related to CPU stats.
class CPUStatsReader {
public:
    /// @brief Retrieves the current CPU statistics from the system's default source.
    /// @return A CPUStats object containing the current CPU statistics.
    ///         The `usage_percent` field will always be 0.0 as this class holds no state to calculate it.
    /// @throws std::runtime_error if the CPU statistics cannot be read (e.g., file not found, parsing error).
    /// @note The actual implementation of this function will be platform-dependent.
    static CPUStats getCPUStats();

    /// @brief Retrieves CPU statistics from a provided input stream.
    /// This is useful for testing or reading from non-standard sources.
    /// @param input An std::istream reference from which to read the CPU statistics data.
    /// @return A CPUStats object parsed from the input stream.
    ///         The `usage_percent` field will always be 0.0 as this class holds no state to calculate it.
    /// @throws std::runtime_error if the data from the stream cannot be parsed correctly.
    static CPUStats getCPUStats(std::istream& input);

private:
    // Prevent instantiation of this utility class.
    CPUStatsReader() = delete;
    ~CPUStatsReader() = delete;
    // Prevent copying and assignment of this utility class.
    // This ensures it remains a purely static interface.
    CPUStatsReader(const CPUStatsReader&) = delete;
    CPUStatsReader& operator=(const CPUStatsReader&) = delete;

    // Platform-specific raw CPUStats retrieval functions
#if defined(_WIN32)
    static CPUStats getRawWindowsCpuStats();
#elif defined(__linux__)
    static CPUStats getRawLinuxCpuStats();
#elif defined(__APPLE__)
    static CPUStats getRawMacCpuStats();
#else
    // Fallback for unsupported platforms if no specific implementation is defined
    static CPUStats getRawUnsupportedCpuStats();
#endif

    /// @brief Helper function to parse a /proc/stat-like line.
    /// This function is private as it's an internal detail of how data is read from streams.
    static CPUStats parseProcStatLine(std::istream& input_stream);
};

} // namespace SystemCPUStats

#endif // SYSTEM_CPU_STATS_H