#include "gpu/opengl/OpenGL.hpp"

#include "app/windows/WindowsMain.hpp"
#include "app/windows/WindowsWindow.hpp"

#include <GL/wglext.h>

#include <memory>

#define WGL_ARB_PROCS \
  /* WGL_ARB_create_context */                        \
  GL(CREATECONTEXTATTRIBS,   CreateContextAttribs);   \
  /* WGL_ARB_extensions_string */                     \
  GL(GETEXTENSIONSSTRING,    GetExtensionsString);    \
  /* WGL_ARB_pixel_format */                          \
  GL(CHOOSEPIXELFORMAT,      ChoosePixelFormat);      \
  GL(GETPIXELFORMATATTRIBFV, GetPixelFormatAttribfv); \
  GL(GETPIXELFORMATATTRIBIV, GetPixelFormatAttribiv)

#define GL(type, name) static PFNWGL##type##ARBPROC wgl##name##ARB
WGL_ARB_PROCS;
#undef GL

namespace Gpu::OpenGL::WGL {

static i32 pixelFormat;

static void queryPixelFormat(HDC dc) {
  if (wglChoosePixelFormatARB) {
    u32 numFormats;
    i32 numFormatsAttr[]{ WGL_NUMBER_PIXEL_FORMATS_ARB };
    okWin(wglGetPixelFormatAttribivARB(dc, 0, 0, 1, numFormatsAttr,
                                       reinterpret_cast<i32*>(&numFormats)));

    i32 formatAttrs[]{
      WGL_SUPPORT_OPENGL_ARB, true,
      WGL_DRAW_TO_WINDOW_ARB, true,
      WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB,
      WGL_COLOR_BITS_ARB,     24,
      WGL_DEPTH_BITS_ARB,     0,
      WGL_STENCIL_BITS_ARB,   0,
      WGL_DOUBLE_BUFFER_ARB,  true,
      WGL_SWAP_METHOD_ARB,    WGL_SWAP_EXCHANGE_ARB,
      WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
      0
    };
    std::unique_ptr<i32[]> formats(new i32[numFormats]);
    okWin(wglChoosePixelFormatARB(dc, formatAttrs, nullptr, 1,
                                  formats.get(), &numFormats));

    u32 bestFormatIndex{ 0u };
    i32 bestFormatScore{ -1 };
    i32 queryAttrs[]{
      WGL_ALPHA_BITS_ARB,
      WGL_DEPTH_BITS_ARB,
      WGL_STENCIL_BITS_ARB
    };
    i32 attrs[countof(queryAttrs)];
    for (auto n{ 0u }; n < numFormats; n++) {
      okWin(wglGetPixelFormatAttribivARB(dc, formats[n], 0, countof(queryAttrs),
                                         queryAttrs, attrs));

      auto score{ 0 };
      if (attrs[0] == 0) score++;
      if (attrs[1] == 0) score++;
      if (attrs[2] == 0) score++;

      if (score > bestFormatScore) {
        bestFormatIndex = n;
        bestFormatScore = score;

        if (score == countof(queryAttrs)) break;
      }
    }

    pixelFormat = formats[bestFormatIndex];
  }
  else {
    ASSERT(0, "TODO");
  }

  ASSERT(pixelFormat);
}

class Viewport : public Gpu::OpenGL::Viewport {
public:
  HWND  window;
  HDC   deviceContext;
  HGLRC renderContext;

