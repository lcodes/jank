#include "CoreDebug.hpp"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// TODO: trigger breakpoint
// TODO: may not want to always abort() on assert failure

void assertFailure(char const* file, u32 line) {
  log(file, line, LOG_SOURCE(Debug).name, LogLevel::Assert, "Assertion failure");
  abort();
}

#if BUILD_DEBUG
void assertFailure(char const* file, u32 line, char const* fmt, ...) {
  char buf[1024];
  va_list args;
  va_start(args, fmt);
  if (vsnprintf(buf, sizeof(buf), fmt, args) >= 0) {
    log(file, line, LOG_SOURCE(Debug).name, LogLevel::Assert, buf);
  }
  __debugbreak();
  abort();
  va_end(args);
}
#endif
