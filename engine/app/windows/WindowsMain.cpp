#include "app/windows/WindowsMain.hpp"
#include "app/windows/WindowsWindow.hpp"

#include <concurrentqueue.h>

#pragma warning(push, 0)
#include <VersionHelpers.h>
#include <Windows.h>
#include <Windowsx.h>
#pragma warning(pop)

namespace App::Windows {

Version Main::osVersion;

constexpr u32 WM_USER_CALLBACK = WM_USER + 1;

static moodycamel::ConcurrentQueue<Main::Fn> mainThreadQueue;

#if 0 //GFX_PRESENT_THREAD
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

#if 0
static ImGuiMouseCursor lastCursor;
static HCURSOR cursors[ImGuiMouseCursor_COUNT];

static HCURSOR InitCursor(LPCWSTR name) { return LoadCursorW(nullptr, name); }

static bool imguiInit;

void jank_imgui_init() {
  auto& io{ ImGui::GetIO() };

  io.ConfigFlags |=
    ImGuiConfigFlags_IsSRGB;
  io.BackendFlags |=
    ImGuiBackendFlags_HasMouseCursors |
    ImGuiBackendFlags_HasSetMousePos |
    ImGuiBackendFlags_HasMouseHoveredViewport |
    //ImGuiBackendFlags_PlatformHasViewports |
    //ImGuiBackendFlags_RendererHasViewports |
    //ImGuiBackendFlags_RendererHasVtxOffset;
    0;

  io.BackendPlatformName = "Win32";
  io.KeyMap[ImGuiKey_Tab] = VK_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
  io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
  io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
  io.KeyMap[ImGuiKey_Home] = VK_HOME;
  io.KeyMap[ImGuiKey_End] = VK_END;
  io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
  io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
  io.KeyMap[ImGuiKey_Space] = VK_SPACE;
  io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
  io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
  io.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
  io.KeyMap[ImGuiKey_A] = 'A';
  io.KeyMap[ImGuiKey_C] = 'C';
  io.KeyMap[ImGuiKey_V] = 'V';
  io.KeyMap[ImGuiKey_X] = 'X';
  io.KeyMap[ImGuiKey_Y] = 'Y';
  io.KeyMap[ImGuiKey_Z] = 'Z';

  cursors[ImGuiMouseCursor_Arrow] = InitCursor(IDC_ARROW);
  cursors[ImGuiMouseCursor_TextInput] = InitCursor(IDC_IBEAM);
  cursors[ImGuiMouseCursor_ResizeAll] = InitCursor(IDC_SIZEALL);
  cursors[ImGuiMouseCursor_ResizeEW] = InitCursor(IDC_SIZEWE);
  cursors[ImGuiMouseCursor_ResizeNS] = InitCursor(IDC_SIZENS);
  cursors[ImGuiMouseCursor_ResizeNESW] = InitCursor(IDC_SIZENESW);
  cursors[ImGuiMouseCursor_ResizeNWSE] = InitCursor(IDC_SIZENWSE);
  cursors[ImGuiMouseCursor_Hand] = InitCursor(IDC_HAND);

  auto& style = ImGui::GetStyle();
  style.WindowRounding = 4;
  style.WindowPadding = { 4, 4 };
  style.FrameRounding = 4;
  style.ItemSpacing = { 4, 4 };
  style.IndentSpacing = 20;
  style.ScrollbarSize = 10;
  style.ScrollbarRounding = style.ScrollbarSize / 2;

  imguiInit = true;
}

#define MAIN_WINDOW() WindowsApp::getMainWindow()->getHandle()

static bool mouseCaptured;
static POINT windowCenter;

void captureMouse() {
  //SetCapture(wnd);
  //GetClipCursor(&oldMouseClip);
  RECT rc;
  GetWindowRect(MAIN_WINDOW(), &rc);
  //ClipCursor(&rc);

  ShowCursor(false);
  windowCenter.x = rc.left + (rc.right - rc.left) / 2;
  windowCenter.y = rc.top + (rc.bottom - rc.top) / 2;
  SetCursorPos(windowCenter.x, windowCenter.y);

  auto pt{ windowCenter };
  ScreenToClient(MAIN_WINDOW(), &pt);
  gl.mouseX = static_cast<f32>(pt.x);
  gl.mouseY = static_cast<f32>(pt.y);
  mouseCaptured = true;
}

bool ImGui_UpdateMouseCursor() {
  if (mouseCaptured) return false;

  auto& io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) return false;

