#include <pybind11/pybind11.h>
#include <pybind11/stl.h> // Required for std::vector for DiskStats/NetStats
#include <pybind11/iostream.h> // For redirecting C++ streams to Python print
#include <thread> // For std::this_thread::sleep_for
#include <chrono> // For std::chrono::milliseconds
#include <sstream> // For std::stringstream
#include <map> // Required if any C++ function returns std::map (e.g., if a future getNetStatsPerInterface returns map)

// Include your C++ headers for the various stats
#include "cpu_stats.hpp"
#include "mem_stats.hpp"
#include "disk_stats.hpp"
#include "net_stats.hpp"

namespace py = pybind11;

// Alias namespaces for convenience in bindings
namespace scs = SystemCPUStats;
using SystemMemoryStats::MemStats;
using SystemMemoryStats::MeMStatsReader;
using SystemDiskStats::DiskStats;
using SystemDiskStats::DiskStatsReader;
using SystemNetStats::NetStats; // Corrected from SystemNetworkStats
using SystemNetStats::NetStatsReader; // Corrected from NetworkStatsReader

// --- New structs and functions for throughput calculations ---
// These are defined here assuming they are utility functions that might not
// belong directly to the reader classes, or for simplicity in this binding file.

// Struct to hold network throughput results in KB/s
struct NetThroughputResult {
    double rx_kbps; // Receive throughput in kilobytes per second
    double tx_kbps; // Transmit throughput in kilobytes per second

    // Constructor for easier initialization
    NetThroughputResult(double rx = 0.0, double tx = 0.0) : rx_kbps(rx), tx_kbps(tx) {}
};

// Function to calculate network throughput between two NetStats snapshots
NetThroughputResult calculateNetworkThroughput(const NetStats& current_stats, const NetStats& previous_stats, long long time_delta_ms) {
    if (time_delta_ms <= 0) {
        // Prevent division by zero or negative time. Raise a Python ValueError.
        throw py::value_error("Time delta must be positive for throughput calculation.");
    }

    // Calculate byte differences using the correct member names from NetStats struct
    double delta_rx_bytes = static_cast<double>(current_stats.bytes_received - previous_stats.bytes_received);
    double delta_tx_bytes = static_cast<double>(current_stats.bytes_sent - previous_stats.bytes_sent);

    // Convert time delta from milliseconds to seconds
    double time_delta_s = static_cast<double>(time_delta_ms) / 1000.0;

    // Calculate throughput in KB/s (bytes / 1024 / seconds)
    double rx_kbps = (delta_rx_bytes / 1024.0) / time_delta_s;
    double tx_kbps = (delta_tx_bytes / 1024.0) / time_delta_s;

    return NetThroughputResult(rx_kbps, tx_kbps);
}

// Struct to hold disk I/O throughput results in KB/s
struct DiskThroughputResult {
    double read_kbps;  // Read throughput in kilobytes per second
    double write_kbps; // Write throughput in kilobytes per second

    // Constructor for easier initialization
    DiskThroughputResult(double read = 0.0, double write = 0.0) : read_kbps(read), write_kbps(write) {}
};

// Function to calculate disk I/O throughput between two DiskStats snapshots
DiskThroughputResult calculateDiskIOThroughput(const DiskStats& current_stats, const DiskStats& previous_stats, long long time_delta_ms) {
    if (time_delta_ms <= 0) {
        // Prevent division by zero or negative time. Raise a Python ValueError.
        throw py::value_error("Time delta must be positive for throughput calculation.");
    }

    // Calculate byte differences
    double delta_read_bytes = static_cast<double>(current_stats.read_bytes - previous_stats.read_bytes);
    double delta_write_bytes = static_cast<double>(current_stats.write_bytes - previous_stats.write_bytes);

    // Convert time delta from milliseconds to seconds
    double time_delta_s = static_cast<double>(time_delta_ms) / 1000.0;

    // Calculate throughput in KB/s (bytes / 1024 / seconds)
    double read_kbps = (delta_read_bytes / 1024.0) / time_delta_s;
    double write_kbps = (delta_write_bytes / 1024.0) / time_delta_s;

    return DiskThroughputResult(read_kbps, write_kbps);
}

