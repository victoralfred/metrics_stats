#ifndef NET_STATS_HPP
#define NET_STATS_HPP

#include <string>
#include <vector> // For returning a collection of stats from raw getters
#include <stdexcept> // For std::runtime_error

// Forward declarations for platform-specific data structures or includes
// Note: Actual platform-specific headers will be included in the .cpp implementation files.

namespace SystemNetStats {

    /// @brief Structure to hold network interface statistics.
    /// Members represent common network interface metrics.
    struct NetStats {
        std::string interface_name;   ///< Name of the network interface (e.g., "eth0", "en0", "Ethernet")
        unsigned long long bytes_received; ///< Total bytes received
        unsigned long long bytes_sent;     ///< Total bytes sent
        unsigned long long packets_received; ///< Total packets received
        unsigned long long packets_sent;     ///< Total packets sent
        unsigned long long errors_in;      ///< Receive errors
        unsigned long long errors_out;     ///< Transmit errors
        unsigned long long drops_in;       ///< Inbound packets dropped
        unsigned long long drops_out;      ///< Outbound packets dropped

        // Default constructor initializes all members
        NetStats(std::string name = "unknown",
                 unsigned long long br = 0, unsigned long long bs = 0,
                 unsigned long long pr = 0, unsigned long long ps = 0,
                 unsigned long long ei = 0, unsigned long long eo = 0,
                 unsigned long long di = 0, unsigned long long dou = 0)
            : interface_name(std::move(name)),
              bytes_received(br), bytes_sent(bs),
              packets_received(pr), packets_sent(ps),
              errors_in(ei), errors_out(eo),
              drops_in(di), drops_out(dou) {}
    };

    /// @brief Class to read and provide network interface statistics from the system.
    /// Provides a cross-platform interface for accessing network statistics.
    class NetStatsReader {
    public:
        /// @brief Retrieves aggregated network statistics across all active interfaces.
        /// This method sums up the statistics from all available network interfaces.
        /// @return A NetStats object containing the aggregated network information.
        /// @throws std::runtime_error if network statistics are not supported on the platform
        /// or if there's an error during retrieval.
        static NetStats getNetStats();

        /// @brief Retrieves network statistics for a specific network interface.
        /// @param interface_name The name of the network interface (e.g., "eth0", "en0").
        /// @return A NetStats object containing the specified interface's information.
        /// @throws std::runtime_error if the specified device is not found or has no stats,
        /// or if network statistics are not supported on the platform.
        static NetStats getNetStats(const std::string& interface_name);

    private:
        /// @brief Private helper to dispatch to the correct platform-specific function.
        /// This centralizes the platform selection logic, returning a vector of raw stats.
        static std::vector<NetStats> getPlatformNetStats();

        // --- Forward Declarations for Private Platform-Specific Helpers ---
        // These functions will be implemented in the respective .cpp files for each platform.
        // They return a vector because a system can have multiple network interfaces.
        static std::vector<NetStats> getRawWindowsNetStats();
        static std::vector<NetStats> getRawLinuxNetStats();
        static std::vector<NetStats> getRawMacNetStats();
        static std::vector<NetStats> getRawUnsupportedNetStats();

        // Add any platform-specific parsing helpers here if needed,
        // similar to parseDiskStatLine in disk_stats.
        // For example, on Linux, you might have:
        // #if defined(__linux__)
        // static NetStats parseNetDevLine(const std::string& line, const std::string& interface_name);
        // #endif
    };

} // namespace SystemNetStats

#endif // NET_STATS_HPP