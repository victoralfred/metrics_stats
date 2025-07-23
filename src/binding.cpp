#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "cpu_stats.hpp"
#include "mem_stats.hpp"

namespace py = pybind11;

// Bring types into local scope
using SystemCPUStats::CPUStats;
using SystemCPUStats::CPUStatsReader;
using SystemMemoryStats::MemStats;
using SystemMemoryStats::MeMStatsReader;

// Create function pointers to the static member functions
SystemCPUStats::CPUStats (*getCPUStatsFunc)() = &SystemCPUStats::CPUStatsReader::getCPUStats;
SystemMemoryStats::MemStats (*getMemStatsFunc)() = &SystemMemoryStats::MeMStatsReader::getMemStats;

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

    m.def("get_mem_stats", getMemStatsFunc);
    m.def("get_cpu_stats", getCPUStatsFunc);
}
