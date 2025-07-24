#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "cpu_stats.hpp"
#include "mem_stats.hpp"
#include "disk_stats.hpp"
#include "net_stats.hpp"

namespace py = pybind11;

// Bring types into local scope
using SystemCPUStats::CPUStats;
using SystemCPUStats::CPUStatsReader;
using SystemMemoryStats::MemStats;
using SystemMemoryStats::MeMStatsReader;
using SystemDiskStats::DiskStats;
using SystemDiskStats::DiskStatsReader;
using SystemNetworkStats::NetStats;
using SystemNetworkStats::NetworkStatsReader;

// Create function pointers to the static member functions
SystemCPUStats::CPUStats (*getCPUStatsFunc)() = &SystemCPUStats::CPUStatsReader::getCPUStats;
SystemMemoryStats::MemStats (*getMemStatsFunc)() = &SystemMemoryStats::MeMStatsReader::getMemStats;
SystemNetworkStats::NetStats (*getNetStatsFunc)() = &SystemNetworkStats::NetworkStatsReader::getNetStats;
SystemDiskStats::DiskStats (*getDiskStatsFunc)() = &SystemDiskStats::DiskStatsReader::getDiskStats;

PYBIND11_MODULE(sysstats, m) {
    py::class_<CPUStats>(m, "CPUStats", "Represents CPU statistics")
        .def_readonly("user", &CPUStats::user)
        .def_readonly("nice", &CPUStats::nice)
        .def_readonly("system", &CPUStats::system)
        .def_readonly("idle", &CPUStats::idle)
        .def_readonly("iowait", &CPUStats::iowait)
        .def_readonly("irq", &CPUStats::irq)
        .def_readonly("softirq", &CPUStats::softirq)
        .def_readonly("steal", &CPUStats::steal)
        .def_readonly("guest", &CPUStats::guest)
        .def_readonly("guest_nice", &CPUStats::guest_nice);

    py::class_<MemStats>(m, "MemStats", "Represents memory statistics")
        .def_readonly("total", &MemStats::total)
        .def_readonly("free", &MemStats::free)
        .def_readonly("available", &MemStats::available)
        .def_readonly("buffers", &MemStats::buffers)
        .def_readonly("cached", &MemStats::cached);
    py::class_<DiskStats>(m, "DiskStats", "Represents disk statistics")
        .def_readonly("read_bytes", &DiskStats::read_bytes)
        .def_readonly("write_bytes", &DiskStats::write_bytes);
    py::class_<NetStats>(m, "NetStats", "Represents network statistics")
        .def_readonly("rx_bytes", &NetStats::rx_bytes)
        .def_readonly("tx_bytes", &NetStats::tx_bytes)
        .def_readonly("rx_rate", &NetStats::rx_rate)
        .def_readonly("tx_rate", &NetStats::tx_rate);

    m.def("get_disk_stats", getDiskStatsFunc, "Get disk statistics");
    m.def("get_net_stats", getNetStatsFunc, "Get network statistics");
    m.def("get_mem_stats", getMemStatsFunc, "Get memory statistics");
    m.def("get_cpu_stats", getCPUStatsFunc, "Get CPU statistics");
}
