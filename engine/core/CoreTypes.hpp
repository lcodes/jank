#pragma once

#include "core/CoreTarget.hpp"

#include <stddef.h>
#include <stdint.h>


// Primitive Data Types
// -----------------------------------------------------------------------------

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8  = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using isize = ptrdiff_t;
using usize = size_t;

using f16 = int16_t;
using f32 = float;
using f64 = double;
using f80 = long double;


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
  NonCopyable& operator=(NonCopyable&&) = delete;
};


// Utility Macros
// -----------------------------------------------------------------------------

/// Helper macro to define complex macros able to be used within an if statement.
/// NOTE: Doesn't work under MSVC if the macro uses ##__VA_ARGS__!
#define SAFE_EXPR(x) do { x; } while (0)
