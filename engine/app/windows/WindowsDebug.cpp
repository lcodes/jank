#include "core/Core.hpp"
#include "core/CoreString.hpp"
#include "core/CoreThread.hpp"

#include <Windows.h>

#if BUILD_DEVELOPMENT

// Debug Thread
// ----------------------------------------------------------------------------

static bool debugThreadRunning;
static Thread debugThread;

static void debugThreadMain() {
  struct DBWinBuffer {
    u32 processId;
    u8 data[4096 - sizeof(u32)];
  };

  Thread::setName(L"Debug");

  auto processId{ GetCurrentProcessId() };
  auto bufReady { okWin(CreateEventW(nullptr, false, true, L"DBWIN_BUFFER_READY")) };
  auto dataReady{ okWin(CreateEventW(nullptr, false, false, L"DBWIN_DATA_READY")) };
  auto fileMap  { okWin(CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr,
                                           PAGE_READWRITE, 0, sizeof(DBWinBuffer),
                                           L"DBWIN_BUFFER")) };
  auto buf{ reinterpret_cast<DBWinBuffer*>(okWin(MapViewOfFile(fileMap, FILE_MAP_READ,
                                                               0, 0, 0))) };

  while (debugThreadRunning) {
    switch (WaitForSingleObject(dataReady, 250)) {
    case WAIT_OBJECT_0:
      if (buf->processId == processId) {
        LOG(Debug, Debug, "%s", buf->data);
      }
      okWin(SetEvent(bufReady));
      break;
    case WAIT_ABANDONED: UNREACHABLE;
    case WAIT_TIMEOUT:   break;
    case WAIT_FAILED:    failWin();
    }
  }

  okWin(UnmapViewOfFile(buf));
  okWin(CloseHandle(fileMap));
  okWin(CloseHandle(dataReady));
  okWin(CloseHandle(bufReady));
}

void initWindowsDebugThread() {
  Thread::Params params{ 0x200, Thread::Priority::Lowest };

  debugThreadRunning = true;
  debugThread = Thread(params, debugThreadMain);
}

void termWindowsDebugThread() {
  debugThreadRunning = false;
  debugThread.join();
}


// Console Window
// ----------------------------------------------------------------------------

// TODO thread to process console events?

struct EnumFontParams {
  UINT family;
  bool found;
};

static int CALLBACK enumFontProc(LOGFONTW    const* logFont    UNUSED,
                                 TEXTMETRICW const* textMetric UNUSED,
                                 DWORD              fontType,
                                 LPARAM             lParam) noexcept
{
  auto params{ reinterpret_cast<EnumFontParams*>(lParam) };
  params->found = true;
  if (fontType & TRUETYPE_FONTTYPE) {
    params->family = TMPF_TRUETYPE;
    return false;
  }
  return true;
}

void initWindowsConsole() {
  HWND wnd;

  // Create the console window when using the Windows subsystem.
  if (!(wnd = GetConsoleWindow())) {
    okWin(AllocConsole());
    okWin(SetConsoleTitleW(L"Jank"));

    FILE* dummy;
    okC(_wfreopen_s(&dummy, L"CONOUT$", L"wb", stdout));
    okC(_wfreopen_s(&dummy, L"CONERR$", L"wb", stderr));
    okC(_wfreopen_s(&dummy, L"CONIN$",  L"rb", stdin));

    wnd = GetConsoleWindow();
  }

  auto handle{ GetStdHandle(STD_OUTPUT_HANDLE) };

  // Set the input and output codepages.
  okWin(SetConsoleCP(CP_UTF8));
  okWin(SetConsoleOutputCP(CP_UTF8));

  okWin(SetConsoleCtrlHandler(nullptr, true));
  //okWin(SetConsoleMode(handle,
                       //ENABLE_ECHO_INPUT |
                       //ENABLE_EXTENDED_FLAGS |
                       //ENABLE_INSERT_MODE |
                       //ENABLE_LINE_INPUT |
                       //ENABLE_MOUSE_INPUT |
                       //ENABLE_QUICK_EDIT_MODE |
                       //ENABLE_WINDOW_INPUT |
                       //ENABLE_VIRTUAL_TERMINAL_INPUT |
                       //ENABLE_PROCESSED_OUTPUT |
                       //ENABLE_VIRTUAL_TERMINAL_PROCESSING
  //));

  // Configure the maximum number of text lines in the console.
  CONSOLE_SCREEN_BUFFER_INFO info;
  okWin(GetConsoleScreenBufferInfo(handle, &info));

  info.dwSize.Y = 1024;
  okWin(SetConsoleScreenBufferSize(handle, info.dwSize));

  // Configure the console display font if its available.
  constexpr auto fontName{ L"Source Code Pro"sv };
  constexpr usize fontNameSize{ (fontName.size() + 1) * sizeof(wchar_t) };

  LOGFONTW lf;
  lf.lfCharSet        = DEFAULT_CHARSET;
  lf.lfPitchAndFamily = 0;
  memcpy(lf.lfFaceName, fontName.data(), fontNameSize);

  auto dc{ GetDC(wnd) };
  EnumFontParams params{};
  EnumFontFamiliesExW(dc, &lf, enumFontProc, reinterpret_cast<LPARAM>(&params), 0);
  ReleaseDC(wnd, dc);

  if (params.found) {
    CONSOLE_FONT_INFOEX font;
    font.cbSize     = sizeof(font);
    font.nFont      = 0;
    font.dwFontSize = { 0, 16 };
    font.FontFamily = params.family;
    font.FontWeight = FW_NORMAL;
    memcpy(font.FaceName, fontName.data(), fontNameSize);
    okWin(SetCurrentConsoleFontEx(handle, false, &font));
  }
}

#endif // BUILD_DEVELOPMENT
