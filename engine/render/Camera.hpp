#pragma once

#include "core/CoreTypes.hpp"

namespace Render {

class Camera : NonCopyable {
  enum class Projection : u8 {
    Perspective,
    Orthographic
  };

  Projection projection : 1;

  // RenderTarget

  // Culling Mask
};

class CameraParams : NonCopyable {

};

} // namespace Render