  auto cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
  if (lastCursor != cursor) {
    lastCursor = cursor;
    SetCursor(cursor == ImGuiMouseCursor_None ? nullptr : cursors[cursor]);
  }
  return true;
}

static void ImGui_UpdateMousePos() {
  auto& io = ImGui::GetIO();
  auto hasVP = io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable;

  POINT pt;
  if (io.WantSetMousePos) {
    pt = {
      static_cast<int>(io.MousePos.x),
      static_cast<int>(io.MousePos.y)
    };
    if (!hasVP) ClientToScreen(MAIN_WINDOW(), &pt);
    SetCursorPos(pt.x, pt.y);
  }

  io.MouseHoveredViewport = 0;
#if 1
  io.MousePos = { -FLT_MAX, -FLT_MAX };

  if (auto activeWindow{ GetForegroundWindow() }; activeWindow == MAIN_WINDOW() || IsChild(activeWindow, MAIN_WINDOW())) {
    if (GetCursorPos(&pt) && ScreenToClient(MAIN_WINDOW(), &pt)) {
      io.MousePos = {
        static_cast<float>(pt.x),
        static_cast<float>(pt.y)
      };
    }
  }
#else
  if (!GetCursorPos(&pt)) {
    io.MousePos = { -FLT_MAX, -FLT_MAX };
    return;
  }

  if (auto active = GetForegroundWindow()) {
    if (IsChild(active, wnd)) active = wnd;

    if (hasVP) {
      if (ImGui::FindViewportByPlatformHandle(active)) {
        io.MousePos = {
          static_cast<float>(pt.x),
          static_cast<float>(pt.y)
        };
      }
    }
    else if (active == wnd) {
      auto clientPt = pt;
      if (ScreenToClient(wnd, &clientPt)) {
        io.MousePos = {
          static_cast<float>(clientPt.x),
          static_cast<float>(clientPt.y)
        };
      }
    }
  }

  if (auto hovered = WindowFromPoint(pt)) {
    if (auto vp = ImGui::FindViewportByPlatformHandle(hovered)) {
      if ((vp->Flags & ImGuiViewportFlags_NoInputs) == 0) {
        io.MouseHoveredViewport = vp->ID;
      }
    }
  }
#endif
}

void jank_imgui_newFrame() {
  if (!mouseCaptured && ImGui::IsMouseClicked(0) && !ImGui::IsAnyWindowHovered() && !ImGui::IsAnyItemHovered()) {
    captureMouse();
  }

  auto& io = ImGui::GetIO();
  // TODO io.DisplaySize
  // TODO update monitors

  io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
  io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
  io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
  io.KeySuper = false;

  ImGui_UpdateMousePos();
  ImGui_UpdateMouseCursor();
}

// TODO change to CF_UNICODETEXT && convert
char const* jank_imgui_getClipboardText(void*) {
  OpenClipboard(nullptr);
  auto data{ GetClipboardData(CF_TEXT) };
  auto text{ static_cast<char const*>(GlobalLock(data)) };
  auto len{ strlen(text) + 1 };
  auto mem{ reinterpret_cast<char*>(malloc(len)) };
  memcpy(mem, text, len);
  GlobalUnlock(data);
  CloseClipboard();
  return mem;
}

void jank_imgui_setClipboardText(void*, char const* text) {
  auto len{ strlen(text) + 1 };
  auto mem{ GlobalAlloc(GMEM_MOVEABLE, len) };
  memcpy(GlobalLock(mem), text, len);
  GlobalUnlock(mem);
  OpenClipboard(nullptr);
  EmptyClipboard();
  SetClipboardData(CF_TEXT, mem);
  CloseClipboard();
}

