#pragma once

#include "core/Core.hpp"

#ifndef GPU_HAS_VULKAN
# define GPU_HAS_VULKAN (PLATFORM_ANDROID || PLATFORM_LINUX || PLATFORM_WINDOWS)
#endif

#ifndef GPU_HAS_D3D12
# define GPU_HAS_D3D12 PLATFORM_WINDOWS
#endif

#ifndef GPU_HAS_D3D11
# define GPU_HAS_D3D11 PLATFORM_WINDOWS
#endif

#ifndef GPU_HAS_METAL
# define GPU_HAS_METAL PLATFORM_APPLE
#endif

#ifndef GPU_HAS_OPENGL
# define GPU_HAS_OPENGL PLATFORM_DESKTOP
#endif

#ifndef GPU_HAS_GLES
# define GPU_HAS_GLES PLATFORM_MOBILE
#endif

#define GPU_NUM_BACKENDS ( \
  GPU_HAS_VULKAN + \
  GPU_HAS_D3D12  + \
  GPU_HAS_D3D11  + \
  GPU_HAS_METAL  + \
  GPU_HAS_OPENGL + \
  GPU_HAS_GLES     \
)

#if GPU_NUM_BACKENDS == 0
# error "No GPU backend"
#endif

#define GPU_STATIC (GPU_NUM_BACKENDS == 1)

namespace App {
class Window;
}

namespace Gpu {

constexpr u32 cpuFrames{ 1 }; // Logic -> Render
constexpr u32 gpuFrames{ 2 }; // Logic -> Render -> GPU

extern u32 cpuIndex;
extern u32 gpuIndex;

enum class Backend : u8 {
  Auto = 0x00,
#if GPU_HAS_VULKAN
  Vulkan = 0x01,
#endif
#if GPU_HAS_D3D12
  D3D12 = 0x02,
#endif
#if GPU_HAS_D3D11
  D3D11 = 0x03,
#endif
#if GPU_HAS_METAL
  Metal = 0x04,
#endif
#if GPU_HAS_GLES || GPU_HAS_OPENGL
  OpenGL = 0x05
#endif
};

void init(Backend backend);
void term();

void run();
void renderKick();
void renderSync();

#define GPU_FN_DYNAMIC(ret, name, ...)  virtual ret name(__VA_ARGS__) = 0
#define GPU_IMP_DYNAMIC(ret, name, ...) ret name(__VA_ARGS__) override
#define GPU_FN_STATIC(ret, name, ...)   ret name(__VA_ARGS__)

#if GPU_STATIC
# define GPU_FN      GPU_FN_STATIC
# define GPU_IMP     GPU_FN_STATIC
# define GPU_VIRTUAL
#else
# define GPU_FN      GPU_FN_DYNAMIC
# define GPU_IMP     GPU_IMP_DYNAMIC
# define GPU_VIRTUAL virtual
#endif

class Viewport : NonCopyable {
protected:
  Viewport() = default;
  GPU_VIRTUAL ~Viewport() = default;

public:
  f32 width{ 1920 };
  f32 height{ 1080 };

  GPU_FN(void, present);

  GPU_FN(void, resize, u32 width, u32 height);
};

class Device : NonCopyable {
protected:
  Device() = default;

public:
  GPU_VIRTUAL ~Device() = default;

  GPU_FN(Viewport*, createViewport, App::Window const* window);

  // Support
  // - Parallel Command Buffers
  // - Tessellation Shaders
  // - Compute Shaders

  // States
  // - Sampler
  // - Rasterizer
  // - DepthStencil
  // - Blend

  // Data
  // - InputLayout
  // - OutputLayout

  // Resources
  // - Buffer (Vertex / Index / Uniform / Storage / Feedback)
  // - Texture (1D, 2D, 3D, 1DArray, 2DArray, Cube / ...)
  // - Targets (Render / DepthStencil)

  // Programs
  // - Vertex / Hull / Domain / Geometry / Pixel / Compute shaders
  // - PipelineLayout
  // - Graphics / Compute pipelines
  // - Resource Views (CPU & GPU)

  // Queries

  // Sync
  // - Fence
  // - Semaphore
  // - WaitIdle
};

class CommandBuffer : NonCopyable {
  // Recording
  GPU_FN(void, end);
  GPU_FN(void, reset);

  // Clearing
  GPU_FN(void, clearRenderTarget);
  GPU_FN(void, clearDepthStencil);
  GPU_FN(void, clearStorage);
  GPU_FN(void, discard);

  // Reading / Writing
  GPU_FN(void, copy);
  GPU_FN(void, copyBuffer);
  GPU_FN(void, copyTexture);

  // Pipeline State
  GPU_FN(void, setPipelineState);
  GPU_FN(void, setTopology);
  GPU_FN(void, setVertexBuffers);
  GPU_FN(void, setIndexBuffer);
  // pushConstants()
  // descriptorSets()



  // beginQuery()
  // endQuery()

  GPU_FN(void, draw);
  GPU_FN(void, drawIndexed);
  GPU_FN(void, dispatch);

  GPU_FN(void, execute);
  GPU_FN(void, executeIndirect);
};

extern Device* device;

} // namespace Gpu

