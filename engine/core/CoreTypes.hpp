/**
 * Basic types definitions.
 */
#pragma once

#include "core/CoreTarget.hpp"

#include <stddef.h>
#include <stdint.h>


// Primitive Data Types
// -----------------------------------------------------------------------------

using i8    = int8_t;      /// 8-bit signed integer.
using i16   = int16_t;     /// 16-bit signed integer.
using i32   = int32_t;     /// 32-bit signed integer.
using i64   = int64_t;     /// 64-bit signed integer.

using u8    = uint8_t;     /// 8-bit unsigned integer.
using u16   = uint16_t;    /// 16-bit unsigned integer.
using u32   = uint32_t;    /// 32-bit unsigned integer.
using u64   = uint64_t;    /// 64-bit unsigned-integer.

using isize = ptrdiff_t;   /// Pointer-sized signed integer.
using usize = size_t;      /// Pointer-sized unsigned integer.

using f16   = int16_t;     /// 16-bit floating point number. (But really "16-bit opaque"!)
using f32   = float;       /// 32-bit floating point number.
using f64   = double;      /// 64-bit floating point number.
using f80   = long double; /// 80-bit floating point number. (But really "at least 64-bit"!)


// Native Character Types
// -----------------------------------------------------------------------------

#if PLATFORM_WINDOWS
using uchar = wchar_t;
# define UFMT "%.*S"
#else
using uchar = char;
# define UFMT "%.*s"
# define TEXT(s) s
#endif


// Basic Math Types
// -----------------------------------------------------------------------------

union Point {
  struct {
    i32 x;
    i32 y;
  };
  i32 v[2];
};

union Size {
  struct {
    u32 width;
    u32 height;
  };
  u32 v[2];
};


// Enum Types
// -----------------------------------------------------------------------------




// Utility Types
// -----------------------------------------------------------------------------

/// Disable copy and move semantics on the derived classes.
class NonCopyable {
protected:
  NonCopyable() = default;
  ~NonCopyable() = default;

public:
  NonCopyable(NonCopyable const&) = delete;
  NonCopyable(NonCopyable&&) noexcept = delete;

  NonCopyable& operator=(NonCopyable const&) = delete;
  NonCopyable& operator=(NonCopyable&&) noexcept = delete;
};

template<typename T>
class Singleton : NonCopyable {
  static T* instance;

protected:
  Singleton() {
    instance = static_cast<T*>(this);
  }
  ~Singleton() {
    instance = nullptr;
  }

public:
  static T* get() { return instance; }
};

template<typename T>
T* Singleton<T>::instance;


// Utility Macros
// -----------------------------------------------------------------------------

/// Helper macro to define complex macros able to be used within an if statement.
/// NOTE: Doesn't work under MSVC if the macro uses ##__VA_ARGS__!
#define SAFE_EXPR(x) do { x; } while (0)

/// Returns the number of elements in a static array.
#if COMPILER_MSVC
# define countof(a) _countof(a)
#else
// Reference: https://stackoverflow.com/a/4415628
# define ERROR_IF_NONZERO(e)   (sizeof(struct { int : -!!(e); }))
# define countof_checkArray(a) ERROR_IF_NONZERO(__builtin_types_compatible_p(typeof(a), typeof(&a[0])))
# define countof(a)            (sizeof(a) / sizeof((a)[0]) + countof_checkArray(a))
#endif
