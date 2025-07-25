#ifndef NET_STATS_HPP
#define NET_STATS_HPP

#include <string>
#include <map>

namespace SystemNetworkStats {

    // Structure to hold network interface statistics.
    // The members correspond to fields in /proc/net/dev
    struct NetStats {
        std::string interface_name; // Name of the network interface (e.g., "eth0", "lo")
        unsigned long long rx_bytes;   // Bytes received
        unsigned long long rx_packets; // Packets received
        unsigned long long rx_errors;  // Receive errors
        unsigned long long rx_dropped; // Receive packets dropped
        unsigned long long tx_bytes;   // Bytes transmitted
        unsigned long long tx_packets; // Packets transmitted
        unsigned long long tx_errors;  // Transmit errors
        unsigned long long tx_dropped; // Transmit packets dropped

        NetStats(std::string name = "", unsigned long long rb = 0, unsigned long long tpb = 0,
                 unsigned long long re = 0, unsigned long long rdr = 0,
                 unsigned long long tb = 0, unsigned long long tpp = 0,
                 unsigned long long te = 0, unsigned long long tdr = 0)
            : interface_name(std::move(name)), rx_bytes(rb), rx_packets(tpb), rx_errors(re), rx_dropped(rdr),
              tx_bytes(tb), tx_packets(tpp), tx_errors(te), tx_dropped(tdr) {}
    };

    // Class to read and provide network interface statistics from the system.
    class NetworkStatsReader {
    public:
        // Retrieves aggregated network statistics for all active interfaces.
        // Returns a NetStats object representing the sum of all relevant network activities.
        static NetStats getNetStats();
        static std::map<std::string, NetStats> getNetStatsPerINterface();
        // Retrieves network statistics for a specific interface.
        // @param interface_name The name of the network interface (e.g., "eth0", "lo").
        // @return A NetStats object for the specified interface.
        // @throws std::runtime_error if the interface is not found or data cannot be read.
        static NetStats getNetStats(const std::string& interface_name);

    private:
        // Helper to parse a line from /proc/net/dev
        // This function will extract the relevant fields to populate a NetStats object.
        static NetStats parseNetStatLine(const std::string& line);
    };

} // namespace SystemNetworkStats

#endif // NET_STATS_HPP