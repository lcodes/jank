#pragma once

#include "app/Main.hpp"

#pragma warning(push, 0)
#include <Windows.h>
#pragma warning(pop)

int WINAPI wWinMain(_In_     HINSTANCE instance,
                    _In_opt_ HINSTANCE prevInstance,
                    _In_     PWSTR     cmdLine,
                    _In_     int       cmdShow);

namespace App::Windows {

class Window;

enum class Version : u8 {
  Win7   = 0x70,
  Win8   = 0x80,
  Win8_1 = 0x81,
  Win10  = 0xA0
};

class Main : public App::Main {
  friend int ::wWinMain(HINSTANCE, HINSTANCE, PWSTR, int);

  friend class Window;

  static Version osVersion;

  static struct {
    bool wantGainFocus : 1;
    bool wantLoseFocus : 1;
  } state;

  static void main();
  static void run();

public:
  static Window* getMainWindow() { return reinterpret_cast<Window*>(App::Main::getMainWindow()); }

  static Version getOSVersion() { return osVersion; }
};

} // namespace App::Windows
