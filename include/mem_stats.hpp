#pragma once
#ifndef SYSTEM_MEMORY_STATS_H
#define SYSTEM_MEMORY_STATS_H
#include <istream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
namespace SystemMemoryStats {
struct MemStats {
    unsigned long total, free, available, buffers, cached;
    double used_percent;  // derived
};
class MeMStatsReader  {
public:
    static MemStats  getMemStats ();
    static MemStats getMemStats (std::istream& input);
    MeMStatsReader() = delete;
   ~MeMStatsReader() = delete;
    MeMStatsReader(const MeMStatsReader&) = delete;
    MeMStatsReader& operator=(const MeMStatsReader&) = delete; 
private:
    inline static MemStats stats;
};
}
#endif // SYSTEM_MEMORY_STATS_H