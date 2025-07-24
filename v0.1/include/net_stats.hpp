#pragma once
#ifndef SYSTEM_NET_STATS_H
#define SYSTEM_NET_STATS_H
#include <istream>
#include <fstream>
#include <sstream>
#include <string>
#include <limits>
#include <stdexcept>
namespace SystemNetworkStats {
struct NetStats {
    unsigned long long rx_bytes, tx_bytes;
    double rx_rate, tx_rate; // bytes/sec
};
class NetworkStatsReader    {
public:
    static NetStats getNetStats();
    static NetStats getNetStats(std::istream& file_path);
    NetworkStatsReader() = delete;
    ~NetworkStatsReader() = delete;
    NetworkStatsReader(const NetworkStatsReader&) = delete;
    NetworkStatsReader& operator=(const NetworkStatsReader&) = delete;
private:
    inline static NetStats stats;
};
}
#endif // SYSTEM_NET_STATS_H