#include "app/App.hpp"
#include "app/GfxTest.hpp"
#include "net/NetSocket.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <AL/al.h>
#include <AL/alc.h>

#include <GLES3/gl3.h>

#include <EGL/egl.h>

#include <stdio.h>

#include <exception>

class HTML5OpenGL : public OpenGL {
public:
  EGLDisplay display;
  EGLContext context;
  EGLSurface surface;

  f64 currentTime;
  f64 deltaTime;

  void present() override {
    eglSwapBuffers(display, surface);
  }

  void clearCurrent() override {
    auto result{ eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) };
    ASSERT(result);
  }

  void makeCurrent() override {
    auto result{ eglMakeCurrent(display, surface, surface, context)};
    ASSERT(result);
  }

  f64 getDeltaTime() override {
    return deltaTime;
  }
};

static HTML5OpenGL gl;

static EGLint const configAttrs[] = {EGL_RED_SIZE,       8,
                                     EGL_GREEN_SIZE,     8,
                                     EGL_BLUE_SIZE,      8,
                                     EGL_ALPHA_SIZE,     EGL_DONT_CARE,
                                     EGL_DEPTH_SIZE,     24,
                                     EGL_STENCIL_SIZE,   8,
                                     EGL_SAMPLE_BUFFERS, 0,
                                     EGL_NONE,           EGL_NONE};

static EGLint const contextAttrs[] = {EGL_CONTEXT_CLIENT_VERSION, 3,
                                      EGL_NONE,                   EGL_NONE};

static char const* alErrorStr(ALenum e) {
#define E(x) case AL_##x: return #x
  switch (e) {
  default: return "UNKNOWN_ERROR";
  case AL_NO_ERROR: return nullptr;
  E(INVALID_NAME);
  E(INVALID_ENUM);
  E(INVALID_VALUE);
  E(INVALID_OPERATION);
  E(OUT_OF_MEMORY);
  }
#undef E
}

static void alCheckImpl(char const* file, uint32_t line) {
  if (auto e{alGetError()}) {
    printf("%s (%s:%u)\n", alErrorStr(e), file, line);
    exit(EXIT_FAILURE);
  }
}

#define alCheck() alCheckImpl(__FILE__, __LINE__)

class InputDevice {
  // name
  // type (keyboard, mouse, gamepad, remote)
  // mouseNumButtons
  // gamepadButtonsMask
  // hasMotion
};

struct Keyboard {
  static constexpr u32 numKeys = 128;

  usize keys[numKeys / sizeof(usize)];
};

union Point {
  u32 v[2];
  struct {
    u32 x;
    u32 y;
  };
};

struct Mouse {
  static constexpr u8 ButtonLeft   { 1 << 0 };
  static constexpr u8 ButtonRight  { 1 << 1 };
  static constexpr u8 ButtonMiddle { 1 << 2 };
  static constexpr u8 Button4      { 1 << 4 };
  static constexpr u8 Button5      { 1 << 5 };
  static constexpr u8 Button6      { 1 << 6 };
  static constexpr u8 Button7      { 1 << 7 };

  Point pos;
  f32 wheelV;
  f32 wheelH;
  u8 buttons;
};

struct Gamepad {
  static constexpr u32 DPadLeft     { 1 << 0 };
  static constexpr u32 DPadRight    { 1 << 1 };
  static constexpr u32 DPadTop      { 1 << 2 };
  static constexpr u32 DPadBottom   { 1 << 3 };
  static constexpr u32 ButtonLeft   { 1 << 4 };
  static constexpr u32 ButtonRight  { 1 << 5 };
  static constexpr u32 ButtonTop    { 1 << 6 };
  static constexpr u32 ButtonBottom { 1 << 7 };
  static constexpr u32 StickLeft    { 1 << 8 };
  static constexpr u32 StickRight   { 1 << 9 };
  static constexpr u32 TriggerLeft  { 1 << 10 };
  static constexpr u32 TriggerRight { 1 << 11 };
  static constexpr u32 Start        { 1 << 12 };
  static constexpr u32 Select       { 1 << 13 };
  static constexpr u32 Home         { 1 << 14 };
  static constexpr u32 Menu         { 1 << 15 };

  using Axis = f32;

