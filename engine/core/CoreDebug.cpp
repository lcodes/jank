#include "core/CoreDebug.hpp"

// TODO: may not want to always abort() on assert failure

#if BUILD_DEVELOPMENT
# include <stdarg.h>
# include <stdio.h>
# include <stdlib.h>

void assertFailure(char const* file, u32 line) {
  log(file, line, LOG_SOURCE_ARGS(Debug), LogLevel::Assert, "Assertion failure");
  BREAKPOINT();
  abort();
}

void assertFailure(char const* file, u32 line, PRINTF_STR char const* fmt, ...) {
  char buf[1024];
  va_list args;
  va_start(args, fmt);
  if (vsnprintf(buf, sizeof(buf), fmt, args) >= 0) {
    log(file, line, LOG_SOURCE_ARGS(Debug), LogLevel::Assert, buf);
  } // TODO: else ?
  va_end(args);
  BREAKPOINT();
  abort();
}
#endif
