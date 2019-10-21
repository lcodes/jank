#include "core/CoreLog.hpp"
#include "core/CoreDebug.hpp"

#include <stdarg.h>
#include <stdio.h>

#define WITH_VA(body) \
  va_list args; \
  va_start(args, fmt); \
  body; \
  va_end(args)

#define LOG_USE_HTML5_CONSOLE 0 // FIXME: why is this so slow?

#if PLATFORM_ANDROID || (PLATFORM_HTML5 && LOG_USE_HTML5_CONSOLE)
# include <stdlib.h>

template<typename LogFn>
static void safeLog(char const* fmt, va_list args, LogFn logFn) {
  char buf[1024];
  auto count{ vsnprintf(buf, sizeof(buf), fmt, args) };
  ASSERT_EX(count >= 0);

  if (count < static_cast<i32>(sizeof(buf))) {
    logFn(count, buf);
  }
  else {
    // Stack buffer wasn't large enough, fallback to heap memory.
    // This is expensive, especially since we already wrote to 'buf'.
    // However, this shouldn't happen very often.
    auto mem{ reinterpret_cast<char*>(malloc(count)) };
    if (!mem) abort();

    auto memCount{ vsnprintf(mem, count, fmt, args) };
    ASSERT_EX(memCount >= 0 && memCount == count);

    logFn(count, mem);
    free(mem);
  }
}
#endif


// Android has built-in logging
// -----------------------------------------------------------------------------

#if PLATFORM_ANDROID
# include <android/log.h>

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

void log(char const* source, u32 sourceLen UNUSED,
         LogLevel level, char const* fmt, ...)
{
  WITH_VA(__android_log_vprint(getPriority(level), source, fmt, args));
}

# if BUILD_DEVELOPMENT
void log(char const* file, u32 line,
         char const* source, u32 sourceLen UNUSED,
         LogLevel level, char const* fmt, ...)
{
  WITH_VA(safeLog(fmt, args, [=](auto len, auto msg) {
    __android_log_print(getPriority(level), source, "%.*s\n  %s:%u",
                        len, msg, file, line);
  }));
}
# endif


// HTML5 has a built-in console
// -----------------------------------------------------------------------------

#elif PLATFORM_HTML5 && LOG_USE_HTML5_CONSOLE
# include <emscripten.h>

static i32 getFlags(LogLevel level) {
  switch (level) {
  case LogLevel::Trace:
  case LogLevel::Debug:
  case LogLevel::Info:   return EM_LOG_CONSOLE;
  case LogLevel::Warn:   return EM_LOG_CONSOLE | EM_LOG_WARN;
  case LogLevel::Error:  return EM_LOG_CONSOLE | EM_LOG_ERROR;
  case LogLevel::Assert:
  case LogLevel::Fatal:  return EM_LOG_CONSOLE | EM_LOG_ERROR | EM_LOG_C_STACK;
  }
}

// TODO: emscripten_log supports printf-style formatting, but we have a va_list.
//       maybe worth making the LOG macro directly call emscripten_log?
//       low priority, logging shouldn't be done in the fast path anyways.

void log(char const* source, u32 sourceLen,
         LogLevel level, char const* fmt, ...)
{
  WITH_VA(safeLog(fmt, args, [=](auto len, auto msg) {
    emscripten_log(getFlags(level), "[%.*s] %.*s",
                   sourceLen, source, len, msg);
  }));
}

# if BUILD_DEVELOPMENT
void log(char const* file, u32 line,
         char const* source, u32 sourceLen,
         LogLevel level, char const* fmt, ...)
{
  WITH_VA(safeLog(fmt, args, [=](auto len, auto msg) {
    emscripten_log(getFlags(level),"[%.*s] %.*s\n  %s:%u",
                   sourceLen, source, len, msg, file, line);
  }));
}
# endif


// Everything else
// -----------------------------------------------------------------------------

#else // PLATFORM
# if PLATFORM_HTML5
#  define SYNCHRONIZED()
# else
#  include <mutex>
#  define SYNCHRONIZED() std::scoped_lock _{ mutex }

static std::mutex mutex;
# endif

# include <string>

using namespace std::literals;

static FILE* getStream(LogLevel level) {
  return level >= LogLevel::Warn ? stderr : stdout;
}

static std::string_view prettyLevel(LogLevel level) {
  switch (level) {
  case LogLevel::Trace:  return "TRACE"sv;
  case LogLevel::Debug:  return "DEBUG"sv;
  case LogLevel::Info:   return "INFO"sv;
  case LogLevel::Warn:   return "WARN"sv;
  case LogLevel::Error:  return "ERROR"sv;
  case LogLevel::Assert: return "ASSERT"sv;
  case LogLevel::Fatal:  return "FATAL"sv;
  default:               UNREACHABLE;
  }
}

# define SOURCE sourceLen, source
# define LEVEL  static_cast<u32>(pretty.size()), pretty.data()

void log(char const* source, u32 sourceLen,
         LogLevel level, PRINTF_STR char const* fmt, ...)
{
  auto stream{ getStream(level) };
  auto pretty{ prettyLevel(level) };
  WITH_VA({
    SYNCHRONIZED();
    fprintf(stream, "[%.*s] %.*s: ", SOURCE, LEVEL);
    vfprintf(stream, fmt, args);
    putc('\n', stream);
  });
}

# if BUILD_DEVELOPMENT
void log(char const* file, u32 line,
         char const* source, u32 sourceLen,
         LogLevel level, PRINTF_STR char const* fmt, ...)
{
  auto stream{ getStream(level) };
  auto pretty{ prettyLevel(level) };
  WITH_VA({
    SYNCHRONIZED();
    fprintf(stream, "[%.*s] %.*s: ", SOURCE, LEVEL);
    vfprintf(stream, fmt, args);
    fprintf(stream, "\n  %s:%u\n", file, line);
  });
}
# endif
#endif // PLATFORM
