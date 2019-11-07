#pragma once

#include "app/AppWindow.hpp"

#include <Windows.h>

// - window classes
// - window styles
// - window procs

// global
// - setting change

// monitors
// - dpi changed
// - display changed

// window state
// - title
// - position
// - size
// - min/max size
// - hover

// events
// - close
// - destroy

// input
// - keyboard
// - mouse
// - gamepad (dis)connected

class WindowsApp;

class WindowsWindow : public AppWindow {
  friend class AppWindow;  // For AppWindow::create()
  friend class WindowsApp; // For init() and term()

  HWND handle;
  DWORD style{ 0 };
  DWORD styleEx{ 0 };

  static void init();
  static void term();

  static LRESULT CALLBACK proc(HWND wnd, UINT msg, WPARAM w, LPARAM l) noexcept;

  WindowsWindow(Params const& params);
  ~WindowsWindow();

public:
  HWND getHandle() const { return handle; }

  void setFullscreen(FullscreenMode mode) override;

  void move(Point pt) override;
  void resize(Size sz) override;

  void setTitle(UStringView title) override;

  void focus() override;
  void close() override;
};

// App::Window -> Gpu::Viewport

// Windows
// - Main Viewport
// - Main Editor
// - ImGui Viewport

// Class Styles
// - DirectX => CS_HREDRAW | CS_VREDRAW
// - OpenGL => CS_OWNDC
