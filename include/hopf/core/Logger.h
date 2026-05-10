#pragma once

#include <string>
#include <vector>

namespace hopf::core {

enum class LogLevel { Info, Warning, Error };

struct LogEntry {
    LogLevel    level;
    std::string message;
};

// In-memory log feeding the editor Console panel. Not thread-safe.
class Logger {
public:
    static void info   (const char* fmt, ...);
    static void warning(const char* fmt, ...);
    static void error  (const char* fmt, ...);

    static const std::vector<LogEntry>& entries();
    static void clear();

private:
    static void log(LogLevel level, const char* fmt, va_list args);
};

} // namespace hopf::core
