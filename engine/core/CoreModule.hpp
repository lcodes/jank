#pragma once

#include "core/CoreTypes.hpp"

class Module : NonCopyable {
protected:
  virtual void init() {}
  virtual void term() {}
};

