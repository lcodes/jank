#pragma once

#include "CoreTypes.hpp"

/// Logging levels. Used for both compile-time and runtime filtering of log statements.
/// File and line information is included with the Trace and Debug levels.
/// The Assert and Fatal levels will automatically break into the debugger.
enum class LogLevel {
  Trace,  /// Verbose information, traces execution. Usually used for troubleshooting.
  Debug,  /// Diagnostic information, outputs internal values. Development only.
  Info,   /// General information, communicates state changes in the system.
  Warn,   /// Recovered errors, The operation completed but fell back to default behaviours..
  Error,  /// Unrecovered errors. The operation could not be completed.
  Assert, /// A contract could not be satisfied, may terminate the application. Development only.
  Fatal   /// About to crash, will terminate the application to prevent undefined behaviour.
};

/// Logging source. Used for compile-time filtering of log statements. Declare using LOG_SOURCE().
struct LogSource {
  char const* name;
  LogLevel minLevel;
};

// TODO use libfmt?
/// Logging sink.
void log(char const* source, LogLevel level, char const* fmt, ...);

#if BUILD_DEVELOPMENT
void log(char const* file, u32 line, char const* source, LogLevel level, char const* fmt, ...);
#endif

/// Expands into the top-level identifier for the given log source name.
/// Used when directly calling the log() functions. Prefer using LOG() instead.
#define LOG_SOURCE(name) ::logSource##name

/// Implements a new logging source.
/// On Android the source name is prefixed with PROJECT_NAME and a dot to become the log tag.
#if PLATFORM_ANDROID
# define DECL_LOG_SOURCE(name, minLevel) \
constexpr LogSource logSource##name{PROJECT_NAME "." #name, LogLevel::minLevel}
#else
# define DECL_LOG_SOURCE(name, minLevel) \
constexpr LogSource logSource##name{#name, LogLevel::minLevel}
#endif

// TODO: disable Debug/Assert levels in !BUILD_DEVELOPMENT
/// Writes an entry into the log.
#define LOG(source, level, fmt, ...) \
  SAFE_EXPR(if constexpr (LOG_SOURCE(source).name != nullptr && LOG_SOURCE(source).minLevel <= LogLevel::level) { \
    if constexpr (LogLevel::level == LogLevel::Assert || LogLevel::level == LogLevel::Debug) { \
      log(__FILE__, __LINE__, LOG_SOURCE(source).name, LogLevel::level, fmt, ##__VA_ARGS__); \
    } \
    else { \
      log(LOG_SOURCE(source).name, LogLevel::level, fmt, ##__VA_ARGS__); \
    } \
  })