  union Stick {
    Axis axes[2];
    struct {
      Axis x;
      Axis y;
    };
  };

  union {
    Stick sticks[2];
    struct {
      Stick stickLeft;
      Stick stickRight;
    };
  };
  union {
    Axis shoulders[2];
    struct {
      Axis shoulderLeft;
      Axis shoulderRight;
    };
  };
  u32 buttons;
};

constexpr u32 maxInputDevices = 8;

static InputDevice inputDevices[maxInputDevices];
static Gamepad     gamepads    [maxInputDevices];

static bool supportsGamepadAPI;

static EM_BOOL jank_html5_onGamepadConnected(int, EmscriptenGamepadEvent const* event UNUSED, void*) {
  printf("gamepad connected\n");
  return EM_TRUE;
}

static EM_BOOL jank_html5_onGamepadDisconnected(int, EmscriptenGamepadEvent const* event UNUSED, void*) {
  printf("gamepad disconnected\n");
  return EM_TRUE;
}

static void pollGamepads() {
  auto result{ emscripten_sample_gamepad_data() };
  ASSERT_EX(result == EMSCRIPTEN_RESULT_SUCCESS);

  auto numGamepads{ emscripten_get_num_gamepads() };
  ASSERT_EX(result != EMSCRIPTEN_RESULT_NOT_SUPPORTED);


}

DECL_LOG_SOURCE(Input, Warn);

EM_JS(void, setLocalStorage, (char const* key, char const* value), {
  localStorage.setItem(UTF8ToString(key), UTF8ToString(value));
});

EM_JS(char*, getLocalStorage, (char const* key, i32* outSize), {
  const item = localStorage.getItem(UTF8ToString(key));
  if (!item) {
      setValue(outSize, 0, "i32");
      return null;
  }
  const size = lengthBytesUTF8(item);
  const buf = _malloc(size);
  stringToUTF8(item, buf, size);
  setValue(outSize, size, "i32");
  return buf;
});

#include "imgui.h"

static void saveImGuiSettings() {
  auto& io{ ImGui::GetIO() };
  if (io.WantSaveIniSettings) {
    auto settings{ ImGui::SaveIniSettingsToMemory() };
    LOG(App, Trace, "Saving ImGui Settings (%d bytes)", strlen(settings));
    setLocalStorage("imgui", settings);
    io.WantSaveIniSettings = false;
  }
}

void jank_imgui_init() {
  auto& io{ ImGui::GetIO() };
  io.IniFilename = nullptr;
  io.LogFilename = nullptr;

  i32 settingsSize;
  if (auto settings{ getLocalStorage("imgui", &settingsSize) }) {
    LOG(App, Trace, "Loading ImGui Settings (%d bytes)", settingsSize);
    ImGui::LoadIniSettingsFromMemory(settings, settingsSize);
    free(settings);
  }

  EM_ASM({
    window["jankCanvasStyle"] = document.querySelector("#canvas").style;
    getPermission("clipboard-write", "jankCanWriteClipboard", () => {});

    function getPermission(name, prop, prompt) {
      window[prop] = false; // TODO clipboard is still new and janky

      /*try {
        navigator.permissions.query({name}).then(result => {
          result.onchange = changed;
          changed();

          function changed() {
            console.log("STATE CHANGED", result.state, navigator.clipboard);
            switch (result.state) {
            case "granted": window[prop] = true; break;
            case "denied":  window[prop] = false; break;
            case "prompt":  prompt(); break;
            }
          }
        });
      }
      catch {
        console.log("Permission not supported: ", prop);
      }*/
    }
  });
}

void jank_imgui_newFrame() {
  saveImGuiSettings();
}

EM_JS(void, setCursor, (u32 id), {
  let name;
  switch (id) {
  case 0: name = "default";     break;
  case 1: name = "text";        break;
  case 2: name = "move";        break;
  case 3: name = "ew-resize";   break;
  case 4: name = "ns-resize";   break;
  case 5: name = "nesw-resize"; break;
  case 6: name = "nwse-resize"; break;
  case 7: name = "pointer";     break;
  }
  window["jankCanvasStyle"].cursor = name;
});

static u32 currentCursor = UINT32_MAX;

