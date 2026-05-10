#include "hopf/core/Logger.h"

#include <array>
#include <cstdarg>
#include <cstdio>

namespace hopf::core {

namespace {
constexpr size_t kMaxEntries = 4096;
std::vector<LogEntry> g_entries;

const char* levelTag(LogLevel l) {
    switch (l) {
        case LogLevel::Info:    return "[info]   ";
        case LogLevel::Warning: return "[warn]   ";
        case LogLevel::Error:   return "[error]  ";
    }
    return "[?]";
}
} // namespace

void Logger::log(LogLevel level, const char* fmt, va_list args) {
    std::array<char, 1024> buf{};
    vsnprintf(buf.data(), buf.size(), fmt, args);

    std::string line = levelTag(level);
    line += buf.data();

    std::fprintf(stderr, "%s\n", line.c_str());

    if (g_entries.size() >= kMaxEntries) {
        g_entries.erase(g_entries.begin(), g_entries.begin() + kMaxEntries / 4);
    }
    g_entries.push_back({level, std::move(line)});
}

void Logger::info(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    log(LogLevel::Info, fmt, ap);
    va_end(ap);
}
void Logger::warning(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    log(LogLevel::Warning, fmt, ap);
    va_end(ap);
}
void Logger::error(const char* fmt, ...) {
    va_list ap; va_start(ap, fmt);
    log(LogLevel::Error, fmt, ap);
    va_end(ap);
}

const std::vector<LogEntry>& Logger::entries() { return g_entries; }
void Logger::clear() { g_entries.clear(); }

} // namespace hopf::core
