#include "logger.hpp"
#include <ctime> // For std::time_t, std::localtime

namespace SystemMetricsLogger {

// Initialize static members
LogLevel Logger::s_currentLogLevel = LogLevel::INFO; // Default to INFO
std::ostream& Logger::s_outputStream = std::cout; // Default to stdout
std::mutex Logger::s_logMutex;

void Logger::setLogLevel(LogLevel level) {
    s_currentLogLevel = level;
}

std::string Logger::getLogLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (level < s_currentLogLevel) {
        return; // Message level is below the configured minimum, so don't log
    }

    std::lock_guard<std::mutex> lock(s_logMutex); // Ensure thread safety

    // Get current time for timestamp
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    s_outputStream << "[" << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %H:%M:%S")
                   << "] [" << getLogLevelString(level) << "] " << message << std::endl;

    if (level == LogLevel::FATAL) {
        // For fatal errors, terminate the program
        std::exit(EXIT_FAILURE);
    }
}

// Public logging methods
void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::fatal(const std::string& message) {
    log(LogLevel::FATAL, message);
}

} // namespace SystemMetricsLogger