void jank_imgui_setCursor(ImGuiMouseCursor cursor) {
  u32 id;
  switch (cursor) {
  case ImGuiMouseCursor_Arrow:      id = 0; break;
  case ImGuiMouseCursor_TextInput:  id = 1; break;
  case ImGuiMouseCursor_ResizeAll:  id = 2; break;
  case ImGuiMouseCursor_ResizeEW:   id = 3; break;
  case ImGuiMouseCursor_ResizeNS:   id = 4; break;
  case ImGuiMouseCursor_ResizeNESW: id = 5; break;
  case ImGuiMouseCursor_ResizeNWSE: id = 6; break;
  case ImGuiMouseCursor_Hand:       id = 7; break;
  default: ASSERT(0); UNREACHABLE;
  }
  if (id != currentCursor) {
    setCursor(id);
    currentCursor = id;
  }
}

EM_JS(void, setClipboardText, (char const* text), {
  if (window["jankCanWriteClipboard"]) {
    navigator.clipboard.writeText(UTF8ToString(text));
  }
});

char const* jank_imgui_getClipboardText(void*) {
  ASSERT(0, "TODO");
  return nullptr;
}

void jank_imgui_setClipboardText(void*, char const* text) {
  setClipboardText(text);
}

static EM_BOOL jank_html5_onKeyDown(int, EmscriptenKeyboardEvent const* event, void*) {
  LOG(Input, Info, "KeyDown: %s", event->key);
  return EM_TRUE;
}

static EM_BOOL jank_html5_onKeyUp(int, EmscriptenKeyboardEvent const* event, void*) {
  LOG(Input, Info, "KeyUp: %s", event->key);
  return EM_TRUE;
}

static bool pointerLocked;
static f32 mouseX, mouseY;

static EM_BOOL jank_html5_onMouseDown(int, EmscriptenMouseEvent const* event, void*) {
  LOG(Input, Info, "MouseDown: %u", event->button);
#if 0 // TODO this is janky, better to keep the pointer lock and draw a software cursor
  if (!ImGui::IsAnyMouseDown()) {
    auto result{ emscripten_request_pointerlock("#canvas", false) };
    pointerLocked = result == EMSCRIPTEN_RESULT_SUCCESS;
    mouseX = static_cast<f32>(event->targetX);
    mouseY = static_cast<f32>(event->targetY);
  }
  #endif
  ImGui::GetIO().MouseDown[event->button] = true;
  return EM_TRUE;
}

static EM_BOOL jank_html5_onMouseUp(int, EmscriptenMouseEvent const* event, void*) {
  LOG(Input, Info, "MouseUp: %u", event->button);
  ImGui::GetIO().MouseDown[event->button] = false;
  if (!ImGui::IsAnyMouseDown() && pointerLocked) {
    emscripten_exit_pointerlock();
    pointerLocked = false;
  }
  return EM_TRUE;
}

static EM_BOOL jank_html5_onMouseMove(int, EmscriptenMouseEvent const* event, void*) {
  LOG(Input, Info, "MouseMove: %u %u", event->targetX, event->targetY);
  if (pointerLocked) {
    ImGui::GetIO().MousePos = { mouseX += event->movementX, mouseY += event->movementY };
  }
  else {
    ImGui::GetIO().MousePos = { static_cast<f32>(event->targetX), static_cast<f32>(event->targetY) };
  }
  return EM_TRUE;
}

static EM_BOOL jank_html5_onMouseEnter(int, EmscriptenMouseEvent const*, void*) {
  LOG(Input, Info, "MouseEnter");
  return EM_TRUE;
}

static EM_BOOL jank_html5_onMouseLeave(int, EmscriptenMouseEvent const*, void*) {
  LOG(Input, Info, "MouseLeave");
  ImGui::GetIO().MousePos = { -FLT_MAX, -FLT_MAX };
  return EM_TRUE;
}

static EM_BOOL jank_html5_onWheel(int, EmscriptenWheelEvent const* event, void*) {
  // ASSERT_EX(event->deltaMode == DOM_DELTA_PIXEL);
  LOG(Input, Warn, "Wheel %u %lf %lf", event->deltaMode, event->deltaX, event->deltaY);
  auto& io{ ImGui::GetIO() };
  io.MouseWheel  -= event->deltaY / 100;
  io.MouseWheelH += event->deltaX / 100;
  return EM_TRUE;
}

