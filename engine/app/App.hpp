#pragma once

#include "Core.hpp"

DECL_LOG_SOURCE(App, Trace);

/// Manages the application state and handles events from the OS.
class App : NonCopyable {
  // Current time
  // Delta time

  // Total frames
  // Average FPS

  // Paths

  // Project name

protected:
  App() = default;

  virtual void onFocusGained();
  virtual void onFocusLost();

  virtual void onPause();
  virtual void onResume();
};
