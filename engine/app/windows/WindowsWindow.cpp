#include "app/windows/WindowsWindow.hpp"
#include "app/windows/WindowsMain.hpp"

#include "core/CoreDebug.hpp"

#include <hidusage.h>
#include <ShellScalingApi.h>

namespace App::Windows {

// Monitor
// ----------------------------------------------------------------------------

static f32 getMonitorDpiScale(HMONITOR monitor) {
  auto x{ 96u };
  auto y{ 96u };
  //if ( >= Win8_1) {
    //okWin(GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &x, &y));
  //}
#if 0
  else {
  auto dc{ GetDC(nullptr) };
  x = static_cast<u32>(GetDeviceCaps(dc, LOGPIXELSX));
  y = static_cast<u32>(GetDeviceCaps(dc, LOGPIXELSY));
  ReleaseDC(nullptr, dc);
  }
#endif

  ASSERT(x == y, "Expected X and Y DPI values to match");
  return x / 96.f;
}

static BOOL enumMonitorProc(HMONITOR monitor, HDC, LPRECT, LPARAM) noexcept {
  MONITORINFO info;
  info.cbSize = sizeof(info);
  okWin(GetMonitorInfoW(monitor, &info));

  Monitor m;
  m.mainPos = {
    info.rcMonitor.left,
    info.rcMonitor.top
  };
  m.workPos = {
    info.rcWork.left,
    info.rcWork.top
  };
  m.mainSize = {
    static_cast<u32>(info.rcMonitor.right - info.rcMonitor.left),
    static_cast<u32>(info.rcMonitor.bottom - info.rcMonitor.top)
  };
  m.workSize = {
    static_cast<u32>(info.rcWork.right - info.rcWork.left),
    static_cast<u32>(info.rcWork.bottom - info.rcWork.top)
  };
  m.dpiScale = getMonitorDpiScale(monitor);

  // TODO store AppMonitor somewhere
  // TODO store HMONITOR too
  return true;
}

static void queryMonitors() {
  okWin(EnumDisplayMonitors(nullptr, nullptr, enumMonitorProc, 0));
}


// Window Class
// ----------------------------------------------------------------------------

static LPCWSTR classAtom;

static void createClass(UINT style, WNDPROC proc) {
  WNDCLASSEXW wc;
  wc.cbSize        = sizeof(wc);
  wc.style         = style;
  wc.lpfnWndProc   = proc;
  wc.cbClsExtra    = 0;
  wc.cbWndExtra    = 0;
  wc.hInstance     = okWin(GetModuleHandleW(nullptr));
  wc.hIcon         = okWin(LoadIconW(wc.hInstance, MAKEINTRESOURCEW(2)));
  wc.hCursor       = okWin(LoadCursorW(nullptr, IDC_ARROW));
  wc.hbrBackground = nullptr;
  wc.lpszMenuName  = nullptr;
  wc.lpszClassName = windowClassName;
  wc.hIconSm       = wc.hIcon;

  classAtom = okWin(reinterpret_cast<LPCWSTR>(RegisterClassExW(&wc)));
}

static void destroyClass() {
  okWin(UnregisterClass(classAtom, GetModuleHandleW(nullptr)));
}


// Window Procedure
// ----------------------------------------------------------------------------

LRESULT Window::proc(HWND wnd, UINT msg, WPARAM w, LPARAM l) noexcept {
  auto window{ reinterpret_cast<Window*>(GetWindowLongPtrW(wnd, GWLP_USERDATA)) };
  ASSERT_EX(window || msg == WM_GETMINMAXINFO || msg == WM_NCCREATE,
            "Unexpected window message 0x%04x before WM_NCCREATE", msg);

  switch (msg) {
  // Window Creation and Destruction
  // --------------------------------------------------------------------------

  case WM_NCCREATE: {
    ASSERT(!window);
    auto params{ reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams };
    SetWindowLongPtrW(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(params));
    return 1; // Continue with window creation.
  }

  case WM_DESTROY:
    if (window == Main::getMainWindow()) {
      Main::quit();
    }
    window->handle = nullptr;
    return 0;

  // State Changes
  // --------------------------------------------------------------------------

  case WM_GETMINMAXINFO:
    // Sent when the window is resized.
    // Note: check initialization because this is also sent before WM_NCCREATE.
    if (window) {
      Size min, max;
      window->getMinMaxSize(&min, &max);

      auto info{ reinterpret_cast<MINMAXINFO*>(l) };
      if (min.width != 0 && min.height != 0) {
        info->ptMinTrackSize = { static_cast<i32>(min.width), static_cast<i32>(min.height) };
      }
      if (max.width != 0 && max.height != 0) {
        info->ptMaxTrackSize = { static_cast<i32>(max.width), static_cast<i32>(max.height) };
      }
      return 0;
    }
    break;

  case WM_CLOSE:
    // TODO confirm
    // TODO sync GPU && destroy viewport
    okWin(DestroyWindow(wnd));
    //window->requestClose = true;
    return 0;

  case WM_MOVE:
    //window->requestMove = true;
    return 0;

  case WM_SIZE:
    //window->requestSize = true;
    return 0;

  case WM_SETFOCUS:
    Main::state.wantGainFocus = true;
    return 0;

  case WM_KILLFOCUS:
    Main::state.wantLoseFocus = true;
    return 0;

  case WM_SETTINGCHANGE:
    return 0;

  case WM_DEVICECHANGE:
    return 0;

  case WM_DISPLAYCHANGE:
    return 0;

  case WM_DPICHANGED:
    return 0;

  // Borderless Fullscreen
  // --------------------------------------------------------------------------

  //case WM_NCCALCSIZE:
  //case WM_NCHITTEST:
  //case WM_NCACTIVATE:
  //case WM_NCPAINT:

  // Painting
  // --------------------------------------------------------------------------

  case WM_PAINT: {
    // Windows sends WM_PAINT messages continuously until the update region is
    // validated. Therefore, perform an empty paint to validate the update region.
    PAINTSTRUCT ps;
    BeginPaint(wnd, &ps);
    EndPaint  (wnd, &ps);
    return 0;
  }

  case WM_ERASEBKGND:
    return 1; // Don't waste time erasing the background, but tell Windows we did it.

  // Input
  // --------------------------------------------------------------------------

  case WM_GETDLGCODE:
    return DLGC_WANTALLKEYS; // We want to handle all keyboard inputs outselves.

  case WM_SYSCOMMAND:
    if (w == SC_KEYMENU) return 0; // Disable menu shortcuts using the ALT key.
    break;

  case WM_SETCURSOR:
    break;

  case WM_TOUCH: {
    TOUCHINPUT inputs[10];
    auto numInputs{ LOWORD(w) };
    ASSERT_EX(numInputs < countof(inputs));

    auto handle{ reinterpret_cast<HTOUCHINPUT>(l) };
    okWin(GetTouchInputInfo(handle, numInputs, inputs, sizeof(TOUCHINPUT)));

    for (auto n{ 0 }; n < numInputs; n++) {
      auto& input{ inputs[n] };
      // TODO process
    }

    okWin(CloseTouchInputHandle(handle));
    return 0;
  }

  case WM_INPUT: {
    RAWINPUT raw;
    u32 rawSize{ sizeof(raw) };
    auto len{ GetRawInputData(reinterpret_cast<HRAWINPUT>(l), RID_INPUT,
                              &raw, &rawSize, sizeof(RAWINPUTHEADER)) };
    if (len == -1) failWin();

    switch (raw.header.dwType) {
    case RIM_TYPEMOUSE:
      if (!(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE)) {
        // raw.data.mouse.lLastX
      }
      else if (raw.data.mouse.usFlags & MOUSE_VIRTUAL_DESKTOP) {
        auto width{ GetSystemMetrics(SM_CXVIRTUALSCREEN) };

        //static_cast<i32>((static_cast<f32>(raw.data.mouse.lLastX) / UINT16_MAX) * width);
      }
      break;

    case RIM_TYPEKEYBOARD:
      //raw.data.keyboard.VKey
      break;

    case RIM_TYPEHID:
      //raw.data.hid.
      break;
    }

    return 0;
  }

  // Legacy Input
  // --------------------------------------------------------------------------

  case WM_CHAR:
    return 0;

  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
    return 0;
  case WM_KEYUP:
  case WM_SYSKEYUP:
    return 0;

  case WM_LBUTTONDOWN:
    return 0;
  case WM_LBUTTONUP:
    return 0;

  case WM_RBUTTONDOWN:
    return 0;
  case WM_RBUTTONUP:
    return 0;

  case WM_MBUTTONDOWN:
    return 0;
  case WM_MBUTTONUP:
    return 0;

  case WM_XBUTTONDOWN:
    return 0;
  case WM_XBUTTONUP:
    return 0;

  case WM_MOUSEWHEEL:
    return 0;
  case WM_MOUSEHWHEEL:
    return 0;

  case WM_MOUSEMOVE:
    return 0;

  case WM_MOUSELEAVE:
    return 0;
  }

  return DefWindowProcW(wnd, msg, w, l);
}


// Window
// ----------------------------------------------------------------------------

static void getStyles(Window::DisplayFlags flags, DWORD* style, DWORD* styleEx, bool layered) {
  *style   = static_cast<DWORD>(flags & Window::DisplayFlags::NoDecoration  ? WS_POPUP         : WS_OVERLAPPEDWINDOW);
  *styleEx = static_cast<DWORD>(flags & Window::DisplayFlags::NoTaskBarIcon ? WS_EX_TOOLWINDOW : WS_EX_APPWINDOW);
  if (flags & Window::DisplayFlags::TopMost) *styleEx |= WS_EX_TOPMOST;
  if (layered) *styleEx |= WS_EX_LAYERED;
}

void Window::init() {
  // TODO SetThreadDpiAwarenessContext

  queryMonitors();
  createClass(CS_OWNDC, Window::proc); // TODO: CS_HREDRAW | CS_VREDRAW

  RAWINPUTDEVICE rid[3];
  rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
  rid[0].usUsage     = HID_USAGE_GENERIC_MOUSE;
  rid[0].dwFlags     = 0;// RIDEV_NOLEGACY;
  rid[0].hwndTarget  = nullptr;

  rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
  rid[1].usUsage     = HID_USAGE_GENERIC_KEYBOARD;
  rid[1].dwFlags     = RIDEV_NOLEGACY;
  rid[1].hwndTarget  = nullptr;

  rid[2].usUsagePage = HID_USAGE_PAGE_GENERIC;
  rid[2].usUsage     = HID_USAGE_GENERIC_GAMEPAD;
  rid[2].dwFlags     = 0;
  rid[2].hwndTarget  = nullptr;
  okWin(RegisterRawInputDevices(rid, countof(rid), sizeof(RAWINPUTDEVICE)));
}

void Window::term() {
  // TODO unregister raw inputs

  destroyClass();

  // TODO free monitors
}

Window::Window(Params const& params) {
  ASSERT(params.title.data()[params.title.size()] == L'\0');

  getStyles(params.flags, &style, &styleEx, false);

  RECT rc{
    params.position.x,
    params.position.y,
    params.position.x + static_cast<i32>(params.size.width),
    params.position.y + static_cast<i32>(params.size.height)
  };
  okWin(AdjustWindowRectEx(&rc, style, false, styleEx));

  auto x{ params.position.x == defaultPosition ? CW_USEDEFAULT : rc.left };
  auto y{ params.position.y == defaultPosition ? CW_USEDEFAULT : rc.top };
  auto w{ rc.right - rc.left };
  auto h{ rc.bottom - rc.top };

  auto parent{ params.parent
    ? reinterpret_cast<Window*>(params.parent)->handle
    : nullptr };

  handle = okWin(CreateWindowEx(styleEx, classAtom, params.title.data(), style,
                                x, y, w, h, parent, nullptr,
                                okWin(GetModuleHandleW(nullptr)), this));

  okWin(RegisterTouchWindow(handle, TWF_FINETOUCH | TWF_WANTPALM));

  setup();
}

Window::~Window() {
  ASSERT(!handle, "Window destroyed before being closed");
}

void Window::setFullscreen(FullscreenMode mode) {
  if (mode != fullscreenMode) {
    // TODO
  }
}

void Window::move(Point pt) {
  RECT rc{ pt.x, pt.y };
  okWin(AdjustWindowRectEx(&rc, style, false, styleEx));
  okWin(SetWindowPos(handle, nullptr, rc.left, rc.top, 0, 0,
                     SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE));
}

void Window::resize(Size sz) {
  RECT rc{ 0, 0, static_cast<i32>(sz.width), static_cast<i32>(sz.height) };
  okWin(AdjustWindowRectEx(&rc, style, false, styleEx));
  okWin(SetWindowPos(handle, nullptr, 0, 0,
                     rc.right - rc.left, rc.bottom - rc.top,
                     SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE));
}

void Window::setTitle(UStringView title) {
  ASSERT(title.data()[title.size()] == L'\0');
  okWin(SetWindowTextW(handle, title.data()));
}

void Window::show() {
  ShowWindow(handle, SW_SHOW);
}

void Window::focus() {
  okWin(BringWindowToTop(handle));
  okWin(SetForegroundWindow(handle));
  okWin(SetFocus(handle));
}

void Window::close() {
  if (GetCapture() == handle) {
    okWin(ReleaseCapture());
    if (!mainWindow) {
      okWin(SetCapture(Main::getMainWindow()->handle));
    }
  }
  okWin(DestroyWindow(handle));
}

} // namespace App::Windows

App::Window* App::Window::create(Params const& params) {
  return new App::Windows::Window(params);
}
