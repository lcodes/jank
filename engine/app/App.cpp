#include "app/App.hpp"

#include "audio/AudioCore.hpp"
#include "gpu/GpuCore.hpp"

bool App::running;

AppWindow* App::mainWindow;

void App::init() {
  ASSERT(!running, "App initializing while running");

  // TODO show splash

  // CONFIG
  // - EnvVars
  // - CmdLine
  // - ConsoleVars
  // - Files
  //   - Install Dir
  //   - User Dir

  // MEMORY
  // - Heap Alloc
  // - Linear Alloc (+Thread)
  // - Arena Alloc (+Thread)
  // - Block Alloc (+Thread)

  // RTTR
  // - Map configs to data
  // - Serialize/Deserialize
  // - Localization
  // - Editor UIs
  // - Networking Data

  // OBJECTS
  // - Entity
  // - Preset
  // - Scene
  // - Curve
  // - Timeline
  // - DataTable
  // - ParticleSystem
  // - Movie
  // - Model
  // - Skeleton
  // - Animation
  // - Texture
  // - LightMap
  // - LightProbe
  // - Material
  // - Shader
  // - Font
  // - CollisionMesh
  // - PhysicsMaterial
  // - Timeline
  // - Sound
  // - Mixer
  // - PostProcessParams
  // - ...

  // LOCALIZATION
  // - Text
  // - Resources
  //   - Images
  //   - Audio

  // TASK
  // - Job Threads
  // - I/O Threads

  Audio::init();
  Gpu::init(Gpu::Backend::OpenGL); // TODO from config

  // TODO: init modules
  // - config
  // - memory
  // - jobs
  // - init jobs
  //   - localization
  //   - assets
  //   - window -> gpu -> shader -> render -> imgui
  //   - physics
  //   - animation
  //   - audio
  //   - input
  //   - networking

  // TODO: run splash while init jobs run

  // TODO: hide splash
  //       show window

  // TODO: kick main job

  running = true;
}

void App::term() {
  running = false;

  // TODO: wait for jobs to finish

  // TODO: term modules
}

void App::gainFocus() {
  LOG(App, Info, "GAIN FOCUS");
}

void App::loseFocus() {
  LOG(App, Info, "LOSE FOCUS");
}

void App::pause() {

}

void App::resume() {

}

void App::lowMemory() {

}

void App::mainJob() {
  while (running) {
    // input jobs
    {
      // - sync render frame
      // - poll input devices
      // - receive network packet
    }

    // setup frame
    // - frame number
    // - frame time

    // logic jobs
    {
      // - physics
      // - animation
      // - transforms
      // - cameras
    }

    // finish frame
    // - end of frame jobs

    // output jobs
    {
      // - submit render frame
      // - sync audio nodes
      // - send network packet
    }
  }
}

