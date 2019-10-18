#include "core/CoreDebug.hpp"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// TODO: may not want to always abort() on assert failure

void assertFailure(char const* file, u32 line) {
  log(file, line, LOG_SOURCE_ARGS(Debug), LogLevel::Assert, "Assertion failure");
  BREAKPOINT();
  abort();
}

#if BUILD_DEBUG
void assertFailure(char const* file, u32 line, char const* fmt, ...) {
  char buf[1024];
  va_list args;
  va_start(args, fmt);
  if (vsnprintf(buf, sizeof(buf), fmt, args) >= 0) {
    log(file, line, LOG_SOURCE_ARGS(Debug), LogLevel::Assert, buf);
  }
  va_end(args);
  BREAKPOINT();
  abort();
}
#endif
