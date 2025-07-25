#include "net_stats.hpp"

#include <stdexcept>
#include <string>
#include <vector>
#include <iostream> // For error logging/debugging
#include <numeric>  // For std::accumulate in aggregated stats

// Platform-specific headers
#if defined(_WIN32) || defined(__WIN64__)
    #include <winsock2.h>
    #include <iphlpapi.h>
    #include <stdio.h> // For sprintf_s
    #pragma comment(lib, "iphlpapi.lib")
    #pragma comment(lib, "ws2_32.lib") // For Winsock functions used by IPHlpApi
#elif defined(__linux__)
    #include <fstream>
    #include <sstream>
    #include <string_view> // For efficient string operations
#elif defined(__APPLE__)
    #include <sys/types.h>
    #include <sys/sysctl.h>
    #include <sys/socket.h>
    #include <net/if.h>       // For struct if_msghdr
    #include <net/if_dl.h>    // For struct sockaddr_dl
    #include <cstring>        // For memcpy
#endif

namespace SystemNetStats {

// --- Private Platform-Specific Helpers Definitions ---
// Note: 'static' keyword is NOT repeated here in the definitions outside the class.

#if defined(_WIN32) || defined(__WIN64__)
std::vector<NetStats> NetStatsReader::getRawWindowsNetStats() {
    std::vector<NetStats> all_stats;
    ULONG buffer_size = 0;

    // First call to get the required buffer size
    DWORD result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, NULL, &buffer_size);
    if (result != ERROR_BUFFER_OVERFLOW) {
        throw std::runtime_error("GetAdaptersAddresses failed to determine buffer size (Error: " + std::to_string(result) + ")");
    }

    std::vector<BYTE> adapter_addresses_buffer(buffer_size);
    PIP_ADAPTER_ADDRESSES pAdapterAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(adapter_addresses_buffer.data());

    // Second call to get the actual data
    result = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX, NULL, pAdapterAddresses, &buffer_size);
    if (result != ERROR_SUCCESS) {
        throw std::runtime_error("GetAdaptersAddresses failed to retrieve adapter information (Error: " + std::to_string(result) + ")");
    }

    for (PIP_ADAPTER_ADDRESSES pAdapter = pAdapterAddresses; pAdapter != NULL; pAdapter = pAdapter->Next) {
        // Only consider Ethernet or WiFi interfaces
        if (pAdapter->IfType == IF_TYPE_ETHERNET || pAdapter->IfType == IF_TYPE_IEEE80211) {
            std::string interface_name;
            if (pAdapter->FriendlyName) {
                // Convert wide string to narrow string
                int size_needed = WideCharToMultiByte(CP_UTF8, 0, pAdapter->FriendlyName, -1, NULL, 0, NULL, NULL);
                interface_name.resize(size_needed);
                WideCharToMultiByte(CP_UTF8, 0, pAdapter->FriendlyName, -1, &interface_name[0], size_needed, NULL, NULL);
                interface_name.pop_back(); // Remove null terminator
            } else {
                interface_name = "Unknown Interface " + std::to_string(pAdapter->IfIndex);
            }

            // Get per-adapter statistics
            MIB_IF_ROW2 ifRow;
            memset(&ifRow, 0, sizeof(ifRow));
            ifRow.InterfaceIndex = pAdapter->IfIndex;
            
            if (GetIfEntry2(&ifRow) == NO_ERROR) {
                NetStats stats(
                    interface_name,
                    ifRow.InOctets,      // Bytes received
                    ifRow.OutOctets,     // Bytes sent
                    ifRow.InUcastPkts + ifRow.InNucastPkts, // Unicast + Non-unicast packets in
                    ifRow.OutUcastPkts + ifRow.OutNucastPkts, // Unicast + Non-unicast packets out
                    ifRow.InErrors,      // Inbound errors
                    ifRow.OutErrors,     // Outbound errors
                    ifRow.InDiscards,    // Inbound discards (drops)
                    ifRow.OutDiscards    // Outbound discards (drops)
                );
                all_stats.push_back(std::move(stats));
            } else {
                std::cerr << "Warning: Failed to get IfEntry2 for interface: " << interface_name << std::endl;
            }
        }
    }
    return all_stats;
}

#elif defined(__linux__)
namespace { // Anonymous namespace for Linux-only internal helpers
// Helper to parse a line from /proc/net/dev
NetStats parseNetDevLine(const std::string& line) {
    std::istringstream iss(line);
    std::string interface_name;
    // Skip initial whitespace
    iss >> std::ws;
    iss >> interface_name;

    // Remove the colon from the interface name (e.g., "eth0:")
    if (!interface_name.empty() && interface_name.back() == ':') {
        interface_name.pop_back();
    }

    NetStats stats(interface_name);
    // The /proc/net/dev format is complex. We'll read specific fields.
    // Format: "interface:bytes_recieved packets_recieved errs drop fifo frames compressed multicast bytes_sent ..."
    // The numbers are separated by spaces.
    // Field indices (0-indexed after interface name, so 0 is bytes_received, 1 is packets_received etc.)
    // received: bytes(0) packets(1) errs(2) drop(3) fifo(4) frame(5) compressed(6) multicast(7)
    // transmitted: bytes(8) packets(9) errs(10) drop(11) fifo(12) colls(13) carrier(14) compressed(15)

    // Read received stats
    if (!(iss >> stats.bytes_received >> stats.packets_received >> stats.errors_in >> stats.drops_in)) {
        throw std::runtime_error("Failed to parse received stats for interface: " + interface_name);
    }
    // Skip 4 dummy fields for received (fifo, frame, compressed, multicast)
    unsigned long long dummy;
    for (int i = 0; i < 4; ++i) iss >> dummy;

    // Read transmitted stats
    if (!(iss >> stats.bytes_sent >> stats.packets_sent >> stats.errors_out >> stats.drops_out)) {
        throw std::runtime_error("Failed to parse transmitted stats for interface: " + interface_name);
    }

    return stats;
}
} // anonymous namespace

