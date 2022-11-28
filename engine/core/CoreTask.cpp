#include "core/CoreTask.hpp"
#include "core/CoreThread.hpp"

#include <blockingconcurrentqueue.h>

#include <mutex>

#if PLATFORM_WINDOWS
# pragma warning(push, 0)
# include <Windows.h>
# pragma warning(pop)
#else
# error "TODO non-windows fibers"
#endif

enum class State : u8 {
  Idle,
  Waiting
};

struct IO {
  Task::Fn         fn;
  Task::IOCounter* counter;
};

struct alignas(cacheLineSize) Fiber {
  void* handle;
};

struct alignas(cacheLineSize) Context {
  void* main;
  void* current;
  void* previous;
  std::atomic<u32>* previousSync;
  State previousState;
};

constexpr i32 numFibers{ 128 };

static thread_local u8 contextIndex = UINT8_MAX;

static Thread*  ioThreads;
static Thread*  jobThreads;
static Context* contexts;
static Fiber*   fibers;

// TODO: rework this to be lock-free
static std::mutex       waitFiberMutex;
static std::mutex       idleFiberMutex;
static std::atomic<i16> idleFiberTop;

// TODO: look into work-stealing and non-blocking queues
static moodycamel::BlockingConcurrentQueue<IO>         ioQueue;
static moodycamel::BlockingConcurrentQueue<Task::Job*> jobQueue;

#if PLATFORM_WINDOWS
# define switchToFiber SwitchToFiber
# define FIBER_API     __stdcall
#else
# error TODO
#endif

void Task::ioMain(u8 id) noexcept {
  Thread::setName(USTR("I/O #%u"), id + 1);

  IO io;
  while (true) {
    ioQueue.wait_dequeue(io);
    if (!io.fn) {
      break;
    }

    io.fn();

    if (io.counter) {
      io.counter->count.fetch_sub(1, std::memory_order_relaxed);
    }
  }
}

static void* getIdleFiber() {
  std::scoped_lock _{ idleFiberMutex };
  auto index{ --idleFiberTop };
  ASSERT(index >= 0, "Fiber underflow");
  return fibers[index].handle;
}

static void freeFiber(void* fiber) {
  std::scoped_lock _{ idleFiberMutex };
  ASSERT(idleFiberTop < numFibers, "Fiber overflow");
  fibers[idleFiberTop++].handle = fiber;
}

static void jobMain(u8 id) noexcept {
  Thread::setName(USTR("Job #%u"), id + 1);

  auto& ctx{ contexts[id] };

#if PLATFORM_WINDOWS
  ctx.main = { okWin(ConvertThreadToFiber(nullptr)) };
#else
# error TODO
#endif

  ctx.current       = { getIdleFiber() };
  ctx.previous      = { nullptr };
  ctx.previousState = { State::Idle };

  contextIndex = { id };
  switchToFiber(ctx.current);

  ASSERT(&ctx == &contexts[contextIndex]);
  freeFiber(ctx.current);

#if PLATFORM_WINDOWS
  okWin(ConvertFiberToThread());
#else
# error TODO
#endif
}

static void cleanup() {
  auto& ctx{ contexts[contextIndex] };
  if (ctx.previousState == State::Waiting) {
    auto sync{ ctx.previousSync };
    ASSERT(sync);
    sync->store(2, std::memory_order_relaxed);
  }
}

void FIBER_API Task::fiberMain(void*) noexcept {
  cleanup();

  Task::Job* job;
  while (true) {
  loop:
    jobQueue.wait_dequeue(job);

    if (job == nullptr) {
      switchToFiber(contexts[contextIndex].main);
    }

    ASSERT_EX(job->fn, "Job is empty");
    job->fn();

    // TODO: are those the right memory orderings?
    auto newCount{ job->counter->count.fetch_sub(1, std::memory_order_seq_cst) - 1 };
    if (newCount == 0) {
      // TODO merge wait fiber/event into one handle
      if (auto state{ job->counter->waitFiberState.load(std::memory_order_relaxed) }) {
        while (state != 2) {
          state = { job->counter->waitFiberState.load(std::memory_order_relaxed) };
          if (state == 0) goto loop;
          SPIN_LOOP_HINT();
        }

        {
          auto& ctx{ contexts[contextIndex] };
          ctx.previousState = { State::Idle };
          ctx.previous = { ctx.current };
          ctx.current = { job->counter->waitFiber.load(std::memory_order_relaxed) };
          switchToFiber(ctx.current);
        }

        cleanup();
      }
      else if (auto waitEvent{ job->counter->waitEvent.load(std::memory_order_seq_cst) }) {
        // TODO not windows only
        okWin(SetEvent(waitEvent));
      }
    }
  }

  ASSERT(0); UNREACHABLE;
}

void Task::run(Fn&& fn) {
  ASSERT(fn, "Cannot submit empty I/O task");
  auto ret{ ioQueue.enqueue({ std::move(fn), nullptr }) };
  ASSERT_EX(ret, "Failed to enqueue I/O task");
}

