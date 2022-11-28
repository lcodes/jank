#include "gpu/GpuCore.hpp"

#include "core/CoreTask.hpp"

void renderMain(); // TODO remove

namespace Gpu {

Device* device;

#if GPU_HAS_VULKAN
namespace Vulkan {
void init();
void term();
}
#endif

#if GPU_HAS_D3D12
namespace D3D12 {
void init();
void term();
}
#endif

#if GPU_HAS_D3D11
namespace D3D11 {
void init();
void term();
}
#endif

#if GPU_HAS_METAL
namespace Metal {
void init();
void term();
}
#endif

#if GPU_HAS_OPENGL || GPU_HAS_GLES
namespace OpenGL {
void init();
void term();
}
#endif

u32 cpuIndex;
u32 gpuIndex;

static Task::Job     renderJobs    [cpuFrames];
static Task::Counter renderCounters[cpuFrames];

static void renderJob();

void init(Backend backend) {
  for (auto& job : renderJobs) {
    job.fn = { renderJob };
  }

  // TODO handle Backend::Auto

  switch (backend) {
  #if GPU_HAS_VULKAN
  case Backend::Vulkan:
    Vulkan::init();
    break;
  #endif
  #if GPU_HAS_D3D12
  case Backend::D3D12:
    D3D12::init();
    break;
  #endif
  #if GPU_HAS_D3D11
  case Backend::D3D11:
    D3D11::init();
    break;
  #endif
  #if GPU_HAS_METAL
  case Backend::Metal:
    Metal::init();
    break;
  #endif
  #if GPU_HAS_OPENGL || GPU_HAS_GLES
  case Backend::OpenGL:
    OpenGL::init();
    break;
  #endif
  }
}

void term() {
  // TODO
}

void run() {
  // TODO copy frame data
}

void renderKick() {
  renderCounters[cpuIndex].reset();
  Task::run(renderCounters[cpuIndex], renderJobs[cpuIndex]);
}

void renderSync() {
  cpuIndex = (cpuIndex + 1) % cpuFrames;
  gpuIndex = (gpuIndex + 1) % gpuFrames;

  Task::wait(renderCounters[cpuIndex]);
}

static void renderJob() {
  renderMain();
}

} // namespace Gpu
