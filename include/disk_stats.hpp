#pragma once
#ifndef SYSTEM_DISK_STATS_H
#define SYSTEM_DISK_STATS_H
#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>
namespace SysytemDiskStats {

struct DiskStats {
    unsigned long long read_bytes, write_bytes;
    double read_rate, write_rate; // bytes/sec
};
class DiskStatsReader {
public:
    static DiskStats getDiskStats();
    static DiskStats getDiskStats(std::string& device);
    DiskStatsReader() = delete;
    ~DiskStatsReader() = delete;
    DiskStatsReader(const DiskStatsReader&) = delete;
    DiskStatsReader& operator=(const DiskStatsReader&) = delete;
};
}
#endif // SYSTEM_DISK_STATS_H
