#include "core/CoreDebug.hpp"
#include "core/CoreString.hpp"

#if PLATFORM_WINDOWS
# include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#include <algorithm>


// User Messages
// ----------------------------------------------------------------------------

void errorMessageBox(uchar const* title, uchar const* msg) {
#if PLATFORM_APPLE
  // FIXME: delegate to AppleWindow, needs Objective-C
  fprintf(stderr, "%s: %s\n", title, msg);
#elif PLATFORM_LINUX
  // FIXME: delegate to LinuxWindow, reuse API detection
  fprintf(stderr, "%s: %s\n", title, msg);
#elif PLATFORM_WINDOWS
  okWin(MessageBoxW(nullptr, msg, title, MB_OK | MB_ICONERROR));
#elif PLATFORM_ANDROID
  // FIXME: delegate to AndroidApp, needs Java
  LOG(Debug, Error, "%s: %s", title, msg);
#else
# error "Missing errorMessageBox implementation"
#endif
}


// Assertions
// ----------------------------------------------------------------------------

// TODO: may not want to always abort() on assert failure

#if BUILD_DEVELOPMENT
# include <stdarg.h>

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


// Error Checking
// ----------------------------------------------------------------------------

static void fatalError(uchar const* msg, u32 length) {
#if PLATFORM_WINDOWS && BUILD_DEBUG
  OutputDebugStringW(msg);
#endif
  LOG(Debug, Fatal, UFMT, length, msg);
  BREAKPOINT();
  abort();
}

static u32 genericError(uchar* buf, usize bufLength) {
  static constexpr UStringView msg = L"Failed to format the error"sv;
  auto len{ std::min(bufLength - 1, msg.size()) + 1 };
  memcpy(buf, msg.data(), msg.size() * sizeof(uchar));
  return len - 1;
}

u32 getErrorC(uchar* buf, usize bufLength, errno_t code) {
#if PLATFORM_WINDOWS
  auto ret{ _wcserror_s(buf, bufLength, code) };
  if (ret == 0) return wcslen(buf);
#else
  // FIXME: move to a file where it can be reused!
  static locale_t locale;
  if (locale == nullptr) {
    locale = newlocale(LC_CTYPE_MASK |
                       LC_NUMERIC_MASK |
                       LC_TIME_MASK |
                       LC_COLLATE_MASK |
                       LC_MONETARY_MASK |
                       LC_MESSAGES_MASK,
                       "", reinterpret_cast<locale_t>(nullptr));
    ASSERT(locale);
  }
  auto str{ strerror_l(code, locale) };
  if (str) {
    auto len{ std::min(bufLength - 1, strlen(str)) + 1 };
    memcpy(buf, str, len);
    return len - 1;
  }
#endif

  return genericError(buf, bufLength);
}

void failC() { failC(errno); }
void failC(errno_t code) {
  uchar buf[1024];
  auto len{ getErrorC(buf, countof(buf), code) };
  fatalError(buf, len);
}

#if PLATFORM_WINDOWS
u32 getErrorCom(uchar* buf, usize bufLength, HRESULT hr) {
  auto len{ FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                           nullptr, static_cast<DWORD>(hr), 0,
                           buf, static_cast<u32>(bufLength), nullptr) };
  if (len != 0) return len;

#if BUILD_DEVELOPMENT
  auto ret{ swprintf_s(buf, bufLength, L"Error 0x%08x formatting HRESULT 0x%08x",
                       static_cast<u32>(GetLastError()), hr) };
  if (ret > 0) return static_cast<u32>(ret);
#endif

  return genericError(buf, bufLength);
}

void failWin() { failWin(GetLastError()); }
void failWin(DWORD code) { failCom(HRESULT_FROM_WIN32(code)); }

void failCom(HRESULT hr) {
  uchar buf[1024];
  auto len{ getErrorCom(buf, countof(buf), hr) };
  fatalError(buf, len);
}
#elif PLATFORM_APPLE
void failKern(kern_return_t code) {
  // TODO
  abort();
}
#endif
