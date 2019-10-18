#include "core/CoreTypes.hpp"

#if PLATFORM_MACOS

#include "imgui.h"

#import <AppKit/AppKit.h>

static ImVector<char> clipboardString;
static NSArray* clipboardArray;

static NSCursor* mouseCursors[ImGuiMouseCursor_COUNT];
static bool mouseCursorHidden;

@interface NSCursor()
+ (instancetype)_windowResizeNorthWestSouthEastCursor;
+ (instancetype)_windowResizeNorthEastSouthWestCursor;
+ (instancetype)_windowResizeNorthSouthCursor;
+ (instancetype)_windowResizeEastWestCursor;
@end

void initImGui() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  auto& io{ImGui::GetIO()};
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  // OSX
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
  io.BackendPlatformName = "macOS";

  // OpenGL
  io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

  constexpr i32 keyOffset{0x100 - 0xF700};
  io.KeyMap[ImGuiKey_Tab] = '\t';
  io.KeyMap[ImGuiKey_LeftArrow] = NSLeftArrowFunctionKey + keyOffset;
  io.KeyMap[ImGuiKey_RightArrow] = NSRightArrowFunctionKey + keyOffset;
  io.KeyMap[ImGuiKey_UpArrow] = NSUpArrowFunctionKey + keyOffset;
  io.KeyMap[ImGuiKey_DownArrow] = NSDownArrowFunctionKey + keyOffset;
  io.KeyMap[ImGuiKey_PageUp] = NSPageUpFunctionKey + keyOffset;
  io.KeyMap[ImGuiKey_PageDown] = NSPageDownFunctionKey + keyOffset;
  io.KeyMap[ImGuiKey_Home] = NSHomeFunctionKey + keyOffset;
  io.KeyMap[ImGuiKey_End] = NSEndFunctionKey + keyOffset;
  io.KeyMap[ImGuiKey_Insert] = NSInsertFunctionKey + keyOffset;
  io.KeyMap[ImGuiKey_Delete] = NSDeleteFunctionKey + keyOffset;
  io.KeyMap[ImGuiKey_Backspace] = 127;
  io.KeyMap[ImGuiKey_Space] = ' ';
  io.KeyMap[ImGuiKey_Enter] = '\r';
  io.KeyMap[ImGuiKey_Escape] = 27;
  io.KeyMap[ImGuiKey_KeyPadEnter] = '\r';
  io.KeyMap[ImGuiKey_A] = 'A';
  io.KeyMap[ImGuiKey_C] = 'C';
  io.KeyMap[ImGuiKey_V] = 'V';
  io.KeyMap[ImGuiKey_X] = 'X';
  io.KeyMap[ImGuiKey_Y] = 'Y';
  io.KeyMap[ImGuiKey_Z] = 'Z';

  mouseCursors[ImGuiMouseCursor_Arrow] = [NSCursor arrowCursor];
  mouseCursors[ImGuiMouseCursor_TextInput] = [NSCursor IBeamCursor];
  mouseCursors[ImGuiMouseCursor_ResizeAll] = [NSCursor closedHandCursor];
  mouseCursors[ImGuiMouseCursor_Hand] = [NSCursor pointingHandCursor];
  mouseCursors[ImGuiMouseCursor_ResizeNS] = [NSCursor resizeUpDownCursor];
  mouseCursors[ImGuiMouseCursor_ResizeEW] = [NSCursor resizeLeftRightCursor];
  mouseCursors[ImGuiMouseCursor_ResizeNESW] = [NSCursor _windowResizeNorthEastSouthWestCursor];
  mouseCursors[ImGuiMouseCursor_ResizeNWSE] = [NSCursor _windowResizeNorthWestSouthEastCursor];

  clipboardArray = [NSArray arrayWithObject:NSPasteboardTypeString];

  io.SetClipboardTextFn = [](void*, char const* str) {
    auto pasteboard{[NSPasteboard generalPasteboard]};
    [pasteboard declareTypes:clipboardArray owner:nil];
    [pasteboard setString:[NSString stringWithUTF8String:str] forType:NSPasteboardTypeString];
  };

  io.GetClipboardTextFn = [](void*) -> char const* {
    auto pasteboard{[NSPasteboard generalPasteboard]};
    auto available{[pasteboard availableTypeFromArray:clipboardArray]};
    if (![available isEqualToString:NSPasteboardTypeString]) return nullptr;

    auto str{[pasteboard stringForType:NSPasteboardTypeString]};
    if (!str) return nullptr;

    auto cstr{[str UTF8String]};
    auto size{strlen(cstr) + 1};
    clipboardString.resize(size);
    memcpy(clipboardString.Data, cstr, size);
    return clipboardString.Data;
  };
}

#endif
