#include "net_stats.hpp"
namespace SystemNetworkStats {
NetStats NetworkStatsReader::getNetStats(std::istream& file_path) {
    if (!file_path || !file_path.good()) {
        throw std::runtime_error("Input file stream is not valid or not open");
    }
    // This function reads network stats from a file, typically /proc/net/dev
    std::string line;
    std::string iface = "eth0"; // Default interface, can be parameterized
    std::printf("Searching for interface: %s\n", iface.c_str());
    bool found = false;
    // Check if the specified interface exists in the file
    while (std::getline(file_path, line)) {
        if (line.find(iface + ":") != std::string::npos) {
            found = true;
            break;
        }
    }
    if (!found) {
        throw std::runtime_error("Interface not found in  /proc/net/dev: " + iface);
    }
    std::printf("Interface %s found, extracting stats\n", iface.c_str());
    // Reset the file stream to the beginning  
    file_path.clear();
    file_path.seekg(0, std::ios::beg);
    while (std::getline(file_path, line)) {
        if (line.find(iface + ":") != std::string::npos) {
            std::istringstream ss(line);
            std::string iface_name;
            unsigned long long rx_bytes, tx_bytes;
            ss >> iface_name >> rx_bytes; // Read RX bytes
            ss.ignore(std::numeric_limits<std::streamsize>::max(), ' '); // Skip the rest of the line until TX bytes
            ss >> tx_bytes; // Read TX bytes
            stats.rx_bytes = rx_bytes;
            stats.tx_bytes = tx_bytes;
            stats.rx_rate = 0.0; // Placeholder for RX rate
            stats.tx_rate = 0.0; // Placeholder for TX rate
            return stats;
        }
    }
    throw std::runtime_error("Interface stats not found after seeking to beginning.");
}
NetStats NetworkStatsReader::getNetStats() {
    std::ifstream file("/proc/net/dev");
    if (!file.is_open()) {
        throw std::runtime_error("Could not open /proc/net/dev");
    }
    return NetworkStatsReader::getNetStats(file);
}
}