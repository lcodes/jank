#include "app/App.hpp"

#include "app/GfxTest.hpp"

#pragma warning(push, 0)
#include <Windows.h>
#include <GL/GL.h>
#include <GL/wglext.h>
#pragma warning(pop)

#include <process.h>

#define WGL_ARB_PROCS \
  /* WGL_ARB_create_context */ \
  GL(CREATECONTEXTATTRIBS, CreateContextAttribs); \
  /* WGL_ARB_extensions_string */ \
  GL(GETEXTENSIONSSTRING, GetExtensionsString); \
  /* WGL_ARB_pixel_format */ \
  GL(CHOOSEPIXELFORMAT, ChoosePixelFormat)

#define GL(type, name) static PFNWGL##type##ARBPROC wgl##name##ARB
WGL_ARB_PROCS;
#undef GL

class WindowsOpenGL : public OpenGL {
public:
  HDC   dc;
  HGLRC context;

  void* getProcAddress(char const* name) override {
    return wglGetProcAddress(name);
  }

  void present() override {
    SwapBuffers(dc);
  }

  void clearCurrent() override {
    wglMakeCurrent(nullptr, nullptr);
  }

  void makeCurrent() override {
    wglMakeCurrent(dc, context);
  }

  f64 getDeltaTime() override {
    return 0.016;
  }
};

static u32 renderMainWin(void* arg) {
#if GFX_PRESENT_THREAD
  renderMain(arg);
#else
  auto gl{ reinterpret_cast<WindowsOpenGL*>(arg) };
  while (true) {
    renderMain(arg);
  }
#endif
  return 0;
}

#if GFX_PRESENT_THREAD
static u32 presentMain(void* arg) {
  auto gl{ reinterpret_cast<WindowsOpenGL*>(arg) };
  while (true) {
    gl->presentReady.wait();
    gl->present();
    gl->renderReady.set();
  }
  return 0;
}
#endif

static LRESULT wndProc(HWND wnd, UINT msg, WPARAM w, LPARAM l) noexcept {
  if (msg == WM_DESTROY) {
    PostQuitMessage(0);
  }
  return DefWindowProcW(wnd, msg, w, l);
}
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
static void redirect(DWORD stdHandle, char const* mode, FILE* fp) {
    auto std{ GetStdHandle(stdHandle) };
    auto osf{ _open_osfhandle(reinterpret_cast<intptr_t>(std), _O_TEXT) };
    auto fd { _fdopen(osf, mode) };
    auto ret{ setvbuf(fd, nullptr, _IONBF, 0) };
    ASSERT(ret == 0, "");
    *fp = *fd;
}

int WINAPI wWinMain(_In_     HINSTANCE instance,
                    _In_opt_ HINSTANCE prevInstance,
                    _In_     PWSTR cmdLine,
                    _In_     int cmdShow)
{
  // Parse args
#if 0
  AllocConsole();

  CONSOLE_SCREEN_BUFFER_INFO info;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
  info.dwSize.Y = 1024;
  SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), info.dwSize);

  redirect(STD_OUTPUT_HANDLE, "w", stdout);
  redirect(STD_ERROR_HANDLE, "w", stderr);
  redirect(STD_INPUT_HANDLE, "r", stdin);
  //ios::sync_with_stdio();
  {
    fprintf(stdout, "FOO HI\n");
    fflush(stdout);
  }
  //LOG(App, Info, "Test");
