#pragma once

#include "core/CoreTypes.hpp"
#include "core/CoreString.hpp"

#if PLATFORM_DESKTOP 
# define HAS_MULTIPLE_WINDOWS  1
# define CAN_MOVE_SIZE_WINDOWS 1
#elif PLATFORM_MOBILE
# define HAS_MULTIPLE_WINDOWS  0
# define CAN_MOVE_SIZE_WINDOWS 0
#else
# error "Unknown platform category"
#endif

namespace Gpu {
class Viewport;
}

namespace App {

enum class FullscreenMode : u8 {
  Windowed,
  Borderless,
  Fullscreen
};

class Window : NonCopyable {
protected:
  Point position;
  Size size;

  Size minSize;
  Size maxSize;

  Gpu::Viewport* viewport;

  FullscreenMode fullscreenMode;
  bool mainWindow : 1;

  Window();

  void setup();

public:
  virtual ~Window() = default;

  bool isMainWindow() const { return mainWindow; }

  Point getPosition() const { return position; }
  Size getSize() const { return size; }

  Gpu::Viewport* getViewport() const { return viewport; }

#if CAN_MOVE_SIZE_WINDOWS
  FullscreenMode getFullscreen() const { return fullscreenMode; }
  virtual void setFullscreen(FullscreenMode mode) = 0;

  virtual void move(Point pt) = 0;
  virtual void resize(Size sz) = 0;

  void getMinMaxSize(Size* min, Size* max) const {
    *min = minSize;
    *max = maxSize;
  }
#endif

#if HAS_MULTIPLE_WINDOWS
  virtual void setTitle(UStringView title) = 0;

  virtual void show() = 0;

  virtual void focus() = 0;
  virtual void close() = 0;

  static constexpr i32 defaultPosition = INT32_MIN;

  // NOTE: Matches the values in ImGuiViewportFlags.
  enum DisplayFlags : u8 {
    None = 0,
    NoDecoration = 1 << 0,
    NoTaskBarIcon = 1 << 1,
    TopMost = 1 << 6
  };

  struct Params {
    Point        position;
    Size         size;
    UStringView  title;
    Window*      parent;
    DisplayFlags flags;
  };

  static Window* create(Params const& params);
#endif
};


// Monitor
// ----------------------------------------------------------------------------

struct Monitor {
  Point mainPos;
  Point workPos;
  Size mainSize;
  Size workSize;
  f32 dpiScale;
};

} // namespace App