std::vector<NetStats> NetStatsReader::getRawLinuxNetStats() {
    std::ifstream file("/proc/net/dev");
    if (!file.is_open()) {
        throw std::runtime_error("Could not open /proc/net/dev. Ensure you have permissions.");
    }

    std::vector<NetStats> all_stats;
    std::string line;

    // Skip header lines (usually 2 lines)
    std::getline(file, line); // header 1
    std::getline(file, line); // header 2

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        try {
            NetStats stats = parseNetDevLine(line);
            // Example filtering for common virtual/non-physical interfaces, keeping "lo" now.
            if (stats.interface_name.find("veth") == std::string::npos &&
                stats.interface_name.find("docker") == std::string::npos &&
                stats.interface_name.find("br-") == std::string::npos) {
                all_stats.push_back(std::move(stats));
            }
        } catch (const std::runtime_error& e) {
            std::cerr << "Warning: Failed to parse net/dev line: " << line << " - " << e.what() << std::endl;
            // Continue to next line
        }
    }
    return all_stats;
}

#elif defined(__APPLE__)
std::vector<NetStats> NetStatsReader::getRawMacNetStats() {
    std::vector<NetStats> all_stats;
    int mib[] = {CTL_NET, AF_ROUTE, 0, 0, AF_UNSPEC, NET_RT_IFLIST};
    size_t len;

    // Get buffer size
    if (sysctl(mib, 6, NULL, &len, NULL, 0) < 0) {
        throw std::runtime_error("sysctl (NET_RT_IFLIST) failed to get buffer size.");
    }

    std::vector<char> buffer(len);
    if (sysctl(mib, 6, buffer.data(), &len, NULL, 0) < 0) {
        throw std::runtime_error("sysctl (NET_RT_IFLIST) failed to get data.");
    }

    char *lim = buffer.data() + len;
    char *next = buffer.data();

    while (next < lim) {
        struct if_msghdr *ifm = (struct if_msghdr *)next;
        next += ifm->ifm_msglen;

        if (ifm->ifm_type == RTM_IFINFO) {
            struct sockaddr_dl *sdl = (struct sockaddr_dl *)(ifm + 1);
            std::string interface_name(sdl->sdl_data, sdl->sdl_nlen);

            // Filter out non-physical interfaces like bridge/virtual ones if desired, but keep lo0 for testing
            if (interface_name.find("bridge") != std::string::npos ||
                interface_name.find("vboxnet") != std::string::npos) {
                continue;
            }

            NetStats stats(interface_name);
            stats.bytes_received = ifm->ifm_data.ifi_ibytes;
            stats.bytes_sent = ifm->ifm_data.ifi_obytes;
            stats.packets_received = ifm->ifm_data.ifi_ipackets;
            stats.packets_sent = ifm->ifm_data.ifi_opackets;
            stats.errors_in = ifm->ifm_data.ifi_ierrors;
            stats.errors_out = ifm->ifm_data.ifi_oerrors;
            // macOS if_msghdr doesn't directly provide 'drops', so setting to 0
            stats.drops_in = 0;
            stats.drops_out = 0;

            all_stats.push_back(std::move(stats));
        }
    }
    return all_stats;
}

#else // Unsupported platform
std::vector<NetStats> NetStatsReader::getRawUnsupportedNetStats() {
    throw std::runtime_error("Network statistics are not supported on this platform.");
}
#endif

/// @brief Private helper to dispatch to the correct platform-specific function.
/// This centralizes the platform selection logic.
std::vector<NetStats> NetStatsReader::getPlatformNetStats() {
#if defined(_WIN32) || defined(__WIN64__)
    return NetStatsReader::getRawWindowsNetStats();
#elif defined(__linux__)
    return NetStatsReader::getRawLinuxNetStats();
#elif defined(__APPLE__)
    return NetStatsReader::getRawMacNetStats();
#else
    return NetStatsReader::getRawUnsupportedNetStats();
#endif
}

// --- Public API Implementation ---

NetStats NetStatsReader::getNetStats() {
    const auto all_stats = NetStatsReader::getPlatformNetStats();

    NetStats aggregated_stats{"aggregated", 0, 0, 0, 0, 0, 0, 0, 0};
    for (const auto& stats : all_stats) {
        aggregated_stats.bytes_received += stats.bytes_received;
        aggregated_stats.bytes_sent += stats.bytes_sent;
        aggregated_stats.packets_received += stats.packets_received;
        aggregated_stats.packets_sent += stats.packets_sent;
        aggregated_stats.errors_in += stats.errors_in;
        aggregated_stats.errors_out += stats.errors_out;
        aggregated_stats.drops_in += stats.drops_in;
        aggregated_stats.drops_out += stats.drops_out;
    }

    return aggregated_stats;
}

NetStats NetStatsReader::getNetStats(const std::string& interface_name) {
    // Adding a check for empty interface_name here to throw std::invalid_argument as expected by some tests
    if (interface_name.empty()) {
        throw std::invalid_argument("Interface name cannot be empty.");
    }

    const auto all_stats = NetStatsReader::getPlatformNetStats();

    for (const auto& stats : all_stats) {
        if (stats.interface_name == interface_name) {
            return stats;
        }
    }

    throw std::runtime_error("Network interface '" + interface_name + "' not found or has no stats.");
}

} // namespace SystemNetStats