static EM_BOOL jank_html5_onResize(int, EmscriptenUiEvent const* event, void*) {
  LOG(Input, Info, "Resize %u %u", event->documentBodyClientWidth, event->documentBodyClientHeight);
  return EM_TRUE;
}

// TODO orientation lock
static EM_BOOL jank_html5_onOrientationChange(int, EmscriptenOrientationChangeEvent const* event, void*) {
  LOG(Input, Info, "Orientation %u", event->orientationIndex);
  return EM_TRUE;
}

static EM_BOOL jank_html5_onWebGLContextLost(int, void const*, void*) {
  return EM_TRUE;
}

static EM_BOOL jank_html5_onWebGLContextRestored(int, void const*, void*) {
  return EM_TRUE;
}

static const char* jank_html5_onBeforeUnload(int, void const*, void*) {
  LOG(App, Info, "BeforeUnload");
  saveImGuiSettings();

  // FIXME: causes a context error in the refreshed page?!
  /*if (gl.context) {
    auto result{ eglDestroyContext(gl.display, gl.context) };
    ASSERT(result, "eglDestroyContext failed");
  }
  if (gl.surface) {
    auto result{ eglDestroySurface(gl.display, gl.surface) };
    ASSERT(result, "eglDestroySurface failed");
  }
  if (gl.display) {
    auto result{ eglTerminate(gl.display) };
    ASSERT(result, "eglTerminate failed");
  }*/
  return nullptr;
}

static void loop() {
  if (supportsGamepadAPI) pollGamepads();

  auto time{ emscripten_performance_now() };
  gl.deltaTime = (time - gl.currentTime) / 1000;
  gl.currentTime = time;
  renderMain(&gl);
}

#ifdef __EMSCRIPTEN_PTHREADS__
void* testMain(void*) {
  LOG(App, Warn, "HELLO WORKER");
  return nullptr;
}
#include <pthread.h>
#endif

