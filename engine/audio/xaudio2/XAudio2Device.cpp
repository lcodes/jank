#include "audio/xaudio2/XAudio2.hpp"

#include "app/windows/WindowsMain.hpp"

#pragma warning(push, 0)
#include <xaudio2.h>
#include <x3daudio.h>
#pragma warning(pop)

DECL_LOG_SOURCE(XAudio2, Info);

namespace Audio::XAudio2 {

static HMODULE dll;

static IXAudio2* device;
static IXAudio2MasteringVoice* master;

static X3DAUDIO_HANDLE x3d;
static X3DAUDIO_LISTENER listener;

#if BUILD_DEVELOPMENT
// TODO isDebug cvar
constexpr bool isDebug = BUILD_DEBUG;
#endif

void init() {
  uchar const* dllName;

  auto flags{ 0u };
  auto os{ App::Windows::Main::getOSVersion() };
  if (os >= App::Windows::Version::Win10) {
    dllName = L"XAudio2_9.dll";
  }
  else if (os >= App::Windows::Version::Win8) {
    dllName = L"XAudio2_8.dll";
  }
#if BUILD_DEVELOPMENT
  else if (isDebug) {
    flags |= XAUDIO2_DEBUG_ENGINE;
    dllName = L"XAudioD2_7.dll";
  }
#endif
  else {
    dllName = L"XAudio2_7.dll";
  }

  dll = okWin(LoadLibraryExW(dllName, nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32));

  XAUDIO2_PROCESSOR processor{ XAUDIO2_DEFAULT_PROCESSOR };

#define getProc(fn) static_cast<decltype(&fn)>(reinterpret_cast<void*>(GetProcAddress(dll, #fn)))
  if (auto createWithVersion{ getProc(XAudio2CreateWithVersionInfo) }) {
    okCom(createWithVersion(&device, flags, processor, NTDDI_VERSION));
  }
  else if (auto create{ getProc(XAudio2Create) }) {
    okCom(create(&device, flags, processor));
  }
  else {
    // TODO better
    exit(EXIT_FAILURE);
  }

#if BUILD_DEVELOPMENT
  if (isDebug) {
    XAUDIO2_DEBUG_CONFIGURATION debug{};
    debug.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
    debug.BreakMask = XAUDIO2_LOG_ERRORS;
    device->SetDebugConfiguration(&debug);
  }
#endif

  DWORD channelMask;
  okCom(device->CreateMasteringVoice(&master));
  okCom(master->GetChannelMask(&channelMask));

  if (auto initX3D{ getProc(X3DAudioInitialize) }) {
    okCom(initX3D(channelMask, X3DAUDIO_SPEED_OF_SOUND, x3d));
  }
  else {
    // TODO better
    exit(EXIT_FAILURE);
  }
#undef getProc
}

void term() {
  if (master) {
    master->DestroyVoice();
    master = nullptr;
  }

  if (device) {
    device->Release();
    device = nullptr;
  }

  if (dll) {
    okWin(FreeLibrary(dll));
  }
}

void run() {
  okCom(device->CommitChanges(XAUDIO2_COMMIT_ALL));
}

Source::Source() {
  WAVEFORMATEX fmt;
  fmt.wFormatTag = WAVE_FORMAT_PCM; // TODO WAVEFORMATEXTENSIBLE
  fmt.nChannels = 1; // TODO
  fmt.wBitsPerSample = 16;
  fmt.nSamplesPerSec = 22050;
  fmt.nBlockAlign = fmt.nChannels * (fmt.wBitsPerSample / 8u);
  fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
  fmt.cbSize = 0;
  okCom(device->CreateSourceVoice(&voice, &fmt)); // TODO more params
}

Source::~Source() {
  voice->DestroyVoice();
}

} // namespace Audio::XAudio2