// Helper function to call CPUStatsReader::getCPUStats(std::istream&) from a Python string.
// Python users will typically pass strings, not std::istream objects.
scs::CPUStats get_cpu_stats_from_string(const std::string& input_str) {
    std::stringstream ss(input_str);
    return scs::CPUStatsReader::getCPUStats(ss);
}

PYBIND11_MODULE(py_metrics_agent, m) { // Module name as requested for py_metrics_agent
    m.doc() = "Pybind11 plugin for system metrics agent (CPU, Memory, Disk, Network)"; // Updated docstring

    // Optional: Redirect C++ std::cout and std::cerr to Python's sys.stdout/stderr
    py::add_ostream_redirect(m, "ostream_redirect");

    // --- CPU Statistics Bindings ---
    py::class_<scs::CPUStats>(m, "CPUStats")
        .def(py::init<>()) // Default constructor: CPUStats()
        // Parameterized constructor matching the C++ CPUStats definition
        .def(py::init<double, double, double, double, double, double, double, double, double, double, double>(),
             py::arg("user") = 0.0, py::arg("nice") = 0.0, py::arg("system") = 0.0,
             py::arg("idle") = 0.0, py::arg("iowait") = 0.0, py::arg("irq") = 0.0,
             py::arg("softirq") = 0.0, py::arg("steal") = 0.0, py::arg("guest") = 0.0,
             py::arg("guest_nice") = 0.0, py::arg("usage_percent") = 0.0)
        // Expose public data members as Python attributes (read/write)
        .def_readwrite("user", &scs::CPUStats::user)
        .def_readwrite("nice", &scs::CPUStats::nice)
        .def_readwrite("system", &scs::CPUStats::system)
        .def_readwrite("idle", &scs::CPUStats::idle)
        .def_readwrite("iowait", &scs::CPUStats::iowait)
        .def_readwrite("irq", &scs::CPUStats::irq)
        .def_readwrite("softirq", &scs::CPUStats::softirq)
        .def_readwrite("steal", &scs::CPUStats::steal)
        .def_readwrite("guest", &scs::CPUStats::guest)
        .def_readwrite("guest_nice", &scs::CPUStats::guest_nice)
        .def_readwrite("usage_percent", &scs::CPUStats::usage_percent)
        // Expose C++ methods as Python methods
        .def("get_total_active_time", &scs::CPUStats::getTotalActiveTime,
             "Calculates the total non-idle CPU time from the snapshot.")
        .def("get_total_time", &scs::CPUStats::getTotalTime,
             "Calculates the total CPU time including idle from the snapshot.")
        // Custom __repr__ for better Python print output
        .def("__repr__",
            [](const scs::CPUStats &s) {
                return "<CPUStats user=" + std::to_string(s.user) +
                       ", system=" + std::to_string(s.system) +
                       ", idle=" + std::to_string(s.idle) +
                       ", usage=" + std::to_string(s.usage_percent) + "%>";
            });

    // Bind CPUStatsReader's static methods directly to the module
    m.def("get_cpu_stats", static_cast<scs::CPUStats (*)()>(&scs::CPUStatsReader::getCPUStats),
        "Retrieves current CPU statistics from the system's default source. Returns raw data.");

    m.def("get_cpu_stats_from_string", &get_cpu_stats_from_string,
        "Retrieves CPU statistics from a provided input string (e.g., mock /proc/stat data).",
        py::arg("input_str"));

    // Bind the standalone calculateUsagePercentage function
    m.def("calculate_cpu_usage_percentage", &scs::calculateUsagePercentage, // Renamed for clarity in Python
        "Calculates the CPU usage percentage between two CPUStats snapshots over a given time delta.",
        py::arg("current_stats"), py::arg("previous_stats"), py::arg("time_delta_ms"));

    // Optional: Add a convenient Python function to get CPU usage over a period.
    m.def("get_cpu_usage_over_time",
          [](double interval_ms) {
              if (interval_ms <= 0) {
                  throw py::value_error("Interval must be positive.");
              }
              // Get first snapshot
              scs::CPUStats prev_stats = scs::CPUStatsReader::getCPUStats();
              // Release GIL while sleeping (good practice for long C++ operations)
              py::gil_scoped_release release;
              std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long long>(interval_ms)));
              py::gil_scoped_acquire acquire; // Acquire GIL back after sleep
              // Get second snapshot
              scs::CPUStats current_stats = scs::CPUStatsReader::getCPUStats();
              // Calculate and return percentage
              return scs::calculateUsagePercentage(current_stats, prev_stats, static_cast<long long>(interval_ms));
          },
          "Gets CPU usage percentage by taking two snapshots with a specified interval (in milliseconds).",
          py::arg("interval_ms"));

    // --- Disk Statistics Bindings ---
    py::class_<DiskStats>(m, "DiskStats")
        .def(py::init<>()) // Default constructor
        // Constructor matching the full DiskStats struct
        .def(py::init<std::string, unsigned long long, unsigned long long, unsigned long long, unsigned long long>(),
             py::arg("device") = "", py::arg("read_bytes") = 0, py::arg("write_bytes") = 0,
             py::arg("read_time_ms") = 0, py::arg("write_time_ms") = 0)
        // Expose public data members for DiskStats
        .def_readwrite("device", &DiskStats::device)
        .def_readwrite("read_bytes", &DiskStats::read_bytes)
        .def_readwrite("write_bytes", &DiskStats::write_bytes)
        .def_readwrite("read_time_ms", &DiskStats::read_time_ms)
        .def_readwrite("write_time_ms", &DiskStats::write_time_ms)
        .def("__repr__", [](const DiskStats &stats) {
            return "<DiskStats(device=" + stats.device +
                ", read_bytes=" + std::to_string(stats.read_bytes) +
                ", write_bytes=" + std::to_string(stats.write_bytes) + ")>";
        });

    // Bind the static functions for DiskStatsReader
    m.def("get_disk_stats_aggregated", // Renamed for clarity for Python consumers
        py::overload_cast<>(&DiskStatsReader::getDiskStats),
        "Get aggregated disk stats for all physical devices.");

    m.def("get_disk_stats_by_device", // Renamed for clarity for Python consumers
        py::overload_cast<const std::string&>(&DiskStatsReader::getDiskStats),
        "Get disk stats for a specific device.",
        py::arg("device_name"));

    // Bind the DiskThroughputResult struct for Python access
    py::class_<DiskThroughputResult>(m, "DiskThroughputResult")
        .def(py::init<>()) // Default constructor
        .def_readwrite("read_kbps", &DiskThroughputResult::read_kbps)
        .def_readwrite("write_kbps", &DiskThroughputResult::write_kbps)
        .def("__repr__", [](const DiskThroughputResult &s) {
            return "<DiskThroughputResult read_kbps=" + std::to_string(s.read_kbps) +
                   ", write_kbps=" + std::to_string(s.write_kbps) + ">";
        });

    // Bind the calculateDiskIOThroughput function
    m.def("calculate_disk_io_throughput", &calculateDiskIOThroughput,
        "Calculates disk I/O throughput (KB/s) between two DiskStats snapshots.",
        py::arg("current_stats"), py::arg("previous_stats"), py::arg("time_delta_ms"));


    // --- Memory Statistics Bindings ---
    py::class_<MemStats>(m, "MemStats")
        .def(py::init<>()) // Default constructor
        .def_readwrite("total", &MemStats::total)
        .def_readwrite("free", &MemStats::free)
        .def_readwrite("available", &MemStats::available)
        .def_readwrite("buffers", &MemStats::buffers)
        .def_readwrite("cached", &MemStats::cached)
        .def_readwrite("swap_total", &MemStats::swap_total)
        .def_readwrite("swap_free", &MemStats::swap_free)
        .def("__repr__", [](const MemStats &s) {
            return "<MemStats total=" + std::to_string(s.total) +
                   ", free=" + std::to_string(s.free) +
                   ", available=" + std::to_string(s.available) +
                   ", cached=" + std::to_string(s.cached) + ">";
        });

    // FIX: Corrected binding for static getMemStats() method - removed unnecessary static_cast
    m.def("get_mem_stats", &MeMStatsReader::getMemStats,
        "Retrieves current memory statistics.");

    // --- Network Statistics Bindings ---
    // Bind NetThroughputResult struct
    py::class_<NetThroughputResult>(m, "NetThroughputResult")
        .def(py::init<>())
        .def_readwrite("rx_kbps", &NetThroughputResult::rx_kbps)
        .def_readwrite("tx_kbps", &NetThroughputResult::tx_kbps)
        .def("__repr__", [](const NetThroughputResult &s) {
            return "<NetThroughputResult rx_kbps=" + std::to_string(s.rx_kbps) +
                   "KB/s, tx_kbps=" + std::to_string(s.tx_kbps) + "KB/s>";
        });

    // Bind the NetStats struct to a Python class
    py::class_<NetStats>(m, "NetStats")
        .def(py::init<>()) // Default constructor
        // Constructor matching the full NetStats struct in net_stats.hpp
        .def(py::init<std::string, unsigned long long, unsigned long long, unsigned long long, unsigned long long,
                      unsigned long long, unsigned long long, unsigned long long, unsigned long long>(),
             py::arg("interface_name") = "",
             py::arg("bytes_received") = 0, py::arg("bytes_sent") = 0, py::arg("packets_received") = 0, py::arg("packets_sent") = 0,
             py::arg("errors_in") = 0, py::arg("errors_out") = 0, py::arg("drops_in") = 0, py::arg("drops_out") = 0) // Corrected tx_errors to errors_out here too
        // Bind each member variable directly. py::rw allows read/write access.
        .def_readwrite("interface_name", &NetStats::interface_name)
        .def_readwrite("bytes_received", &NetStats::bytes_received)
        .def_readwrite("bytes_sent", &NetStats::bytes_sent)
        .def_readwrite("packets_received", &NetStats::packets_received)
        .def_readwrite("packets_sent", &NetStats::packets_sent)
        .def_readwrite("errors_in", &NetStats::errors_in)
        .def_readwrite("errors_out", &NetStats::errors_out)
        .def_readwrite("drops_in", &NetStats::drops_in)
        .def_readwrite("drops_out", &NetStats::drops_out)
        // Add a __repr__ method for better printing in Python
        .def("__repr__",
             [](const NetStats &s) {
                 return "<NetStats interface_name='" + s.interface_name +
                        "', bytes_received=" + std::to_string(s.bytes_received) +
                        ", bytes_sent=" + std::to_string(s.bytes_sent) + ">";
             });

    // Bind the NetStatsReader class to a Python class
    // All methods are static, so no need for a constructor binding for the class itself
    m.def("get_net_stats_aggregated", // Renamed for clarity for Python consumers
        py::overload_cast<>(&NetStatsReader::getNetStats),
        "Retrieves aggregated network statistics across all active interfaces.");

    m.def("get_net_stats_by_interface", // Renamed for clarity for Python consumers
        py::overload_cast<const std::string&>(&NetStatsReader::getNetStats),
        "Retrieves network statistics for a specific network interface.",
        py::arg("interface_name"));

    // Bind the calculateNetworkThroughput function
    m.def("calculate_network_throughput", &calculateNetworkThroughput,
        "Calculates network throughput (KB/s) between two NetStats snapshots.",
        py::arg("current_stats"), py::arg("previous_stats"), py::arg("time_delta_ms"));
}
