/**
 * Build configuration, compiler, platform and architecture detection.
 */
#pragma once


// Configuration
// -----------------------------------------------------------------------------

/// Name of the project being built.
#ifndef PROJECT_NAME
# define PROJECT_NAME "Jank"
#endif

// TODO: whats the best way to detect this?
/// Minimal optimizations with full runtime assertions.
#ifndef BUILD_DEBUG
# if defined(_DEBUG) || !defined(NDEBUG)
#  define BUILD_DEBUG 1
# else
#  define BUILD_DEBUG 0
# endif
#endif

/// Full optimizations with partial runtime assertions.
#ifndef BUILD_DEVELOPMENT
# define BUILD_DEVELOPMENT BUILD_DEBUG
#endif

/// Editor support is enabled.
#ifndef BUILD_EDITOR
# define BUILD_EDITOR 0
#endif

/// Runtime profiling is enabled.
#ifndef BUILD_PROFILE
# define BUILD_PROFILE 0
#endif


// Compiler
// -----------------------------------------------------------------------------

// NOTE: Clang and Intel compilers define __GNUC__ and must be detected first.
#if defined(__clang__)
# define COMPILER_CLANG 1
#elif defined(__ICC)
# define COMPILER_INTEL 1
#elif defined(__GNUC__)
# define COMPILER_GCC 1
#elif defined(_MSC_VER)
# define COMPILER_MSVC 1
#elif defined(__EMSCRIPTEN__)
# define COMPILER_EMSCRIPTEN 1
#else
# error Unsupported compiler
#endif

#define COMPILER_GNU_COMPATIBLE (COMPILER_CLANG || COMPILER_INTEL || COMPILER_GCC)

#ifndef COMPILER_CLANG
# define COMPILER_CLANG 0
#endif

#ifndef COMPILER_INTEL
# define COMPILER_INTEL 0
#endif

#ifndef COMPILER_GCC
# define COMPILER_GCC 0
#endif

#ifndef COMPILER_MSVC
# define COMPILER_MSVC 0
#endif

#ifndef COMPILER_EMSCRIPTEN
# define COMPILER_EMSCRIPTEN 0
#endif

#if COMPILER_MSVC && defined(_MSVC_LANG)
# define LANGUAGE_VERSION _MSVC_LANG
#elif defined(__cplusplus)
# define LANGUAGE_VERSION __cplusplus
#else
# define LANGUAGE_VERSION 0L
#endif

#if LANGUAGE_VERSION < 201703L
# error A C++17 compiler is required.
#endif

// TODO doesnt work even with switch present?
//#if COMPILER_MSVC && defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
//# error "/experimental:preprocessor" is required for MSVC
//#endif

// Platform
// -----------------------------------------------------------------------------

#if defined(__APPLE__)
# include <TargetConditionals.h>
# define PLATFORM_APPLE 1
# if TARGET_OS_IPHONE
#  define PLATFORM_IPHONE 1
#  if TARGET_OS_IOS
#   define PLATFORM_IOS 1
#  elif TARGET_OS_TV
#   define PLATFORM_TVOS 1
#  elif TARGET_OS_WATCH
#   define PLATFORM_WATCHOS 1
#  else
#   error Unsupported iPhone platform
#  endif
# elif TARGET_OS_MAC
#  define PLATFORM_MACOS 1
# else
#  error Unsupported Apple platform
# endif
#elif defined(__linux__)
# if defined(__ANDROID__)
#  define PLATFORM_ANDROID 1
# else
#  define PLATFORM_LINUX 1
# endif
#elif defined(_WIN32)
# define PLATFORM_WINDOWS 1
#elif defined(__EMSCRIPTEN__)
# define PLATFORM_HTML5 1
#else
# error Unsupported platform
#endif

/// Platform implementing the POSIX standard.
#define PLATFORM_POSIX (!PLATFORM_WINDOWS && !PLATFORM_HTML5)

/// Platform is one of macOS, iOS, tvOS or watchOS.
#ifndef PLATFORM_APPLE
# define PLATFORM_APPLE 0
#endif

/// Platform is macOS. Uses AppKit.
#ifndef PLATFORM_MACOS
# define PLATFORM_MACOS 0
#endif

/// Platform is one of iOS, tvOS or watchOS. Uses UIKit.
#ifndef PLATFORM_IPHONE
# define PLATFORM_IPHONE 0
#endif

