#pragma once

#include "core/CoreTypes.hpp"

namespace Render {

class Light : NonCopyable {
  enum class Type : u8 {
    Directional,
    Point,
    Spot
  };

  // From transform components
  // - Position
  // - Direction

  // Color
  // Intensity

  // Profile

  Type type : 2;
};

class LightParams : NonCopyable {

};

} // namespace Render

