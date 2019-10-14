#include "Core.hpp"

#include "GfxTest.hpp"

#include <Windows.h>

class WindowsOpenGL : public OpenGL {
  HDC   dc;
  HGLRC context;

public:
  void clearCurrent() {
    wglMakeCurrent(nullptr, nullptr);
  }

  void makeCurrent() {
    wglMakeCurrent(dc, context);
  }
};

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE prevInstance, PWSTR cmdLine, int cmdShow) {
  // Parse args

  // Create window
#if 0
  PIXELFORMATDESCRIPTOR pfd{};
  pfd.nSize = 1;
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 32;
  pfd.iLayerType = PFD_MAIN_PLANE;

  auto format{ ChoosePixelFormat(dc, &pfd) };
  SetPixelFormat(dc, format, &pfd);

  auto gl{ wglCreateContext(dc) };

  wglGetProcAddress("");
#endif
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
