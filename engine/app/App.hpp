#pragma once

#include "core/Core.hpp"
#include "core/CoreModule.hpp"

#include <functional>

DECL_LOG_SOURCE(App, Trace);

class AppWindow;

class App : public Singleton<App> {
  static bool running;

  App() = delete;

protected:
  static AppWindow* mainWindow;

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

  static AppWindow* getMainWindow() { return mainWindow; }
};

