#include "mem_stats.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>

namespace SystemMemoryStats {

    unsigned long long MeMStatsReader::parseMeminfoLine(const std::string& line, const std::string& key) {
        // Look for the key at the beginning of the line
        if (line.rfind(key, 0) == 0) { // Check if 'line' starts with 'key'
            std::istringstream iss(line);
            std::string temp_key;
            unsigned long long value;
            std::string unit; // Should be "kB"

            iss >> temp_key >> value >> unit;

            if (iss.fail() || unit != "kB") {
                throw std::runtime_error("Failed to parse value for key '" + key + "' from line: " + line);
            }
            return value;
        }
        return 0; // Return 0 if the key is not found on this line
    }

    MemStats MeMStatsReader::getMemStats() {
        std::ifstream file("/proc/meminfo");
        if (!file.is_open()) {
            throw std::runtime_error("Could not open /proc/meminfo for reading memory statistics.");
        }

        MemStats stats;
        std::string line;

        // Use a map to track found keys to prevent double counting or unnecessary parsing.
        // This is useful if parsing stops after certain keys are found for optimization,
        // though for meminfo, it's typically quick to read the whole file.
        std::map<std::string, bool> found_keys;

        while (std::getline(file, line)) {
            try {
                if (line.rfind("MemTotal:", 0) == 0) {
                    stats.total = parseMeminfoLine(line, "MemTotal:");
                    found_keys["MemTotal"] = true;
                } else if (line.rfind("MemFree:", 0) == 0) {
                    stats.free = parseMeminfoLine(line, "MemFree:");
                    found_keys["MemFree"] = true;
                } else if (line.rfind("MemAvailable:", 0) == 0) {
                    stats.available = parseMeminfoLine(line, "MemAvailable:");
                    found_keys["MemAvailable"] = true;
                } else if (line.rfind("Buffers:", 0) == 0) {
                    stats.buffers = parseMeminfoLine(line, "Buffers:");
                    found_keys["Buffers"] = true;
                } else if (line.rfind("Cached:", 0) == 0) {
                    stats.cached = parseMeminfoLine(line, "Cached:");
                    found_keys["Cached"] = true;
                } else if (line.rfind("SwapTotal:", 0) == 0) {
                    stats.swap_total = parseMeminfoLine(line, "SwapTotal:");
                    found_keys["SwapTotal"] = true;
                } else if (line.rfind("SwapFree:", 0) == 0) {
                    stats.swap_free = parseMeminfoLine(line, "SwapFree:");
                    found_keys["SwapFree"] = true;
                }

                // Optimization: If all expected keys are found, we can stop reading the file.
                // Adjust this check if more fields are added to MemStats.
                if (found_keys["MemTotal"] && found_keys["MemFree"] && found_keys["MemAvailable"] &&
                    found_keys["Buffers"] && found_keys["Cached"] &&
                    found_keys["SwapTotal"] && found_keys["SwapFree"]) {
                    break;
                }
            } catch (const std::runtime_error& e) {
                std::cerr << "Warning: Skipping malformed meminfo line: " << e.what() << std::endl;
            }
        }

        // Basic validation: ensure essential fields were read
        if (!(found_keys["MemTotal"] && found_keys["MemFree"] && found_keys["MemAvailable"])) {
             throw std::runtime_error("Failed to find essential memory statistics in /proc/meminfo.");
        }

        return stats;
    }

} // namespace SystemMemoryStats