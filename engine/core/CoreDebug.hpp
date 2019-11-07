#pragma once

#include "core/CoreLog.hpp"

#if BUILD_DEBUG
DECL_LOG_SOURCE(Debug, Trace);
#elif BUILD_DEVELOPMENT
DECL_LOG_SOURCE(Debug, Debug);
#else
DECL_LOG_SOURCE(Debug, Warn);
#endif


// User Messages
// ----------------------------------------------------------------------------

void errorMessageBox(uchar const* title, uchar const* msg);


// Assertions
// ----------------------------------------------------------------------------

/// Enhanced version of the C assert macro.
#if BUILD_DEVELOPMENT
# define ASSERT(test, ...) \
  do { \
    if UNLIKELY(!(test)) { \
      assertFailure(__FILE__, __LINE__, ##__VA_ARGS__); \
    } \
  } while (0)
#else
# define ASSERT(test, ...) static_cast<void>(0)
#endif

/// Expensive assertion. Only enabled in debug builds.
#if BUILD_DEBUG
# define ASSERT_EX(...) ASSERT(__VA_ARGS__)
#else
# define ASSERT_EX(...) (static_cast<void>(0))
#endif

/// Writes the formatted assertion message and aborts.
#if BUILD_DEVELOPMENT
void assertFailure(char const* file, u32 line);
void assertFailure(char const* file, u32 line, PRINTF_STR char const* fmt, ...)
  PRINTF_FMT(3, 4);
#endif


// Error Checking
// ----------------------------------------------------------------------------

u32 getErrorC(uchar* buf, usize bufLength, errno_t code);

void failC();
void failC(errno_t code);

inline void okC(i32 result) { if UNLIKELY(result != 0) failC(); }

template<typename T>
inline T* okC(T* result) {
  if UNLIKELY(result == nullptr) failC();
  return result;
}

#if PLATFORM_WINDOWS
using BOOL    = int;
using DWORD   = unsigned long;
using HRESULT = long;

u32 getErrorCom(uchar* buf, usize bufLength, HRESULT hr);

void failWin();
void failWin(DWORD code);
void failCom(HRESULT hr);

template<typename T>
inline T okWin(T result) {
  if UNLIKELY(!result) failWin();
  return result;
}

inline void okCom(HRESULT hr) { if UNLIKELY(hr < 0) failCom(hr); }
#elif PLATFORM_APPLE
using kern_return_t = int;

void failKern(kern_return_t code);

inline void okKern(kern_return_t code) { if UNLIKELY(code != 0) failKern(code); }
#endif


// Debugging Helpers
// ----------------------------------------------------------------------------

/// Break into the debugger
#if BUILD_DEBUG
# if COMPILER_MSVC
#  define BREAKPOINT() __debugbreak()
# elif COMPILER_EMSCRIPTEN
#  include <emscripten.h>
#  define BREAKPOINT() emscripten_debugger()
# elif PLATFORM_POSIX
#  include <signal.h>
#  define BREAKPOINT() raise(SIGINT)
# else
#  define BREAKPOINT()
# endif
#else
# define BREAKPOINT()
#endif

// TODO: check if a debugger is present
