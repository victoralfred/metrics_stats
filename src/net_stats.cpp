#include "net_stats.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>

namespace SystemNetworkStats {

    NetStats NetworkStatsReader::parseNetStatLine(const std::string& line) {
        std::istringstream iss(line);
        NetStats stats;
        std::string interface_colon; // "eth0:" or "lo:" etc.

        // Fields from /proc/net/dev (whitespace separated after interface name):
        // (RX) bytes packets errs drop fifo frame compressed multicast | (TX) bytes packets errs drop fifo colls carrier compressed
        unsigned long long rx_bytes, rx_packets, rx_errors, rx_dropped;
        unsigned long long tx_bytes, tx_packets, tx_errors, tx_dropped;
        unsigned long long dummy; // For skipping other fields

        if (!(iss >> interface_colon // e.g., "eth0:"
                 >> rx_bytes
                 >> rx_packets
                 >> rx_errors
                 >> rx_dropped
                 >> dummy >> dummy >> dummy >> dummy // fifo, frame, compressed, multicast
                 >> tx_bytes
                 >> tx_packets
                 >> tx_errors
                 >> tx_dropped)) // fifo, colls, carrier, compressed
        {
            throw std::runtime_error("Failed to parse netstat line: " + line);
        }

        // Remove the trailing colon from the interface name
        if (!interface_colon.empty() && interface_colon.back() == ':') {
            stats.interface_name = interface_colon.substr(0, interface_colon.length() - 1);
        } else {
            stats.interface_name = interface_colon; // Should not happen if parsing correctly
        }

        stats.rx_bytes = rx_bytes;
        stats.rx_packets = rx_packets;
        stats.rx_errors = rx_errors;
        stats.rx_dropped = rx_dropped;
        stats.tx_bytes = tx_bytes;
        stats.tx_packets = tx_packets;
        stats.tx_errors = tx_errors;
        stats.tx_dropped = tx_dropped;

        return stats;
    }

    NetStats NetworkStatsReader::getNetStats() {
        std::ifstream file("/proc/net/dev");
        if (!file.is_open()) {
            throw std::runtime_error("Could not open /proc/net/dev for reading network statistics.");
        }

        NetStats aggregated_stats; // All members initialized to 0 by default constructor
        aggregated_stats.interface_name = "aggregated";

        std::string line;
        std::getline(file, line); // Skip header line 1
        std::getline(file, line); // Skip header line 2

        while (std::getline(file, line)) {
            // Trim leading whitespace for interfaces like "  eth0:"
            size_t first_char = line.find_first_not_of(" \t");
            if (first_char == std::string::npos) continue; // Empty or whitespace-only line
            std::string trimmed_line = line.substr(first_char);

            try {
                NetStats current_interface_stats = parseNetStatLine(trimmed_line);
                
                // Aggregate only non-loopback interfaces for a more "real" network usage.
                // You might adjust this logic based on what "aggregated" means for your use case.
                if (current_interface_stats.interface_name != "lo") {
                    aggregated_stats.rx_bytes += current_interface_stats.rx_bytes;
                    aggregated_stats.rx_packets += current_interface_stats.rx_packets;
                    aggregated_stats.rx_errors += current_interface_stats.rx_errors;
                    aggregated_stats.rx_dropped += current_interface_stats.rx_dropped;
                    aggregated_stats.tx_bytes += current_interface_stats.tx_bytes;
                    aggregated_stats.tx_packets += current_interface_stats.tx_packets;
                    aggregated_stats.tx_errors += current_interface_stats.tx_errors;
                    aggregated_stats.tx_dropped += current_interface_stats.tx_dropped;
                }

            } catch (const std::runtime_error& e) {
                // Log or ignore lines that cannot be parsed
                std::cerr << "Warning: Skipping malformed netstat line: " << e.what() << std::endl;
            }
        }
        return aggregated_stats;
    }

    NetStats NetworkStatsReader::getNetStats(const std::string& interface_name) {
        std::ifstream file("/proc/net/dev");
        if (!file.is_open()) {
            throw std::runtime_error("Could not open /proc/net/dev for reading network statistics.");
        }

        std::string line;
        std::getline(file, line); // Skip header line 1
        std::getline(file, line); // Skip header line 2

        while (std::getline(file, line)) {
            // Trim leading whitespace and check if the line contains the specified interface name
            size_t first_char = line.find_first_not_of(" \t");
            if (first_char == std::string::npos) continue;
            std::string trimmed_line = line.substr(first_char);

            // Check if the trimmed line starts with the interface name followed by a colon
            if (trimmed_line.rfind(interface_name + ":", 0) == 0) {
                return parseNetStatLine(trimmed_line);
            }
        }
        throw std::runtime_error("Network interface '" + interface_name + "' not found in /proc/net/dev.");
    }
    std::map<std::string, NetStats> NetworkStatsReader::getNetStatsPerINterface() {
        std::map<std::string, NetStats> stats_map;
        std::ifstream file("/proc/net/dev");
        if (!file.is_open()) {
            throw std::runtime_error("Could not open /proc/net/dev for reading network statistics.");
        }
        std::string line;
        std::getline(file, line); // Skip header line 1
        std::getline(file, line); // Skip header line 2
        while (std::getline(file, line)) {
            size_t first_char = line.find_first_not_of(" \t");
            if (first_char == std::string::npos) continue; // Empty or whitespace-only line
            std::string trimmed_line = line.substr(first_char);
            try {
                NetStats current_interface_stats = parseNetStatLine(trimmed_line);
                stats_map[current_interface_stats.interface_name] = current_interface_stats;
            } catch (const std::runtime_error& e) {
                std::cerr << "Warning: Skipping malformed netstat line: " << e.what() << std::endl;
            }
        }
        return stats_map;
    }

} // namespace SystemNetworkStats
