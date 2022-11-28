#pragma once

#include "app/Window.hpp"

#pragma warning(push, 0)
#include <Windows.h>
#pragma warning(pop)

namespace App::Windows {

constexpr uchar const* windowClassName = L"Jank";

class Main;

class Window : public App::Window {
  friend class App::Window; // For AppWindow::create()
  friend class Main;        // For init() and term()

  HWND  handle;
  DWORD style;
  DWORD styleEx;

  static void init();
  static void term();

  static LRESULT CALLBACK proc(HWND wnd, UINT msg, WPARAM w, LPARAM l) noexcept;

  Window(Params const& params);
  ~Window();

public:
  HWND getHandle() const { return handle; }

  void setFullscreen(FullscreenMode mode) override;

  void move(Point pt) override;
  void resize(Size sz) override;

  void setTitle(UStringView title) override;

  void show() override;

  void focus() override;
  void close() override;
};

} // namespace App::Windows
