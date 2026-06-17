#include "engine/core/logger.hpp"

namespace Engine::Core {

namespace {

const char* LevelTag(LogLevel level) {
    switch (level) {
        case LogLevel::Info:  return "info";
        case LogLevel::Warn:  return "warn";
        case LogLevel::Error: return "error";
    }
    return "?";
}

}  // namespace

void LogMessage(LogLevel level, const char* category, const char* fmt, ...) {
    std::FILE* stream = (level == LogLevel::Error) ? stderr : stdout;
    std::fprintf(stream, "[%s][%s] ", category, LevelTag(level));

    std::va_list args;
    va_start(args, fmt);
    std::vfprintf(stream, fmt, args);
    va_end(args);

    std::fputc('\n', stream);
    std::fflush(stream);
}

}  // namespace Engine::Core
