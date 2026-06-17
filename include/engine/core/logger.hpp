#pragma once

#include <cstdarg>
#include <cstdio>

namespace Engine::Core {

enum class LogLevel : int {
    Info  = 0,
    Warn  = 1,
    Error = 2,
};

// Writes a single log line to stdout (Info/Warn) or stderr (Error) with a
// "[Category] " prefix. Format is printf-style.
//
// Intentionally minimal — when log volume justifies it, swap the body for
// spdlog or similar without changing callers (design D6).
void LogMessage(LogLevel level, const char* category, const char* fmt, ...);

}  // namespace Engine::Core

// Convenience macros. Variadic so callers write SF_LOG_INFO("Cat", "fmt %d", x).
#define SF_LOG_INFO(category, ...)  ::Engine::Core::LogMessage(::Engine::Core::LogLevel::Info,  category, __VA_ARGS__)
#define SF_LOG_WARN(category, ...)  ::Engine::Core::LogMessage(::Engine::Core::LogLevel::Warn,  category, __VA_ARGS__)
#define SF_LOG_ERROR(category, ...) ::Engine::Core::LogMessage(::Engine::Core::LogLevel::Error, category, __VA_ARGS__)
