#pragma once

#include "gpu/GpuCore.hpp"

#if PLATFORM_ANDROID || PLATFORM_HTML5
# include <GLES3/gl3.h>
# include <GLES3/gl3ext.h>
#elif PLATFORM_LINUX
# include <GL/gl.h>
# include <GL/glcorearb.h>
# include <GL/glext.h>
#elif PLATFORM_MACOS
# include <OpenGL/gl3.h>
# include <OpenGL/gl3ext.h>
#elif PLATFORM_IPHONE
# include <OpenGLES/ES3/gl.h>
# include <OpenGLES/ES3/glext.h>
#elif PLATFORM_WINDOWS
# pragma warning(push, 0)
# include <Windows.h>
# include <gl/GL.h>
# include <GL/glcorearb.h>
# include <GL/glext.h>
# pragma warning(pop)
#else
# error "Unknown OpenGL platform"
#endif

DECL_LOG_SOURCE(OpenGL, Info);

namespace Gpu::OpenGL {

class Device : public Gpu::Device {
public:
  Device() = default;
  ~Device() = default;

  GPU_IMP(Gpu::Viewport*, createViewport, App::Window const* window);
};

class Viewport : public Gpu::Viewport {
public:
  void makeCurrent();
  void clearCurrent();

  void* getProcAddress(char const* name);
};

class Shader {
  u32 id;
};

class Pipeline {
  u32 id;
};

class Buffer {
  u32 id;
};

class Texture {
  u32 id;
};

class SamplerState {

};

class RasterizerState {
  i32 depthBias;
  f32 depthBiasClamp;
  f32 slopeScaledDepthBias;
  u32 sampleCount   : 4;
  bool ccw          : 1;
  bool depthClip    : 1;
  bool multisample  : 1;
  bool smoothLine   : 1;
  bool conservative : 1;
};

class DepthStencilState {
  bool depthEnable   : 1;
  bool stencilEnable : 1;
};

class BlendState {
  bool enable : 1;
};

} // namespace Gpu::OpenGL