static void setKey(WPARAM key, bool state) {
  InputKeys k;
  switch (key) {
  case 'W': k = InputKeys::W; break;
  case 'S': k = InputKeys::S; break;
  case 'A': k = InputKeys::A; break;
  case 'D': k = InputKeys::D; break;
  case VK_SHIFT: k = InputKeys::Shift; break;
  case VK_SPACE: k = InputKeys::Space; break;
  default: return;
  }
  gl.input[static_cast<u32>(k)] = state;
}

bool ImGui_WndProc(HWND wnd, UINT msg, WPARAM w, LPARAM l) {
  int btn;

  switch (msg) {
  case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: btn = 0; goto handleMouseDown;
  case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: btn = 1; goto handleMouseDown;
  case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: btn = 2; goto handleMouseDown;
  case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
    btn = GET_XBUTTON_WPARAM(w) == XBUTTON1 ? 3 : 4;
  handleMouseDown:
    if (!ImGui::IsAnyMouseDown() && GetCapture() == nullptr) SetCapture(wnd);
    ImGui::GetIO().MouseDown[btn] = true;
    return true;

  case WM_LBUTTONUP: btn = 0; goto handleMouseUp;
  case WM_RBUTTONUP: btn = 1; goto handleMouseUp;
  case WM_MBUTTONUP: btn = 2; goto handleMouseUp;
  case WM_XBUTTONUP:
    btn = GET_XBUTTON_WPARAM(w) == XBUTTON1 ? 3 : 4;
  handleMouseUp:
    ImGui::GetIO().MouseDown[btn] = false;
    if (!ImGui::IsAnyMouseDown() && GetCapture() == wnd) ReleaseCapture();
    return true;

  case WM_MOUSEWHEEL:
    ImGui::GetIO().MouseWheel += static_cast<float>(GET_WHEEL_DELTA_WPARAM(w)) / WHEEL_DELTA;
    return true;
  case WM_MOUSEHWHEEL:
    ImGui::GetIO().MouseWheelH += static_cast<float>(GET_WHEEL_DELTA_WPARAM(w)) / WHEEL_DELTA;
    return true;

  case WM_KEYDOWN: case WM_SYSKEYDOWN:
    if (w <= UINT8_MAX) ImGui::GetIO().KeysDown[w] = true;
    return true;
  case WM_KEYUP: case WM_SYSKEYUP:
    if (w <= UINT8_MAX) ImGui::GetIO().KeysDown[w] = false;
    return true;

  case WM_CHAR:
    ImGui::GetIO().AddInputCharacter(static_cast<unsigned int>(w));
    return true;

  case WM_SETCURSOR:
    if (imguiInit && LOWORD(l) == HTCLIENT) return ImGui_UpdateMouseCursor();
    break;

  case WM_DISPLAYCHANGE:
    //updateMonitors = true;
    return true;
  }

  return false;
}

static LRESULT wndProc(HWND wnd, UINT msg, WPARAM w, LPARAM l) noexcept {

  //if (!mouseCaptured && ImGui_WndProc(wnd, msg, w, l)) {
  //  return 0;
  //}

  switch (msg) {
  case WM_SIZE: {
    auto width{ LOWORD(l) };
    auto height{ HIWORD(l) };
    gl.width = width;
    gl.height = height;
    return 0;
  }

  case WM_KEYDOWN:
    setKey(w, true);
    return 0;
  case WM_KEYUP:
    setKey(w, false);
    if (w == VK_ESCAPE && mouseCaptured) {
      mouseCaptured = false;
      //ReleaseCapture();
      //ClipCursor(&oldMouseClip);
      ShowCursor(true);
    }
    return 0;

  case WM_LBUTTONDOWN:
    if (!mouseCaptured) {
      captureMouse();
    }
    return 0;

  case WM_MOUSELEAVE:
    gl.mouseX = -1;
    gl.mouseY = -1;
    return 0;

  case WM_MOUSEMOVE:
    if (mouseCaptured) {
      auto x{ static_cast<f32>(GET_X_LPARAM(l)) };
      auto y{ static_cast<f32>(GET_Y_LPARAM(l)) };
      if (x != gl.mouseX || y != gl.mouseY) {
        gl.xOffset += x - gl.mouseX;
        gl.yOffset += y - gl.mouseY;
        SetCursorPos(windowCenter.x, windowCenter.y);
      }
    }
    return 0;

  case WM_USER_CALLBACK: {
    App::Fn fn;
    auto ret{ mainThreadQueue.try_dequeue(fn) };
    ASSERT(ret);
    fn();
    break;
  }
  }
  return DefWindowProcW(wnd, msg, w, l);
}
#endif

