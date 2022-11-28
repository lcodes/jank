#pragma once

#include "core/CoreTypes.hpp"

struct IXAudio2SourceVoice;

namespace Audio::XAudio2 {

void init();
void term();
void run();

class Buffer {

};

class Source {
  IXAudio2SourceVoice* voice;

public:
  Source();
  ~Source();
};

} // namespace Audio::XAudio2

