#include "app/Main.hpp"

#pragma warning(push, 0)
#include <Windows.h>
#include <wincodec.h>
#pragma warning(pop)

constexpr uchar const* className = L"Splash";

void* App::Main::showSplash() {
  IWICImagingFactory* factory;
  okCom(CoCreateInstance(CLSID_WICImagingFactory2, nullptr, CLSCTX_INPROC_SERVER,
                         __uuidof(IWICImagingFactory), reinterpret_cast<void**>(&factory)));

  // TODO proper path
  IWICBitmapDecoder* decoder;
  okCom(factory->CreateDecoderFromFilename(L"../assets/macos/icon_512x512.png", nullptr,
                                           GENERIC_READ, WICDecodeMetadataCacheOnDemand,
                                           &decoder));

  IWICBitmapFrameDecode* frame;
  okCom(decoder->GetFrame(0, &frame));

  IWICFormatConverter* converter;
  okCom(factory->CreateFormatConverter(&converter));
  okCom(converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone,
                              nullptr, 0.f, WICBitmapPaletteTypeMedianCut));

  i32 width, height;
  okCom(converter->GetSize(reinterpret_cast<u32*>(&width),
                           reinterpret_cast<u32*>(&height)));

  auto pixelsSize{ width * height * 4u };
  auto pixels{ new u8[pixelsSize] };
  okCom(converter->CopyPixels(nullptr, width * 4u, pixelsSize, pixels));

  auto bmp{ okWin(CreateBitmap(width, height, 1, 32, pixels)) };
  delete pixels;

  converter->Release();
  frame->Release();
  decoder->Release();
  factory->Release();

  WNDCLASSEXW wc{ sizeof(wc) };
  wc.style = 0;// CS_DROPSHADOW;
  wc.lpfnWndProc   = DefWindowProcW;
  wc.hInstance     = GetModuleHandleW(nullptr);
  wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
  wc.lpszClassName = className;

  auto classAtom{ reinterpret_cast<LPCWSTR>(okWin(RegisterClassExW(&wc))) };

  // TODO monitors are already queried in WindowsWindow.cpp, use them
  MONITORINFO mi;
  mi.cbSize = sizeof(mi);
  auto monitor{ okWin(MonitorFromPoint({ 0, 0 }, MONITOR_DEFAULTTOPRIMARY)) };
  okWin(GetMonitorInfoW(monitor, &mi));

  // TODO title from product name
  auto x{ mi.rcMonitor.left + (mi.rcMonitor.right - mi.rcMonitor.left - width) / 2 };
  auto y{ mi.rcMonitor.top + (mi.rcMonitor.bottom - mi.rcMonitor.top - height) / 2 };
  auto window{ okWin(CreateWindowExW(WS_EX_LAYERED, classAtom, L"Jank",
                                     WS_POPUPWINDOW | WS_VISIBLE, x, y, width, height,
                                     nullptr, nullptr, wc.hInstance, nullptr)) };

  auto dc{ okWin(CreateCompatibleDC(nullptr)) };
  okWin(SelectObject(dc, bmp));

  POINT pt{ 0, 0 };
  SIZE sz{ width, height };
  BLENDFUNCTION blend;
  blend.BlendOp             = AC_SRC_OVER;
  blend.BlendFlags          = 0;
  blend.SourceConstantAlpha = 0xFF;
  blend.AlphaFormat         = AC_SRC_ALPHA;
  okWin(UpdateLayeredWindow(window, nullptr, nullptr, &sz, dc, &pt, 0, &blend, ULW_ALPHA));

  okWin(DeleteDC(dc));
  okWin(DeleteObject(bmp));

  okWin(SetTimer(window, 0, 10, nullptr));

  return window;
}

void App::Main::closeSplash(void* handle) {
  auto window{ static_cast<HWND>(handle) };
  okWin(KillTimer(window, 0));
  okWin(DestroyWindow(window));
  okWin(UnregisterClass(className, GetModuleHandleW(nullptr)));
}

void App::Main::updateSplash(void* handle, uchar const* caption, f32 percent) {
  auto window{ static_cast<HWND>(handle) };

  // TODO update caption && progress

  MSG msg;
  BOOL result;
  while ((result = GetMessageW(&msg, window, 0, 0)) > 0) {
    if (msg.message == WM_TIMER) break;

    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  okWin(result != -1);
  ASSERT(result != 0);
}
