#include "app/Main.hpp"
#include "app/Window.hpp"

#include "core/CoreTask.hpp"

#include "app/Input.hpp"
#include "audio/AudioCore.hpp"
#include "gpu/GpuCore.hpp"
#include "physics/Physics2D.hpp"
#include "ui/UICore.hpp"
#include "world/Transform.hpp"

#if BUILD_EDITOR
# include "asset/Library.hpp"
#endif

namespace App {

// The main job drives the engine for the duration of the process.
// It is kicked during initialization and loops until termination.
static Task::Job     mainJobHandle;
static Task::Counter mainJobCounter;

bool Main::running;

Window* Main::mainWindow;

Window::Window() : mainWindow(Main::getMainWindow() == nullptr) {}

void Window::setup() {
  viewport = Gpu::device->createViewport(this);
}


// Engine Initialization
// ----------------------------------------------------------------------------

void Main::init() {
  ASSERT(!running, "App initializing while running");

  auto splash{ showSplash() };

  auto wait = [splash](Task::Counter& counter) {
    while (!counter.done()) {
      updateSplash(splash, USTR("Hi"), 0.f);
    }
  };

  Task::init();

  // Phase 1: Systems not requiring a window or swap chain
  // --------------------------------------------------------------------------
  {
    // GPU jobs must complete before any window is created. 
    // CPU jobs run parallel to both GPU and window initialization.
    Task::Job gpuJobs[]{
      { []() { Gpu::init(Gpu::Backend::OpenGL); } }, // TODO backend from config
    };
    Task::Job cpuJobs[]{
      { Audio::init },
      { Input::init },
      { Physics2D::init },
      { UI::init },
    #if BUILD_EDITOR
      { Asset::Library::init }
    #endif
    };

    Task::Counter gpuCounter;
    Task::Counter cpuCounter;
    Task::run(gpuCounter, gpuJobs, countof(gpuJobs));
    Task::run(cpuCounter, cpuJobs, countof(cpuJobs));

    wait(gpuCounter);

#if HAS_MULTIPLE_WINDOWS
    Window::Params windowParams;
    // TODO from config
    windowParams.position = { Window::defaultPosition, Window::defaultPosition };
    windowParams.size = { 1920, 1080 };
    windowParams.title = L"Jank"sv; // TODO from product name
    windowParams.parent = nullptr;
    windowParams.flags = Window::DisplayFlags::None;
    mainWindow = Window::create(windowParams);
    // TODO other windows are restored from ImGui viewports?
#else
    // TODO init main window && swap chain
#endif

    wait(cpuCounter);
  }

  // Phase 2: Systems requiring a window and swap chain
  // --------------------------------------------------------------------------
  {
    Task::Job jobs[]{
      { UI::initRender }
    };
    Task::Counter counter;
    Task::run(counter, jobs, countof(jobs));
    wait(counter);
  }

#if BUILD_EDITOR
  // Initialize editor
  // - ImGui Docking
  // - ImGui windows
  // - Scene viewport
  // - Link server
#else
  // Initialize launch scene
#endif

  // Phase 3: Engine Ready
  // --------------------------------------------------------------------------
#if HAS_MULTIPLE_WINDOWS
  mainWindow->show();
#endif

  closeSplash(splash);

  running = true;
  mainJobHandle = { mainJob };
  Task::run(mainJobCounter, mainJobHandle);
}


// Engine Termination
// ----------------------------------------------------------------------------

static void run(Task::Job* jobs, u32 count) {
  Task::Counter counter;
  Task::run(counter, jobs, count);
  Task::waitAsync(counter);
}

void Main::term() {
  running = false;

  Task::waitAsync(mainJobCounter);

  {
    Task::Job jobs[]{
      { UI::termRender }
    };
    run(jobs, countof(jobs));
  }

  {
    Task::Job jobs[]{
      { Audio::term },
      { Gpu::term },
      { Input::term },
      { Physics2D::term },
      { UI::term },
    #if BUILD_EDITOR
      { Asset::Library::term }
    #endif
    };
    run(jobs, countof(jobs));
  }

  Task::term();
}


// Engine Simulation
// ----------------------------------------------------------------------------

void temp() {}

void Main::mainJob() {
  while (running) {
    // Input Jobs
    // ------------------------------------------------------------------------
    {
      Task::Job jobs[]{
        { Input::run }
        // - receive network packet
      };
      Task::Counter counter;
      Task::run(counter, jobs, countof(jobs));
      Task::wait(counter);
    }

    Gpu::renderSync();

    // setup frame
    // - frame number
    // - frame time

    // Logic Jobs
    // ------------------------------------------------------------------------
    {
      // TODO split in phases
      // - pre physics
      // - physics
      // - post physics
      // - transforms
      // - post transform
      // - visibility
      Task::Job jobs[]{
        { temp }
        // - physics
        // - animation
        // - transforms
        // - cameras
      };
      Task::Counter counter;
      Task::run(counter, jobs, countof(jobs));
      Task::wait(counter);
    }

    // finish frame
    // - end of frame jobs

    // Output Jobs
    // ------------------------------------------------------------------------
    {
      // GPU jobs must complete before the render frame is kicked.
      // CPU jobs run parallel to the GPU sync and render frame jobs.
      Task::Job gpuJobs[]{
        { Gpu::run },
        { UI::run },
      };
      Task::Job cpuJobs[]{
        { Audio::run },
        // - send network packet
      };

      Task::Counter gpuCounter;
      Task::Counter cpuCounter;
      Task::run(gpuCounter, gpuJobs, countof(gpuJobs));
      Task::run(cpuCounter, cpuJobs, countof(cpuJobs));

      Task::wait(gpuCounter);
      Gpu::renderKick();

      Task::wait(cpuCounter);
    }
  }

  Gpu::renderSync(); // TODO multiple frames
}


// Process Event Handling
// ----------------------------------------------------------------------------

void Main::gainFocus() {
  LOG(Main, Info, "GAIN FOCUS");
}

void Main::loseFocus() {
  LOG(Main, Info, "LOSE FOCUS");
}

void Main::pause() {

}

void Main::resume() {

}

void Main::lowMemory() {

}

} // namespace App

