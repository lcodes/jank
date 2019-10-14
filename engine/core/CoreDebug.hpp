#pragma once

#include "CoreLog.hpp"

#if BUILD_DEBUG
DECL_LOG_SOURCE(Debug, Trace);
#elif BUILD_DEVELOPMENT
DECL_LOG_SOURCE(Debug, Debug);
#else
DECL_LOG_SOURCE(Debug, Warn);
#endif

/// Enhanced version of the C assert macro.
#if BUILD_DEBUG || BUILD_DEVELOPMENT
# define ASSERT(test, ...) \
  SAFE_EXPR(if UNLIKELY(!(test)) { \
    assertFailure(__FILE__, __LINE__, ##__VA_ARGS__); \
  })
#else
# define ASSERT(test, ...) static_cast<void>(0)
#endif

/// Expensive assertion. Only enabled in debug builds.
#if BUILD_DEBUG
# define ASSERT_EX(...) assert(__VA_ARGS__)
#else
# define ASSERT_EX(...) (static_cast<void>(0))
#endif

/// Writes the formatted assertion message and aborts.
void assertFailure(char const* file, u32 line);

#if BUILD_DEBUG
void assertFailure(char const* file, u32 line, char const* fmt, ...);
#endif

// TODO: breakpoints
// TODO: check if a debugger is present
