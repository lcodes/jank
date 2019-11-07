#include "app/windows/WindowsWindow.hpp"
#include "app/windows/WindowsApp.hpp"

#include "core/CoreDebug.hpp"

//#include <ShellScalingApi.h>


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

  AppMonitor m;
  m.mainPos = {
    info.rcMonitor.left,
    info.rcMonitor.top
  };
  m.workPos = {
    info.rcWork.left,
    info.rcWork.top
  };
  m.mainSize = {
    static_cast<u32>(info.rcMonitor.right  - info.rcMonitor.left),
    static_cast<u32>(info.rcMonitor.bottom - info.rcMonitor.top)
  };
  m.workSize = {
    static_cast<u32>(info.rcWork.right  - info.rcWork.left),
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
  wc.hIcon         = okWin(LoadIconW(nullptr, IDI_APPLICATION)); // TODO MAKEINTRESOURCEW(2)
  wc.hCursor       = okWin(LoadCursorW(nullptr, IDC_ARROW));
  wc.hbrBackground = nullptr;
  wc.lpszMenuName  = nullptr;
  wc.lpszClassName = L"Jank";
  wc.hIconSm       = wc.hIcon;

  classAtom = okWin(reinterpret_cast<LPCWSTR>(RegisterClassExW(&wc)));
}

static void destroyClass() {
  okWin(UnregisterClass(classAtom, GetModuleHandleW(nullptr)));
}


// Window Procedure
// ----------------------------------------------------------------------------

LRESULT WindowsWindow::proc(HWND wnd, UINT msg, WPARAM w, LPARAM l) noexcept {
  auto window{ reinterpret_cast<WindowsWindow*>(GetWindowLongPtrW(wnd, GWLP_USERDATA)) };
  if (!window) {
    if (msg == WM_NCCREATE) {
      auto params{ reinterpret_cast<CREATESTRUCTW*>(l)->lpCreateParams };
      SetWindowLongPtrW(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(params));
      return 1;
    }
    return DefWindowProcW(wnd, msg, w, l);
  }

  switch (msg) {
  case WM_DESTROY:
    if (window == WindowsApp::getMainWindow()) {
      PostQuitMessage(0);
    }
    window->handle = nullptr;
    return 0;

  case WM_CLOSE:
    // TODO confirm
    okWin(DestroyWindow(wnd));
    return 0;

  case WM_GETMINMAXINFO: {
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

  //case WM_NCCALCSIZE:
  //case WM_NCHITTEST:
  //case WM_NCACTIVATE:
  //case WM_NCPAINT:

  case WM_PAINT: {
    PAINTSTRUCT ps;
    BeginPaint(wnd, &ps);
    EndPaint(wnd, &ps);
    return 0;
  }

  case WM_ERASEBKGND:
    return 1;

  case WM_SETCURSOR:
    break;

  case WM_MOVE:
    return 0;

  case WM_SIZE:
    return 0;

  case WM_SETFOCUS:
    WindowsApp::state.wantGainFocus = true;
    return 0;

  case WM_KILLFOCUS:
    WindowsApp::state.wantLoseFocus = true;
    return 0;

  case WM_GETDLGCODE:
    return DLGC_WANTALLKEYS;

  case WM_SYSCOMMAND:
    if ((w & 0xFFF0) == SC_KEYMENU) return 0;
    break;

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

  case WM_SETTINGCHANGE:
    return 0;

  case WM_DEVICECHANGE:
    return 0;

  case WM_DISPLAYCHANGE:
    return 0;

  case WM_DPICHANGED:
    return 0;
  }

  return DefWindowProcW(wnd, msg, w, l);
}


// Window
// ----------------------------------------------------------------------------

static void getStyles(AppWindow::DisplayFlags flags, DWORD* style, DWORD* styleEx, bool layered) {
  *style = flags & AppWindow::DisplayFlags::NoDecoration ? WS_POPUP : WS_OVERLAPPEDWINDOW;
  *styleEx = flags & AppWindow::DisplayFlags::NoTaskBarIcon ? WS_EX_TOOLWINDOW : WS_EX_APPWINDOW;
  if (flags & AppWindow::DisplayFlags::TopMost) *styleEx |= WS_EX_TOPMOST;
  if (layered) *styleEx |= WS_EX_LAYERED;
}

void WindowsWindow::init() {
  // TODO SetThreadDpiAwarenessContext

  queryMonitors();
  createClass(CS_OWNDC, WindowsWindow::proc); // TODO: CS_HREDRAW | CS_VREDRAW
}

void WindowsWindow::term() {
  destroyClass();
}

AppWindow* AppWindow::create(Params const& params) {
  return new WindowsWindow(params);
}

WindowsWindow::WindowsWindow(Params const& params) {
  ASSERT(params.title.data()[params.title.size()] == L'\0');

  getStyles(params.flags, &style, &styleEx, false);

  RECT rc{
    params.position.x,
    params.position.y,
    params.position.x + params.size.width,
    params.position.y + params.size.height
  };
  okWin(AdjustWindowRectEx(&rc, style, false, styleEx));

  auto x{ params.position.x == defaultPosition ? CW_USEDEFAULT : rc.left };
  auto y{ params.position.y == defaultPosition ? CW_USEDEFAULT : rc.top };
  auto w{ rc.right  - rc.left };
  auto h{ rc.bottom - rc.top };

  auto parent{ params.parent
    ? reinterpret_cast<WindowsWindow*>(params.parent)->handle
    : nullptr };

  handle = okWin(CreateWindowEx(styleEx, classAtom, params.title.data(), style,
                                x, y, w, h, parent, nullptr,
                                okWin(GetModuleHandleW(nullptr)), this));
}

WindowsWindow::~WindowsWindow() {
  ASSERT(!handle, "Window destroyed before being closed");
}

void WindowsWindow::setFullscreen(FullscreenMode mode) {
  if (mode != fullscreenMode) {

  }
}

void WindowsWindow::move(Point pt) {
  RECT rc{ pt.x, pt.y };
  okWin(AdjustWindowRectEx(&rc, style, false, styleEx));
  okWin(SetWindowPos(handle, nullptr, rc.left, rc.top, 0, 0,
                     SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE));
}

void WindowsWindow::resize(Size sz) {
  RECT rc{ 0, 0, sz.width, sz.height };
  okWin(AdjustWindowRectEx(&rc, style, false, styleEx));
  okWin(SetWindowPos(handle, nullptr, 0, 0,
                     rc.right - rc.left, rc.bottom - rc.top,
                     SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE));
}

void WindowsWindow::setTitle(UStringView title) {
  ASSERT(title.data()[title.size()] == L'\0');
  okWin(SetWindowTextW(handle, title.data()));
}

void WindowsWindow::focus() {
  okWin(BringWindowToTop(handle));
  okWin(SetForegroundWindow(handle));
  okWin(SetFocus(handle));
}

void WindowsWindow::close() {
  if (GetCapture() == handle) {
    okWin(ReleaseCapture());
    if (!mainWindow) {
      okWin(SetCapture(WindowsApp::getMainWindow()->handle));
    }
  }
  okWin(DestroyWindow(handle));
}

