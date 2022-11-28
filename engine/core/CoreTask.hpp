#pragma once

#include "core/Core.hpp"

#include <atomic>
#include <functional>

namespace App {
class Main;
}

class Task {
  Task() = delete;

  static void ioMain(u8 id) noexcept;

#if PLATFORM_WINDOWS
  static void __stdcall fiberMain(void*) noexcept;
#else
# error TODO
#endif

public:
  using Fn = std::function<void()>;

  class IOCounter : NonCopyable {
    friend class Task;

    std::atomic<u32> count{ 0 };

  public:
    bool done() const { return count.load(std::memory_order_relaxed) == 0; }
  };

  class Counter : NonCopyable {
    friend class Task;

    std::atomic<void*> waitFiber{ nullptr };
    std::atomic<void*> waitEvent{ nullptr };
    std::atomic<u32>   waitFiberState{ 0 };
    std::atomic<u32>   count{ 0 };

    u32 get() const { return count.load(std::memory_order_relaxed); }

  public:
    bool done() const { return get() == 0; }

    void reset() {
      ASSERT_EX(get() == 0, "Cannot reset a counter with active jobs");
      waitFiberState.store(0, std::memory_order_relaxed);
      waitFiber.store(nullptr, std::memory_order_relaxed);
      waitEvent.store(nullptr, std::memory_order_relaxed);
    }
  };

  class Job {
    friend class Task;
    friend void Task::fiberMain(void*) noexcept;

  public:
    Job() = default;
    Job(Fn&& fn) : fn(std::move(fn)) {}
    
    Fn fn;

  private:
    Task::Counter* counter{ nullptr };
  };

  static void run(Fn&& job);
  static void run(IOCounter& counter, Fn&& job);
  static void run(Counter& counter, Job& job);
  static void run(Counter& counter, Job* jobs, u32 count);
  static void wait(Counter& counter);
  static void waitAsync(Counter& counter);

private:
  static void init();
  static void term();

  friend class App::Main;
};
