#include "CoreLog.hpp"

#include <stdarg.h>
#include <stdio.h>

#define WITH_VA(body) \
  va_list args; \
  va_start(args, fmt); \
  body; \
  va_end(args)


// Android has built-in logging
// -----------------------------------------------------------------------------

#if PLATFORM_ANDROID

#include <android/log.h>

#include <stdlib.h>

static android_LogPriority getPriority(LogLevel level) {
  switch (level) {
  case LogLevel::Trace: return ANDROID_LOG_VERBOSE;
  case LogLevel::Debug: return ANDROID_LOG_DEBUG;
  case LogLevel::Info:  return ANDROID_LOG_INFO;
  case LogLevel::Warn:  return ANDROID_LOG_WARN;
  case LogLevel::Error: return ANDROID_LOG_ERROR;
  case LogLevel::Assert:
  case LogLevel::Fatal: return ANDROID_LOG_FATAL;
  default:              UNREACHABLE;
  }
}

void log(char const* source, LogLevel level, char const* fmt, ...) {
  WITH_VA(__android_log_vprint(getPriority(level), source, fmt, args));
}

#if BUILD_DEVELOPMENT
# define FMT_FULL "%.*s\n  %s:%u"

void log(char const* file, u32 line, char const* source, LogLevel level, char const* fmt, ...) {
  WITH_VA({
    // We want to issue a single call to android's log API.
    // So we first format the user's message and then log it with the file and line.
    char buf[1024];
    auto count{vsnprintf(buf, sizeof(buf), fmt, args)};
    if (count < 0) abort(); // TODO: better handling?
    if (count < sizeof(buf)) {
      __android_log_print(getPriority(level), source, FMT_FULL, count, buf, file, line);
    }
    else {
      // Stack buffer wasn't large enough, fallback to heap memory.
      // This is expensive, especially since we already wrote to 'buf'.
      // However, this shouldn't happen very often, and only for development.
      auto mem{reinterpret_cast<char*>(malloc(count))};
      if (!mem) abort();
      count = vsnprintf(mem, count, fmt, args);
      if (count < 0) abort();
      __android_log_print(getPriority(level), source, FMT_FULL, count, mem, file, line);
      free(mem);
    }
  });
}
#endif


// Everything else
// -----------------------------------------------------------------------------

#else // PLATFORM_ANDROID

#if PLATFORM_HTML5
# define SYNCHRONIZED()
#else
# include <mutex>
# define SYNCHRONIZED() std::scoped_lock _{mutex}

static std::mutex mutex;
#endif

static FILE* getStream(LogLevel level) {
  return level >= LogLevel::Warn ? stderr : stdout;
}

static char const* prettyLevel(LogLevel level) {
  switch (level) {
  case LogLevel::Trace:  return "TRACE";
  case LogLevel::Debug:  return "DEBUG";
  case LogLevel::Info:   return "INFO";
  case LogLevel::Warn:   return "WARN";
  case LogLevel::Error:  return "ERROR";
  case LogLevel::Assert: return "ASSERT";
  case LogLevel::Fatal:  return "FATAL";
  default:               UNREACHABLE;
  }
}

void log(char const* source, LogLevel level, char const* fmt, ...) {
  auto stream{getStream(level)};
  WITH_VA({
    SYNCHRONIZED();
    fprintf(stream, "[%s] %s: ", source, prettyLevel(level));
    vfprintf(stream, fmt, args);
    putc('\n', stream);
  });
}

#if BUILD_DEVELOPMENT
void log(char const* file, u32 line, char const* source, LogLevel level, char const* fmt, ...) {
  auto stream{getStream(level)};
  WITH_VA({
    SYNCHRONIZED();
    fprintf(stream, "[%s] %s: ", source, prettyLevel(level));
    vfprintf(stream, fmt, args);
    fprintf(stream, "\n  %s:%u\n", file, line);
  });
}
#endif

#endif // !PLATFORM_ANDROID
