#include "Core.hpp"

#if PLATFORM_WINDOWS

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prevInstance, PWSTR cmdLine, int cmdShow) {
  // Parse args

  // Create window

  MSG msg;
  while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT) {
      break;
    }

    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  // Terminate

  return 0;
}

#endif