/// Platform is iOS running iPhone and iPad.
#ifndef PLATFORM_IOS
# define PLATFORM_IOS 0
#endif

/// Platform is tvOS running AppleTV.
#ifndef PLATFORM_TVOS
# define PLATFORM_TVOS 0
#endif

/// Platform is watchOS running Apple Watch.
#ifndef PLATFORM_WATCHOS
# define PLATFORM_WATCHOS 0
#endif

/// Platform uses the Android Native Development Kit.
#ifndef PLATFORM_ANDROID
# define PLATFORM_ANDROID 0
#endif

/// Platform is one of the many Linux flavors.
#ifndef PLATFORM_LINUX
# define PLATFORM_LINUX 0
#endif

/// Platform is Windows.
#ifndef PLATFORM_WINDOWS
# define PLATFORM_WINDOWS 0
#endif

/// Platform is the Web through Emscripten. Uses either asm.js or WebAssembly.
#ifndef PLATFORM_HTML5
# define PLATFORM_HTML5 0
#endif


// Architecture
// -----------------------------------------------------------------------------

#if defined(__i386) || defined(_M_IX86)
# define ARCH_X86 1
#elif defined(__x86_64__) || defined(_M_AMD64)
# define ARCH_X64 1
#elif defined(__arm__) || defined(_M_ARM)
# define ARCH_ARM 1
#elif defined(__aarch64__)
# define ARCH_ARM64 1
#elif defined(__asmjs__)
# define ARCH_ASMJS 1
#elif defined(__wasm__)
# define ARCH_WASM 1
#else
# error Unsupported architecture
#endif

#ifndef ARCH_X86
# define ARCH_X86 0
#endif

#ifndef ARCH_X64
# define ARCH_X64 0
#endif

#ifndef ARCH_ARM
# define ARCH_ARM 0
#endif

#ifndef ARCH_ARM64
# define ARCH_ARM64 0
#endif

#ifndef ARCH_ASMJS
# define ARCH_ASMJS 0
#endif

#ifndef ARCH_WASM
# define ARCH_WASM 0
#endif


// Attributes
// -----------------------------------------------------------------------------

#define FALLTHROUGH  [[fallthrough]]
#define UNUSED       [[maybe_unused]]
#define NO_DISCARD   [[nodiscard]]

#if COMPILER_GNU_COMPATIBLE
# define UNREACHABLE __builtin_unreachable()
#elif COMPILER_MSVC
# define UNREACHABLE __assume(0)
#else
# define UNREACHABLE
#endif

#if LANGUAGE_VERSION >= 201803L
# define LIKELY(x)   (x) [[likely]]
# define UNLIKELY(x) (x) [[unlikely]]
#elif COMPILER_GNU_COMPATIBLE
# define LIKELY(x)   (__builtin_expect(!!(x), 1))
# define UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
# define LIKELY(x)   (x)
# define UNLIKELY(x) (x)
#endif

#if COMPILER_GNU_COMPATIBLE
# define PREFETCH(x, hint) __builtin_prefetch(x, 0, hint)
#elif COMPILER_MSVC
# define PREFETCH(x, hint) _mm_prefetch(x, hint)
#else
# define PREFETCH(x, hint)
#endif

#if COMPILER_GNU_COMPATIBLE
# define PRINTF_FORMAT(fmt, arg) __attribute((format(printf, fmt, arg)))
#else
# define PRINTF_FORMAT(fmt, arg)
#endif

#if COMPILER_MSVC
# define PRINTF_STR _Printf_format_string_
#else
# define PRINTF_STR
#endif


// Misc
// -----------------------------------------------------------------------------

// Request the extended standard library available since C11.
#define __STDC_WANT_LIB_EXT1__ 1

#if PLATFORM_WINDOWS
// Windows is natively UTF-16. Using its Unicode APIs prevents the expensive
// string conversions performed by the ANSI variants.
# ifndef UNICODE
#  error Windows builds must use Unicode
# endif
// Greatly limits the headers pulled in when including <Windows.h>.
# ifndef WIN32_MEAN_AND_LEAN
#  define WIN32_MEAN_AND_LEAN
# endif
# ifndef NOMINMAX
#  define NOMINMAX
# endif
#endif

#if COMPILER_CLANG
# pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
# pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
# pragma clang diagnostic ignored "-Wnested-anon-types"
#endif