void Task::run(IOCounter& counter, Fn&& fn) {
  ASSERT(fn, "Cannot submit empty I/O task");
  auto ret{ ioQueue.enqueue({ std::move(fn), &counter }) };
  ASSERT_EX(ret, "Failed to enqueue I/O task");
}

void Task::run(Counter& counter, Job& job) {
  ASSERT_EX(counter.get() == 0, "Job counter is not empty");
  counter.count.store(1, std::memory_order_relaxed);

  job.counter = &counter;
  auto ret{ jobQueue.enqueue(&job) };
  ASSERT_EX(ret, "Failed to enqueue job");
}

void Task::run(Counter& counter, Job* jobs, u32 count) {
  ASSERT_EX(counter.get() == 0);
  counter.count.store(count, std::memory_order_relaxed);

  for (auto n{ 0u }; n < count; n++) {
    jobs[n].counter = &counter;
    auto ret{ jobQueue.enqueue(&jobs[n]) };
    ASSERT_EX(ret, "Failed to enqueue job");
  }
}

void Task::wait(Counter& counter) {
  ASSERT_EX(contextIndex != UINT8_MAX, "Task::wait called outside a job worker");
  if (counter.get() == 0) return;

  auto expected{ 0u };
  auto success{ counter.waitFiberState.compare_exchange_strong(expected, 1, std::memory_order_relaxed) };
  ASSERT(success, "Multiple fibers waiting on counter");

  if (counter.get() == 0) {
    // FIXME: probably race condition if caller destroys counter before job fiber reads it
    counter.waitFiberState.store(0, std::memory_order_relaxed);
    return;
  }

  auto& ctx{ contexts[contextIndex] };
  ctx.previousState = { State::Waiting };
  ctx.previous      = { ctx.current };
  ctx.current       = { getIdleFiber() };
  ctx.previousSync  = &counter.waitFiberState;

  counter.waitFiber.store(ctx.previous, std::memory_order_seq_cst);
  switchToFiber(ctx.current);

  ASSERT(contexts[contextIndex].previousState == State::Idle);
  freeFiber(contexts[contextIndex].previous);
}

void Task::waitAsync(Counter& counter) {
  ASSERT_EX(contextIndex == UINT8_MAX, "Task::waitAsync called on a job worker");
  if (counter.get() == 0) return;
  
  counter.waitEvent.store(okWin(CreateEventW(nullptr, false, false, nullptr)),
                          std::memory_order_seq_cst);

  if (counter.get() != 0) {
    okWin(WaitForSingleObject(counter.waitEvent, INFINITE) == WAIT_OBJECT_0);
  }

  okWin(CloseHandle(counter.waitEvent));
  counter.waitEvent.store(nullptr, std::memory_order_relaxed);
}

void Task::init() {
  auto numJobWorkers{ Thread::getNumCPUs() };
  auto numIOWorkers { numJobWorkers / 2 };

  ioThreads  = { new Thread[numIOWorkers] };
  jobThreads = { new Thread[numJobWorkers] };
  contexts   = { new Context[numJobWorkers] };
  fibers     = { new Fiber  [numFibers] };

  idleFiberTop.store(numFibers, std::memory_order_release);

  for (auto n{ 0u }; n < numFibers; n++) {
  #if PLATFORM_WINDOWS
    fibers[n].handle = { CreateFiber(0x1000, Task::fiberMain, nullptr) };
  #else
  # error TODO
  #endif
  }

  Thread::Params params{ UINT16_MAX, Thread::Priority::Low };

  for (u8 n{ 0u }; n < numIOWorkers; n++) {
    ioThreads[n] = { Thread{ params, ioMain, n } };
  }

  params.priority = { Thread::Priority::High };

  for (u8 n{ 0u }; n < numJobWorkers; n++) {
    params.affinityMask = { 1ull << n };
    jobThreads[n] = { Thread{ params, jobMain, n } };
  }
}

void Task::term() {
  auto numJobWorkers{ Thread::getNumCPUs() };
  auto numIOWorkers { numJobWorkers / 2 };

  for (auto n{ 0u }; n < numIOWorkers; n++) {
    auto ret{ ioQueue.enqueue({}) };
    ASSERT(ret, "Failed to enqueue final I/O task");
  }
  for (auto n{ 0u }; n < numIOWorkers; n++) {
    ioThreads[n].join();
  }

  for (auto n{ 0u }; n < numJobWorkers; n++) {
    auto ret{ jobQueue.enqueue(nullptr) };
    ASSERT(ret, "Failed to enqueue final job");
  }
  for (auto n{ 0u }; n < numJobWorkers; n++) {
    jobThreads[n].join();
  }

  ASSERT(jobQueue.size_approx() == 0);
  ASSERT(ioQueue .size_approx() == 0);
  ASSERT(idleFiberTop.load(std::memory_order_relaxed) == numFibers);

  for (auto n{ 0u }; n < numFibers; n++) {
  #if PLATFORM_WINDOWS
    ASSERT_EX(fibers[n].handle);
    DeleteFiber(fibers[n].handle);
  #else
  # error TODO
  #endif
  }

  delete[] fibers;
  delete[] contexts;
  delete[] jobThreads;
  delete[] ioThreads;
}
