#pragma once

#include "core/Core.hpp"

#define GFX_PRESENT_THREAD 1 && !PLATFORM_HTML5

#if PLATFORM_POSIX
# include <pthread.h>

class SyncEvent {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  bool flag;

public:
  SyncEvent(bool initValue = false) : flag(initValue) {
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);
  }

  ~SyncEvent() {
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
  }

  void set() {
    pthread_mutex_lock(&mutex);
    flag = true;
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond);
  }

  void wait() {
    pthread_mutex_lock(&mutex);
    while (!flag) pthread_cond_wait(&cond, &mutex);
    flag = false;
    pthread_mutex_unlock(&mutex);
  }
};
#elif PLATFORM_WINDOWS
# pragma warning(push, 0)
# include <Windows.h>
# pragma warning(pop)

class SyncEvent {
  HANDLE handle;

public:
  SyncEvent(bool initValue = false) :
    handle(CreateEventW(nullptr, false, initValue, nullptr)) {}

  ~SyncEvent() {
    CloseHandle(handle);
  }

  void set() {
    SetEvent(handle);
  }

  void wait() {
    WaitForSingleObject(handle, INFINITE);
  }
};
#elif PLATFORM_HTML5
// No implementation ?
#else
# error TODO Implement SyncEvent for the target platform
#endif

class OpenGL {
public:
#if GFX_PRESENT_THREAD
  SyncEvent renderReady;
  SyncEvent presentReady;
#endif

  f32 width;
  f32 height;
  f32 dpi;

  OpenGL() :
#if GFX_PRESENT_THREAD
    renderReady(true),
#endif
    width(0), height(0), dpi(1) {}

  virtual void* getProcAddress(char const* name UNUSED) { ASSERT(0); UNREACHABLE; }

  virtual void present() {}

  virtual void clearCurrent() = 0;
  virtual void makeCurrent() = 0;

  virtual f64 getDeltaTime() = 0;

#if PLATFORM_IPHONE
  virtual u32 getSurface() = 0;
#endif
};

void* renderMain(void* arg);
