#include "gpu/opengl/OpenGL.hpp"

namespace Gpu::OpenGL {

void init() {
  device = new Device();
}

void term() {
  delete device;
}

} // namespace Gpu::OpenGL

