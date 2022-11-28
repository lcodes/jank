#pragma once

#include "core/Core.hpp"

#include <functional>

DECL_LOG_SOURCE(Main, Trace);

namespace App {

class Window;

class Main : public Singleton<Main> {
  static bool running;

  Main() = delete;

protected:
  static Window* mainWindow;

  static void* showSplash();
  static void closeSplash(void* handle);
  static void updateSplash(void* handle, uchar const* caption, f32 percent);

  static void init();
  static void term();

  static void mainJob();

  static void gainFocus();
  static void loseFocus();

  static void pause();
  static void resume();

  static void lowMemory();

public:
  using Fn = std::function<void()>;

  static void runOnMainThread(Fn&& fn);

  static void quit();

  static bool isRunning() { return running; }

  static Window* getMainWindow() { return mainWindow; }
};

} // namespace App