int main() {
#ifdef __EMSCRIPTEN_PTHREADS__
  pthread_t t;
  pthread_create(&t, nullptr, &testMain, nullptr);
#endif

  {
#define window EMSCRIPTEN_EVENT_TARGET_WINDOW
    emscripten_set_keydown_callback(window, nullptr, true, jank_html5_onKeyDown);
    emscripten_set_keyup_callback  (window, nullptr, true, jank_html5_onKeyUp);

    emscripten_set_mousedown_callback ("#canvas", nullptr, true, jank_html5_onMouseDown);
    emscripten_set_mouseup_callback   ("#canvas", nullptr, true, jank_html5_onMouseUp);
    emscripten_set_mousemove_callback ("#canvas", nullptr, true, jank_html5_onMouseMove);
    emscripten_set_mouseenter_callback("#canvas", nullptr, true, jank_html5_onMouseEnter);
    emscripten_set_mouseleave_callback("#canvas", nullptr, true, jank_html5_onMouseLeave);

    emscripten_set_wheel_callback("#canvas", nullptr, true, jank_html5_onWheel);

    // touch events

    supportsGamepadAPI = emscripten_sample_gamepad_data() != EMSCRIPTEN_RESULT_NOT_SUPPORTED;

    emscripten_set_gamepadconnected_callback   (nullptr, true, jank_html5_onGamepadConnected);
    emscripten_set_gamepaddisconnected_callback(nullptr, true, jank_html5_onGamepadDisconnected);

    emscripten_set_resize_callback(window, nullptr, true, jank_html5_onResize);

    emscripten_set_orientationchange_callback(nullptr, true, jank_html5_onOrientationChange);

    // fullscreenchange (request_fullscreen/exit_fullscreen)
    // pointerlockchange (request_pointerlock/exit_pointerlock)
    // visibilitychange

    // batterycharging
    // batterylevelchange

    // vibration API

    emscripten_set_webglcontextlost_callback    (window, nullptr, true, jank_html5_onWebGLContextLost);
    emscripten_set_webglcontextrestored_callback(window, nullptr, true, jank_html5_onWebGLContextRestored);

    emscripten_set_beforeunload_callback(nullptr, jank_html5_onBeforeUnload);
#undef window
  }

  {
    gl.width = 1920;
    gl.height = 1024;

    EM_ASM({
      let canvas = document.getElementById("canvas");
      canvas.width = 1920;
      canvas.height = 1024;
    });

    gl.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (gl.display == EGL_NO_DISPLAY) {
      printf("eglGetDisplay\n");
      return -1;
    }

    if (!eglInitialize(gl.display, nullptr, nullptr)) {
      printf("eglInitialize\n");
      return -1;
    }

    EGLint configCount;
    EGLConfig config;
    if (!eglChooseConfig(gl.display, configAttrs, &config, 1, &configCount)) {
      printf("eglChooseConfig\n");
      return -1;
    }

    gl.surface = eglCreateWindowSurface(gl.display, config, 0, nullptr);
    if (gl.surface == EGL_NO_SURFACE) {
      printf("eglCreateWindowSurface\n");
      return -1;
    }

    gl.context = eglCreateContext(gl.display, config, EGL_NO_CONTEXT, contextAttrs);
    if (gl.context == EGL_NO_CONTEXT) {
      printf("eglCreateContext\n");
      return -1;
    }
  }

  {
#if !PLATFORM_HTML5
    auto hasEnumeration{alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT")};
    if (hasEnumeration) {
      printf("has enum\n");
    }
#endif

    auto device{alcOpenDevice(nullptr)};
    if (!device) {
      printf("alcOpenDevice\n");
      return -1;
    }

    auto context{alcCreateContext(device, nullptr)};
    if (!device) {
      alcCloseDevice(device);
      printf("alcCreateContext\n");
      return -1;
    }

    if (!alcMakeContextCurrent(context)) {
      alcDestroyContext(context);
      alcCloseDevice(device);
      printf("alcMakeContextCurrent\n");
      return -1;
    }

    ALuint source;
    alGenSources(1, &source);
    alCheck();

    alSourcef(source, AL_PITCH, 1);           alCheck();
    alSourcef(source, AL_GAIN, 1);            alCheck();
    alSource3f(source, AL_POSITION, 0, 0, 0); alCheck();
    alSource3f(source, AL_VELOCITY, 0, 0, 0); alCheck();
    alSourcei(source, AL_LOOPING, AL_FALSE);  alCheck();

    auto fp{fopen("demo/sounds/test.wav", "rb")};
    if (!fp) {
      printf("fopen\n");
      return -1;
    }
    fseek(fp, 0, SEEK_END);
    auto sz{ftell(fp)};
    fseek(fp, 0, SEEK_SET);

    auto data{static_cast<uint8_t*>(malloc(sz))};
    fread(data, sz, 1, fp);
    fclose(fp);

    auto offset{22}; // ignore RIFF/fmt/format headers

    auto channels{data[offset++]};
    channels |= data[offset++] << 8;
    printf("Channels: %u\n", channels);

    uint32_t freq{data[offset++]};
    freq |= data[offset++] << 8;
    freq |= data[offset++] << 16;
    freq |= data[offset++] << 24;
    printf("Frequency: %u\n", freq);

    offset += 6; // ignore block size & bps

    auto bits{data[offset++]};
    bits |= data[offset++] << 8;
    printf("Bits: %u\n", bits);

    auto format{0};
    switch (bits) {
    case 8:
      switch (channels) {
      case 1: format = AL_FORMAT_MONO8; break;
      case 2: format = AL_FORMAT_STEREO8; break;
      }
      break;
    case 16:
      switch (channels) {
      case 1: format = AL_FORMAT_MONO16; break;
      case 2: format = AL_FORMAT_STEREO16; break;
      }
      break;
    }

    offset += 8; // ignore the data chunk

    ALuint buffer;
    alGenBuffers(1, &buffer);                                   alCheck();
    alBufferData(buffer, format, data+offset, sz-offset, freq); alCheck();
    alSourceQueueBuffers(source, 1, &buffer);                   alCheck();
    alSourcePlay(source);                                       alCheck();
  }

  // Socket sock{};
  // sock.TEST();

  printf("Aw Yeah!\n");
  emscripten_set_main_loop(loop, 0, true);
  printf("Oh no!\n");
  return 0;
}
