/**
 * Reference:
 * https://www.openal.org/documentation/OpenAL_Programmers_Guide.pdf
 */
#include "audio/openal/OpenAL.hpp"

#include "core/Core.hpp"

#include <AL/al.h>
#include <AL/alc.h>

DECL_LOG_SOURCE(OpenAL, Info);


// Utilities
// ----------------------------------------------------------------------------

#if BUILD_DEVELOPMENT
# define alCheck() alCheckImpl(__FILE__, __LINE__)
# define alcCheck() alcCheckImpl(__FILE__, __LINE__)

static void alCheckImpl(char const* file, u32 line) {
  if (auto e{ alGetError() }) {
    char const* str;
    switch (e) {
  #define E(x) case AL_##x: str = #x; break
      E(INVALID_NAME);
      E(INVALID_ENUM);
      E(INVALID_VALUE);
      E(INVALID_OPERATION);
      E(OUT_OF_MEMORY);
  #undef E
    default: ASSERT(0, "Unknown AL Error: 0x%04x", e); UNREACHABLE;
    }
    assertFailure(file, line, str);
  }
}

static void alcCheckImpl(char const* file, u32 line) {
  if (auto e{ alcGetError() }) {
    char const* str;
    switch (e) {
    #define E(x) case ALC_##x: str = #x; break
      E(INVALID_DEVICE);
      E(INVALID_CONTEXT);
      E(INVALID_ENUM);
      E(INVALID_VALUE);
      E(OUT_OF_MEMORY);
    #undef E
    default: ASSERT(0, "Unknown ALC Error: 0x%04x", e); UNREACHABLE;
    }
    assertFailure(file, line, str);
  }
}
#else
# define alCheck()
# define alcCheck()
#endif


// Device
// ----------------------------------------------------------------------------

namespace Audio::OpenAL {

static ALCdevice*  device;
static ALCcontext* context;

void init() {
  {
    ALCint major, minor;
    alcGetIntegerv(nullptr, ALC_MAJOR_VERSION, sizeof(major), &major);
    alcGetIntegerv(nullptr, ALC_MINOR_VERSION, sizeof(minor), &minor);
    alcCheck();
    LOG(OpenAL, Info, "ALC version: %d.%d", major, minor);

  // TODO use it?
    auto defaultDevice{ alcGetString(nullptr, ALC_DEFAULT_DEVICE_SPECIFIER) };
    auto extensions{ alcGetString(nullptr, ALC_EXTENSIONS) };
    LOG(OpenAL, Info, "ALC default device: %s", defaultDevice);
    LOG(OpenAL, Info, "ALC extensions: %s", extensions);

    // TODO extensions
  }

#if !PLATFORM_HTML5
  auto hasEnumeration{ alcIsExtensionPresent(nullptr, "ALC_ENUMERATION_EXT") };
  if (hasEnumeration) {
    // TODO enumerate
  }
#endif

  device = alcOpenDevice(nullptr); // TODO devicename
  ASSERT(device); // TODO real check

  context = alcCreateContext(device, nullptr); // TODO attrlist
  alcCheck();

  {
    auto vendor{ alGetString(AL_VENDOR) };
    auto version{ alGetString(AL_VERSION) };
    auto renderer{ alGetString(AL_RENDERER) };
    auto extensions{ alGetString(AL_EXTENSIONS) };
    alCheck();
    LOG(OpenAL, Info, "AL vendor: %s", vendor);
    LOG(OpenAL, Info, "AL version: %s", version);
    LOG(OpenAL, Info, "AL renderer: %s", renderer);
    LOG(OpenAL, Info, "AL extensions: %s", extensions);

    // TODO extensions
  }
}

void term() {
  if (context) {
    alcDestroyContext(context);
    alcCheck();
    context = nullptr;
  }

  if (device) {
    alcCloseDevice(device);
    alcCheck();
    device = nullptr;
  }
}

void sync() {
  alcSuspendContext(context);
  alcCheck();

  // TODO update everything

  alcProcessContext(context);
  alcCheck();
}


// Buffers
// ----------------------------------------------------------------------------

Buffer::Buffer() {
  alGenBuffers(1, &id);
  alCheck();
}

Buffer::~Buffer() {
  if (id) alDeleteBuffers(1, &id);
}


// Sources
// ----------------------------------------------------------------------------

Source::Source() {
  alGenSources(1, &id);
  alCheck();
}

Source::~Source() {
  if (id) alDeleteSources(1, &id);
}

} // namespace Audio::OpenAL
