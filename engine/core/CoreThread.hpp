#pragma once

#include "core/CoreDebug.hpp"

#include <memory>
#include <tuple>

#if ARCH_X86 || ARCH_X64
# define SPIN_LOOP_HINT() _mm_pause()
#elif ARCH_ARM || ARCH_ARM64
# define SPIN_LOOP_HINT() __yield()
#else
# define SPIN_LOOP_HINT()
#endif

// TODO: fallbacks for HTML5 (compile without pthread, async thread creation otherwise)

/**
 * Inspired from std::thread, with support to specify the stack size, priority and affinity.
 */
class Thread {
public:
  // NOTE: matches the THREAD_PRIORITY_* defines on Windows.
  enum class Priority {
    Lowest  = -2,
    Low     = -1,
    Normal  =  0,
    High    =  1,
    Highest =  2
  };

  static constexpr u64 defaultAffinityMask = 0xFFFF'FFFF'FFFF'FFFF;

  struct Params {
    u32      stackSize    = 0;
    Priority priority     = Priority::Normal;
    u64      affinityMask = defaultAffinityMask;
  };

  template<typename F, typename... Args>
  Thread(Params const& params, F f, Args... args) {
    using T = std::tuple<std::decay_t<F>, std::decay_t<Args>...>;
    auto  t = std::make_unique<T>(std::forward<F>(f), std::forward<Args>(args)...);
    handle = create(params, getMain<T>(std::make_index_sequence<sizeof...(Args) + 1>{}), t.get());
    t.release();
  }

  Thread() : handle(nullptr) {}
  ~Thread() { join(); }

  Thread(Thread const&) = delete;
  void operator=(Thread const&) = delete;

  Thread(Thread&& t) noexcept : handle(t.handle) {
    t.handle = nullptr;
  }
  void operator=(Thread&& t) noexcept {
    ASSERT(!joinable(), "Cannot move into a running thread.");
    handle = t.handle;
    t.handle = nullptr;
  }

  bool joinable() const { return handle != nullptr; }
  void join();

  static void setName(PRINTF_STR uchar const* fmt, ...) PRINTF_FMT(1, 2);
  static void setAffinity(u64 mask);
  static void setPriority(Priority priority);

  static u32 getNumCPUs();

private:
  template<typename T, usize... ArgIndices>
  static
#if PLATFORM_WINDOWS
  u32 __stdcall
#else
  void*
#endif
  main(void* arg) noexcept {
    T entry;
    {
      std::unique_ptr<T> ptr(static_cast<T*>(arg));
      entry = *ptr;
    }
    std::invoke(std::move(std::get<ArgIndices>(entry))...);
    return 0;
  }

  template<typename T, size_t... ArgIndices>
  [[nodiscard]]
  static constexpr auto getMain(std::index_sequence<ArgIndices...>) noexcept {
    return &main<T, ArgIndices...>;
  }

#if PLATFORM_WINDOWS
  using Entry = u32 __stdcall (void*) noexcept;
#else
  using Entry = void* (void*) noexcept;
#endif

  static void* create(Params const& params, Entry entry, void* arg);

  void* handle;
};
