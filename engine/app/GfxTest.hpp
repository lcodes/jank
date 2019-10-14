#pragma once

#include "CoreTypes.hpp"

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
#else
# error TODO Implement SyncEvent for the target platform
#endif

class OpenGL {
public:
  SyncEvent renderReady;
  SyncEvent presentReady;

  f32 width;
  f32 height;
  f32 dpi;

  OpenGL() : renderReady(true), width(0), height(0), dpi(1) {}

  virtual void present() {}

  virtual void clearCurrent() = 0;
  virtual void makeCurrent() = 0;

  virtual double getDeltaTime() = 0;
};

void* renderMain(void* arg);
