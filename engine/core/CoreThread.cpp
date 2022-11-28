#include "core/CoreThread.hpp"

#include <stdio.h>

#if PLATFORM_WINDOWS
# pragma warning(push, 0)
# include <process.h>
# include <Windows.h>
# pragma warning(pop)
#elif PLATFORM_POSIX
# include <pthread.h>

constexpr i32 threadPolicy = SCHED_RR;

static sched_param getThreadPriority(Thread::Priority priority) {
  static i32 threadPriorityMin;
  static i32 threadPriorityMax;

  if (threadPriorityMin == 0) {
    threadPriorityMin = sched_get_priority_min(threadPolicy);
    threadPriorityMax = sched_get_priority_max(threadPolicy);
  }

  auto range{ threadPriorityMax - threadPriorityMin };
  switch (priority) {
  case Thread::Priority::Lowest:  return threadPriorityMin + 1;
  case Thread::Priority::Low:     return threadPriorityMin + range / 3;
  case Thread::Priority::Normal:  return threadPriorityMin + range / 2;
  case Thread::Priority::High:    return threadPriorityMax - range / 3;
  case Thread::Priority::Highest: return threadPriorityMax - 1;
  }
}
#endif

#if PLATFORM_APPLE
static void setThreadAffinity(mach_port_t thread, u64 mask) {
  thread_affinity_policy_data_t policy{ mask };
  okKern(thread_policy_set(thread, THREAD_AFFINITY_POLICY,
                           reinterpret_cast<thread_policy_t>(&policy),
                           THREAD_AFFINITY_POLICY_COUNT));
}
#elif PLATFORM_LINUX
static void setThreadAffinity(pthread_t thread, u64 mask) {
  cpu_set_t set;
  CPU_ZERO(&set);

  for (auto i{ 0 }; i < 64; i++) {
    if (mask & (1 << i)) {
      CPU_SET(i, &set);
    }
  }

  okC(pthread_setaffinity_np(thread, sizeof(set), &set));
}
#endif

void* Thread::create(Params const& params, Entry entry, void* arg) {
#if PLATFORM_WINDOWS
  auto result{ _beginthreadex(nullptr, params.stackSize, entry, arg, CREATE_SUSPENDED, nullptr) };
  auto handle{ okC(reinterpret_cast<HANDLE>(result)) };
  okWin(SetThreadAffinityMask(handle, params.affinityMask));
  okWin(SetThreadPriority(handle, static_cast<i32>(params.priority)));
  if (ResumeThread(handle) == static_cast<DWORD>(-1)) failWin();
#else
  pthread_t handle;
  pthread_attr_t attr;
  pthread_attr_t* pattr;

  if (params.stackSize == 0 && params.priority == Priority::Normal) {
    pattr = nullptr;
  }
  else {
    pattr = &attr;
    okC(pthread_attr_init(pattr));

    if (params.stackSize != 0) {
      okC(pthread_attr_setstacksize(pattr, params.stackSize));
    }

    if (params.priority != Priority::Normal) {
      auto param{ getThreadPriority(params.priority) };
      okC(pthread_attr_setschedparam(pattr, &param));
      okC(pthread_attr_setschedpolicy(pattr, threadPolicy));
    }
  }

# if PLATFORM_APPLE
  okC(pthread_create_suspended_np(&handle, pattr, entry, arg));
  auto machThread{ pthread_mach_thread_np(handle) };
  if (param.affinityMask != defaultAffinityMask) {
    setThreadAffinity(machThread, param.affinityMask);
  }
  okC(thread_resume(machThread));
# elif PLATFORM_LINUX
  okC(pthread_create(&handle, pattr, entry, arg));
  if (param.affinityMask != defaultAffinityMask) {
    setThreadAffinity(handle, param.affinityMask);
  }
# else
#  error "Missing Thread::create implementation"
# endif

  if (pattr) {
    okC(pthread_attr_destroy(pattr));
  }
#endif

  return handle;
}

void Thread::join() {
  if (joinable()) {
  #if PLATFORM_WINDOWS
    auto ret{ WaitForSingleObject(handle, INFINITE) };
    if (ret != WAIT_OBJECT_0) failWin();
  #else
    okC(pthread_join(&handle, nullptr));
  #endif

    handle = nullptr;
  }
}

void Thread::setName(PRINTF_STR uchar const* fmt, ...) {
#if BUILD_DEVELOPMENT
  uchar name[16];
  va_list args;
  va_start(args, fmt);

# if PLATFORM_WINDOWS
  auto len{ vswprintf(name, countof(name), fmt, args) };
  ASSERT_EX(len > 0);
  okCom(SetThreadDescription(GetCurrentThread(), name));
# else
  auto len{ vsnprintf(name, countof(name), fmt, args) };
  ASSERT_EX(len > 0);

#  if PLATFORM_APPLE
  okC(pthread_setname_np(name));
#  elif PLATFORM_LINUX
  okC(pthread_setname_np(pthread_self(), name));
#  else
#   error "Missing Thread::setName implementation"
#  endif
# endif

  va_end(args);
#endif
}

void Thread::setAffinity(u64 mask) {
#if PLATFORM_WINDOWS
  okWin(static_cast<DWORD>(SetThreadAffinityMask(GetCurrentThread(), mask)));
#elif PLATFORM_APPLE
  setThreadAffinity(pthread_mach_thread_np(pthread_self()), mask);
#elif PLATFORM_LINUX
  setThreadAffinity(pthread_self(), mask);
#else
# error "Missing Thread::setAffinity implementation"
#endif
}

void Thread::setPriority(Priority priority) {
#if PLATFORM_WINDOWS
  okWin(SetThreadPriority(GetCurrentThread(), static_cast<i32>(priority)));
#elif PLATFORM_POSIX
  auto param{ getThreadPriority(priority) };
  okC(pthread_setschedparam(pthread_self(), threadPolicy, &param));
#else
# error "Missing Thread::setPriority implementation"
#endif
}

u32 Thread::getNumCPUs() {
  static u32 numCPUs;
  if (numCPUs == 0) {
  #if PLATFORM_WINDOWS
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    numCPUs = si.dwNumberOfProcessors;
  #elif PLATFORM_POSIX
    numCPUs = static_cast<u32>(sysconf(_SC_NPROCESSORS_ONLN));
  #else
  # error "Missing Thread::getNumCPUs implementation"
  #endif
  }
  return numCPUs;
}