  Viewport(App::Windows::Window const* window) : window(window->getHandle()) {
    HGLRC shareContext;

    if (window->isMainWindow()) {
      // Create a dummy OpenGL context.
      WNDCLASSEXW wc{ sizeof(wc) };
      wc.style         = CS_OWNDC;
      wc.lpfnWndProc   = DefWindowProcW;
      wc.hInstance     = GetModuleHandleW(nullptr);
      wc.lpszClassName = L"OpenGL";
      auto dummyClass{ okWin(reinterpret_cast<LPCWSTR>(RegisterClassExW(&wc))) };

      auto dummyWnd{ okWin(CreateWindowEx(0, dummyClass, L"", 0, 0, 0, 0, 0,
                                          nullptr, nullptr, wc.hInstance, nullptr)) };

      auto dummyDC{ okWin(GetDC(dummyWnd)) };

      PIXELFORMATDESCRIPTOR pfd{ sizeof(pfd) };
      pfd.nVersion   = 1;
      pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
      pfd.iPixelType = PFD_TYPE_RGBA;
      pfd.cColorBits = 24;
      pfd.iLayerType = PFD_MAIN_PLANE;

      auto dummyFormat{ okWin(ChoosePixelFormat(dummyDC, &pfd)) };
      okWin(SetPixelFormat(dummyDC, dummyFormat, &pfd));

      auto dummyContext{ okWin(wglCreateContext(dummyDC)) };
      okWin(wglMakeCurrent(dummyDC, dummyContext));

      // Load the WGL functions.
# define GL(type, name) wgl##name##ARB = \
  static_cast<PFNWGL##type##ARBPROC>(reinterpret_cast<void*>(wglGetProcAddress("wgl" #name "ARB")))
      WGL_ARB_PROCS;
# undef GL

      // Query WGL for extensions.
      if (wglGetExtensionsStringARB) {
        auto exts{ okWin(wglGetExtensionsStringARB(dummyDC)) };
        LOG(OpenGL, Info, "WGL Extensions: %s", exts);

        // TODO
      }

      // Cleanup the dummy OpenGL context.
      okWin(wglDeleteContext(dummyContext));
      okWin(ReleaseDC(dummyWnd, dummyDC));
      okWin(DestroyWindow(dummyWnd));
      okWin(UnregisterClass(dummyClass, wc.hInstance));

      shareContext = nullptr;
    }
    else {
      auto mainViewport{ App::Windows::Main::getMainWindow()->getViewport() };
      shareContext = static_cast<Viewport*>(mainViewport)->renderContext;
    }

    deviceContext = okWin(GetDC(this->window));

    if (!pixelFormat) {
      queryPixelFormat(deviceContext);
    }

    okWin(SetPixelFormat(deviceContext, pixelFormat, nullptr));

    // TODO extensions
    // - flush_control
    // - swap_control / swap_control_tear

    if (wglCreateContextAttribsARB) {
      // TODO check extension support
      // - create context no error
      // - profile / es_profile / es2_profile
      // - robustness

      i32 contextAttrs[]{
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 6,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        WGL_CONTEXT_FLAGS_ARB, /* WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB |*/ WGL_CONTEXT_DEBUG_BIT_ARB,
        0
      };
      renderContext = okWin(wglCreateContextAttribsARB(deviceContext, shareContext, contextAttrs));
    }
    else {
      ASSERT(0, "TODO");
    }
  }

  ~Viewport() {
    okWin(wglDeleteContext(renderContext));
    okWin(ReleaseDC(window, deviceContext));
  }

  GPU_IMP(void, present) {
    okWin(SwapBuffers(deviceContext));
  }

  GPU_IMP(void, resize, u32, u32) {}
};

} // namespace Gpu::OpenGL

void* Gpu::OpenGL::Viewport::getProcAddress(char const* name) {
  return wglGetProcAddress(name);
}

void Gpu::OpenGL::Viewport::makeCurrent() {
  auto self{ static_cast<Gpu::OpenGL::WGL::Viewport*>(this) };
  okWin(wglMakeCurrent(self->deviceContext, self->renderContext));
}

void Gpu::OpenGL::Viewport::clearCurrent() {
  okWin(wglMakeCurrent(nullptr, nullptr));
}

Gpu::Viewport* Gpu::OpenGL::Device::createViewport(App::Window const* window) {
  return new WGL::Viewport(static_cast<App::Windows::Window const*>(window));
}
