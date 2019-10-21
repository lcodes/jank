// TODO: use libfmt?
// TODO: completely disable Debug/Assert levels in !BUILD_DEVELOPMENT
// TODO: more compile-time log elision

#pragma once

#include "CoreTypes.hpp"

/// Logging levels. Used for both compile-time and runtime filtering of log statements.
/// File and line information is included with the Trace and Debug levels.
/// The Assert and Fatal levels will automatically break into the debugger.
enum class LogLevel : u8 {
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
  u8          nameLen;
  LogLevel    minLevel;
};

/// Logging sink.
void log(char const* source, u32 sourceLen,
         LogLevel level, PRINTF_STR char const* fmt, ...)
  PRINTF_FMT(4, 5);

#if BUILD_DEVELOPMENT
void log(char const* file, u32 line,
         char const* source, u32 sourceLen,
         LogLevel level, PRINTF_STR char const* fmt, ...)
  PRINTF_FMT(6, 7);
#endif

/// Expands into the top-level identifier for the given log source name.
/// Used when directly calling the log() functions. Prefer using LOG() instead.
#define LOG_SOURCE(sourceName) ::logSource##sourceName
#define LOG_SOURCE_ARGS(sourceName) \
  LOG_SOURCE(sourceName).name,      \
  LOG_SOURCE(sourceName).nameLen

/// Implements a new logging source.
/// On Android the source name is prefixed with PROJECT_NAME and a dot to become the log tag.
#if PLATFORM_ANDROID
# define DECL_LOG_SOURCE(name, minLevel) \
  constexpr LogSource logSource##name{   \
    PROJECT_NAME "." #name,              \
    sizeof(PROJECT_NAME "." #name) - 1,  \
    LogLevel::minLevel                   \
  }
#else
# define DECL_LOG_SOURCE(name, minLevel)         \
  constexpr LogSource logSource##name{           \
    #name, sizeof(#name) - 1, LogLevel::minLevel \
  }
#endif

/// HACK: work around MSVC intellisense not handling ##__VA_ARGS__ inside nested macros
#define LOG_BEGIN(source, level, test)                                                                   \
  do {                                                                                                   \
   if constexpr (LOG_SOURCE(source).name != nullptr && LOG_SOURCE(source).minLevel <= LogLevel::level) { \
     if constexpr (test(LogLevel::level == LogLevel::Assert || LogLevel::level == LogLevel::Debug)) {

#define LOG_END() \
      }           \
    }             \
  } while (0)

/// Writes an entry into the log.
#if BUILD_DEVELOPMENT
# define LOG(source, level, fmt, ...)                                                      \
  LOG_BEGIN(source, level,)                                                                \
    log(__FILE__, __LINE__, LOG_SOURCE_ARGS(source), LogLevel::level, fmt, ##__VA_ARGS__); \
  }                                                                                        \
  else {                                                                                   \
    log(LOG_SOURCE_ARGS(source), LogLevel::level, fmt, ##__VA_ARGS__);                     \
  LOG_END()
#else
# define LOG(source, level, fmt, ...)                                  \
  LOG_BEGIN(source, level, !)                                          \
    log(LOG_SOURCE_ARGS(source), LogLevel::level, fmt, ##__VA_ARGS__); \
  LOG_END()
#endif
