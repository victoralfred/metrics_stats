#pragma once

#ifndef SYSTEM_CPU_STATS_H
#define SYSTEM_CPU_STATS_H
#include <istream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

namespace SystemCPUStats {
struct CPUStats {
    // CPU time fields
    double user;       // User CPU time
    double nice;       // Nice CPU time
    double system;     // System CPU time
    double idle;       // Idle CPU time
    double iowait;     // I/O wait CPU time
    double irq;        // Hardware interrupt CPU time
    double softirq;    // Software interrupt CPU time
    double steal;      // Steal time (time spent in other OS)
    double guest;      // Guest CPU time
    double guest_nice; // Guest nice CPU time
    double usage_percent;  // derived
};
class CPUStatsReader  {
    public:
    static CPUStats getCPUStats ();
    static CPUStats getCPUStats (std::istream& input);
    CPUStatsReader () = delete;
    ~CPUStatsReader () = delete;
    CPUStatsReader (const CPUStatsReader &) = delete;
    CPUStatsReader & operator=(const CPUStatsReader &) = delete;
private:
    inline static CPUStats stats;
};
}
#endif // SYSTEM_CPU_STATS_READER_H