#if BUILD_DEVELOPMENT
// See WindowsDebug.cpp
void initConsole();
void initDebugThread();
void termDebugThread();
#endif

void Main::main() {
  // TODO: Parse args

  auto singleton{ okWin(CreateMutexW(nullptr, false, windowClassName)) };
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    if (auto window{ FindWindowW(windowClassName, nullptr) }) {
      okWin(SetForegroundWindow(window));
      okWin(BringWindowToTop(window));
    }

    okWin(CloseHandle(singleton));
    exit(EXIT_FAILURE);
  }

  /**/ if (IsWindows10OrGreater())      osVersion = Version::Win10;
  else if (IsWindows8Point1OrGreater()) osVersion = Version::Win8_1;
  else if (IsWindows8OrGreater())       osVersion = Version::Win8;
  else if (IsWindows7OrGreater())       osVersion = Version::Win7;
  else {
    errorMessageBox(L"Unsupported Windows Version", L"Windows 7 or greater is required.");
    exit(EXIT_FAILURE);
  }

  okCom(CoInitializeEx(nullptr,
                       COINIT_MULTITHREADED |
                       COINIT_DISABLE_OLE1DDE |
                       COINIT_SPEED_OVER_MEMORY));

  // TODO load config
  // TODO gpu driver -> wnd class style

#if BUILD_DEVELOPMENT
  initConsole();
  initDebugThread();
#endif

  Window::init();

  init();
  run();
  term();

#if BUILD_DEVELOPMENT
  termDebugThread();
#endif

  CoUninitialize();

  okWin(CloseHandle(singleton));
}

void Main::run() {
  MSG msg;
  BOOL result;

  // Run the message pump until WM_QUIT (ret = 0) or failure (ret = -1).
  while ((result = GetMessageW(&msg, nullptr, 0, 0)) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);

    // Empty the message queue, required to gracefully handle application focus.
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      if (msg.message == WM_QUIT) return; // PeekMessage always retrieves WM_QUIT.

      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }

    // Update the application focus state from the received window focus events.
    // Does nothing if the focus transferred from one application window to another.
    if (state.wantLoseFocus) {
      state.wantLoseFocus = false;
      if (state.wantGainFocus) {
        state.wantGainFocus = false;
      }
      else {
        loseFocus();
      }
    }
    else if (state.wantGainFocus) {
      state.wantGainFocus = false;
      gainFocus();
    }
  }

  okWin(result != -1);
  ASSERT(msg.message == WM_QUIT);
}

} // namespace App::Windows

void App::Main::runOnMainThread(Fn&& fn) {
  App::Windows::mainThreadQueue.enqueue(std::move(fn));

  ASSERT(0, "TODO");
  //okWin(PostMessageW(MAIN_WINDOW(), WM_USER_CALLBACK, 0, 0));
}

void App::Main::quit() {
  PostQuitMessage(0);
}

// Entry point when using the Windows subsystem.
int WINAPI wWinMain(_In_     HINSTANCE instance     UNUSED,
                    _In_opt_ HINSTANCE prevInstance UNUSED,
                    _In_     PWSTR     cmdLine      UNUSED,
                    _In_     int       cmdShow      UNUSED) {
  // TODO check prevInstance

  App::Windows::Main::main();
  return 0;
}

/// Entry point when using the Console subsystem.
int main() {
  return wWinMain(nullptr, nullptr, nullptr, 0);
}
