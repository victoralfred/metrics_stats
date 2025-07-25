#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <iostream>
#include <mutex> // For thread safety
#include <chrono> // For timestamps
#include <iomanip> // For put_time

namespace SystemMetricsLogger {

// Define different log levels
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

// Simple Logger class
class Logger {
public:
    // Sets the minimum log level to be displayed. Messages below this level will be ignored.
    static void setLogLevel(LogLevel level);

    // Logs a message at the DEBUG level.
    static void debug(const std::string& message);

    // Logs a message at the INFO level.
    static void info(const std::string& message);

    // Logs a message at the WARNING level.
    static void warning(const std::string& message);

    // Logs a message at the ERROR level.
    static void error(const std::string& message);

    // Logs a message at the FATAL level (and may terminate the program).
    static void fatal(const std::string& message);

private:
    // Private constructor to prevent instantiation (Logger is a static class).
    Logger() = delete;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static LogLevel s_currentLogLevel; // The minimum level to log
    static std::ostream& s_outputStream; // The stream to write to (e.g., std::cout, std::cerr)
    static std::mutex s_logMutex; // Mutex for thread-safe logging

    // Internal helper to get string representation of LogLevel
    static std::string getLogLevelString(LogLevel level);

    // Internal helper to format and print the log message
    static void log(LogLevel level, const std::string& message);
};

} // namespace SystemMetricsLogger

#endif // LOGGER_HPP