#endif

  WindowsOpenGL gl;
  gl.width = 1920;
  gl.height = 1024;

  // Create window
  WNDCLASSEXW wc{ sizeof(wc) };
  wc.style = CS_OWNDC;// CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = wndProc;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.hIcon = LoadIconW(wc.hInstance, MAKEINTRESOURCEW(2));
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BACKGROUND + 1);
  wc.lpszClassName = L"OpenGL";
  wc.hIconSm = wc.hIcon;
  auto classAtom = reinterpret_cast<LPCWSTR>(RegisterClassExW(&wc));

  auto styleEx = WS_EX_APPWINDOW;
  auto style = WS_OVERLAPPEDWINDOW;
  RECT rc{ 0, 0, gl.width, gl.height };
  AdjustWindowRectEx(&rc, style, false, styleEx);

  auto wnd = CreateWindowEx(styleEx, classAtom, TEXT(""), style,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            rc.right - rc.left,
                            rc.bottom - rc.top,
                            nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);

  ShowWindow(wnd, SW_SHOWDEFAULT);

  gl.dc = GetDC(wnd);

  PIXELFORMATDESCRIPTOR pfd{};
  pfd.nSize = 1;
  pfd.nVersion = 1;
  pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 32;
  pfd.iLayerType = PFD_MAIN_PLANE;

  auto format{ ChoosePixelFormat(gl.dc, &pfd) };
  SetPixelFormat(gl.dc, format, &pfd);

  gl.context = wglCreateContext(gl.dc);
  gl.makeCurrent();

#define GL(type, name) wgl##name##ARB = reinterpret_cast<PFNWGL##type##ARBPROC>(wglGetProcAddress("wgl" #name "ARB"))
  WGL_ARB_PROCS;
#undef GL

  auto whee = glGetString(GL_EXTENSIONS); // deprecated in 3.0, removed in 3.1
  auto exts = wglGetExtensionsStringARB(gl.dc);

  LOG(App, Info, "Fake Context Core extensions: %s", whee);
  LOG(App, Info, "Fake Context WGL extensions: %s", exts);
  LOG(App, Info, "Fake Context Version: %s", glGetString(GL_VERSION));
  LOG(App, Info, "Fake Context Vendor: %s", glGetString(GL_VENDOR));
  LOG(App, Info, "Fake Context Renderer: %s", glGetString(GL_RENDERER));
  //LOG(App, Info, "Fake Context GLSL: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

  wglDeleteContext(gl.context);
  //ReleaseDC(wnd, gl.dc);

  // TODO debug context, forward compatible
  u32 numFormats;
  i32 formatAttrs[]{
    WGL_SUPPORT_OPENGL_ARB, true,
    WGL_DRAW_TO_WINDOW_ARB, true,
    WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
    WGL_COLOR_BITS_ARB, 24,
    WGL_DEPTH_BITS_ARB, 0,
    WGL_STENCIL_BITS_ARB, 0,
    WGL_DOUBLE_BUFFER_ARB, true,
    WGL_SWAP_METHOD_ARB, WGL_SWAP_EXCHANGE_ARB,
    WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
    0
  };
  wglChoosePixelFormatARB(gl.dc, formatAttrs, nullptr, 1, &format, &numFormats);
  SetPixelFormat(gl.dc, format, &pfd);

  i32 contextAttrs[]{
    WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    WGL_CONTEXT_MINOR_VERSION_ARB, 6,
    WGL_CONTEXT_FLAGS_ARB, 0,
    0
  };
  gl.context = wglCreateContextAttribsARB(gl.dc, nullptr, contextAttrs);
  gl.makeCurrent();

  exts = wglGetExtensionsStringARB(gl.dc);

  LOG(App, Info, "WGL extensions: %s", exts);
  LOG(App, Info, "Version: %s", glGetString(GL_VERSION));
  LOG(App, Info, "Vendor: %s", glGetString(GL_VENDOR));
  LOG(App, Info, "Renderer: %s", glGetString(GL_RENDERER));

  gl.clearCurrent();

  _beginthreadex(nullptr, 0, renderMainWin, &gl, 0, nullptr);

#if GFX_PRESENT_THREAD
  _beginthreadex(nullptr, 0, presentMain, &gl, 0, nullptr);
#endif

  MSG msg;
  auto running{ true };
  while (running) {
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) {
        running = false;
        break;
      }

      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }

    // TODO run
    Sleep(10);
  }

  // Terminate

  return 0;
}

int main() {
  return wWinMain(nullptr, nullptr, nullptr, 0);
}

