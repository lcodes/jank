#include "Core.hpp"

#include <emscripten/emscripten.h>
#include <emscripten/html5.h>

#include <AL/al.h>
#include <AL/alc.h>

#include <GLES3/gl3.h>

#include <EGL/egl.h>

#include <stdio.h>

#include <exception>

#include "NetSocket.hpp"

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

static EM_BOOL onGamepadConnected(int, EmscriptenGamepadEvent const* event, void*) {
  printf("gamepad connected\n");
  return true;
}

static EM_BOOL onGamepadDisconnected(int, EmscriptenGamepadEvent const* event, void*) {
  printf("gamepad disconnected\n");
  return true;
}

void loop() {

}

DECL_LOG_SOURCE(App, Debug);

int main() {
  LOG(App, Info, "Hello HTML5 %u!", 42);
  LOG(App, Debug, "WHEE");
  {
    emscripten_set_gamepadconnected_callback(nullptr, true, onGamepadConnected);
    emscripten_set_gamepaddisconnected_callback(nullptr, true, onGamepadDisconnected);
  }

  {
    auto display{eglGetDisplay(EGL_DEFAULT_DISPLAY)};
    if (display == EGL_NO_DISPLAY) {
      printf("eglGetDisplay\n");
      return -1;
    }

    if (!eglInitialize(display, nullptr, nullptr)) {
      printf("eglInitialize\n");
      return -1;
    }

    EGLint configCount;
    EGLConfig config;
    if (!eglChooseConfig(display, configAttrs, &config, 1, &configCount)) {
      printf("eglChooseConfig\n");
      return -1;
    }

    auto surface{eglCreateWindowSurface(display, config, 0, nullptr)};
    if (surface == EGL_NO_SURFACE) {
      printf("eglCreateWindowSurface\n");
      return -1;
    }

    auto context{eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttrs)};
    if (context == EGL_NO_CONTEXT) {
      printf("eglCreateContext\n");
      return -1;
    }

    if (!eglMakeCurrent(display, surface, surface, context)) {
      printf("eglMakeCurrent\n");
      return -1;
    }

    glClearColor(.2, .3, .4, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();

    eglSwapBuffers(display, surface);
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

  Socket sock{};
  sock.TEST();

  printf("Aw Yeah!\n");
  emscripten_set_main_loop(loop, 0, true);
  printf("Oh no!\n");
  return 0;
}
