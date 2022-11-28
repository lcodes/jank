#pragma once

#include "core/CoreTypes.hpp"

namespace Audio::OpenAL {

void init();
void term();
void sync();

class Buffer {
  u32 id;

public:
  Buffer();
  ~Buffer();
};

class Source {
  u32 id;
  f32 gain;
  f32 pitch;
  bool loop;

public:
  Source();
  ~Source();
};

} // namespace OpenAL
