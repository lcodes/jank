#pragma once

#include "app/App.hpp"

#include <Windows.h>

int WINAPI wWinMain(_In_     HINSTANCE instance,
                    _In_opt_ HINSTANCE prevInstance,
                    _In_     PWSTR     cmdLine,
                    _In_     int       cmdShow);

class WindowsWindow;

enum class WindowsVersion : u8 {
  Win7   = 0x70,
  Win8   = 0x80,
  Win8_1 = 0x81,
  Win10  = 0xA0
};

class WindowsApp : public App {
  friend int wWinMain(HINSTANCE, HINSTANCE, PWSTR, int);

  friend class WindowsWindow;

  static WindowsVersion osVersion;

  static struct {
    bool wantGainFocus : 1;
    bool wantLoseFocus : 1;
  } state;

  static void main();
  static void run();

public:
  static WindowsWindow* getMainWindow() { return reinterpret_cast<WindowsWindow*>(App::getMainWindow()); }

  static WindowsVersion getOSVersion() { return osVersion; }
};
