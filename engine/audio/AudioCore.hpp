#pragma once

#include "core/CoreTypes.hpp"

#if PLATFORM_APPLE
# include "audio/coreaudio/CoreAudio.hpp"
#elif PLATFORM_WINDOWS
# include "audio/xaudio2/XAudio2.hpp"
#elif PLATFORM_ANDROID || PLATFORM_LINUX || PLATFORM_HTML5
# include "audio/openal/OpenAL.hpp"
#else
# error "No audio backend"
#endif

namespace Audio {

#if PLATFORM_APPLE
using namespace CoreAudio;
#elif PLATFORM_WINDOWS
using namespace XAudio2;
#elif PLATFORM_ANDROID || PLATFORM_LINUX || PLATFORM_HTML5
using namespace OpenAL;
#else
# error "No audio backend"
#endif

} // namespace Audio

