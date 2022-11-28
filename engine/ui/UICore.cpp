#include "ui/UICore.hpp"

#include "gpu/GpuCore.hpp"

struct FrameData {
  //Gpu::IndexBuffer*  indexBuffer;
  //Gpu::VertexBuffer* vertexBuffer;
  void* indexData;
  void* vertexData;
};

//static Gpu::GraphicsPipeline* pipeline;
//static Gpu::Texture* fontTexture;

static FrameData frameData[Gpu::cpuFrames];

void UI::init() {
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  auto& io{ ImGui::GetIO() };

  // Platform init
}

void UI::term() {
  ImGui::DestroyContext();
}

void UI::initRender() {
  auto& io{ ImGui::GetIO() };

  // Renderer init

  // Create pipeline

  // Create font texture

  // Create frameData buffers
}

void UI::termRender() {

}

void UI::run() {
  auto& io{ ImGui::GetIO() };

  // wait for last frame's render data to be uploaded to the GPU
  // -OR-
  // memcpy it after render, and upload async

  // TODO sync state
  // - display size
  // - delta time
  // - cursor

  //ImGui::NewFrame();

  // run through UI callbacks
}

void UI::render() {
  //ImGui::Render();

  //auto data{ ImGui::GetDrawData() };

  // upload GPU data

  // run through viewports/draw cmds
  // present viewports (except primary)
}

