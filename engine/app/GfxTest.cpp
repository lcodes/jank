#include "GfxTest.hpp"
#include "App.hpp"

#include "imgui.h"

#include <pthread.h>
#include <string>

#if PLATFORM_ANDROID
# include <GLES3/gl3.h>
# include <GLES3/gl3ext.h>
#elif PLATFORM_MACOS
# include <OpenGL/gl3.h>
# include <OpenGL/gl3ext.h>
#elif PLATFORM_IPHONE
# include <OpenGLES/ES3/gl.h>
# include <OpenGLES/ES3/glext.h>
#else
# error Don't know how to include OpenGL for the target platform
#endif

DECL_LOG_SOURCE(GfxTest, Info);


// OpenGL Helpers
// -----------------------------------------------------------------------------

static void glCheck() {
  if (auto e{glGetError()}; e != GL_NO_ERROR) {
    char const* str;
    switch (e) {
#define E(x) case GL_##x: str = #x; break
      E(INVALID_ENUM);
      E(INVALID_VALUE);
      E(INVALID_OPERATION);
      E(OUT_OF_MEMORY);
      E(INVALID_FRAMEBUFFER_OPERATION);
#undef E
    default: ASSERT(0, "Unknown GL Error: %04x", e);
    }
    ASSERT(0, str);
  }
}

static void glCheckFramebuffer() {
  if (auto e{glCheckFramebufferStatus(GL_FRAMEBUFFER)}; e != GL_FRAMEBUFFER_COMPLETE) {
    char const* str;
    switch (e) {
#define E(x) case GL_FRAMEBUFFER_##x: str = #x; break;
      E(INCOMPLETE_ATTACHMENT);
      //E(INCOMPLETE_DIMENSIONS);
      E(INCOMPLETE_MISSING_ATTACHMENT);
      E(UNSUPPORTED);
    default: ASSERT(0, "Unknown GL Framebuffer Error: %04x", e);
#undef E
    }
    ASSERT(0, str);
  }
}

using get_t = decltype(glGetShaderiv);
using get_info_log_t = decltype(glGetShaderInfoLog);

template<get_t get, get_info_log_t getInfoLog, i32 status>
static void glCheckInfoLog(u32 object) {
  i32 success;
  get(object, status, &success);
  if (!success) {
    i32 size;
    get(object, GL_INFO_LOG_LENGTH, &size);
    std::string s;
    s.resize(size);
    getInfoLog(object, size, nullptr, s.data());
    LOG(App, Error, "Failed:\n%.*s", size, s.data());
    ASSERT(0);
  }
}

#define glCheckShader(x)  glCheckInfoLog<glGetShaderiv, glGetShaderInfoLog, GL_COMPILE_STATUS>(x)
#define glCheckProgram(x) glCheckInfoLog<glGetProgramiv, glGetProgramInfoLog, GL_LINK_STATUS>(x)


// OpenGL + ImGui test
// -----------------------------------------------------------------------------

void* renderMain(void* arg) {
#if PLATFORM_APPLE
  pthread_setname_np("Render");
#elif PLATFORM_POSIX
  pthread_setname_np(pthread_self(), "Render");
#else

#endif

  auto gl{reinterpret_cast<OpenGL*>(arg)};
  gl->makeCurrent();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui::StyleColorsDark();

  LOG(App, Info, "OpenGL Version %s", glGetString(GL_VERSION));
  LOG(App, Info, "OpenGL Vendor: %s", glGetString(GL_VENDOR));
  LOG(App, Info, "OpenGL Renderer: %s", glGetString(GL_RENDERER));

  u32 fbo, tex;
  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gl->width, gl->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
  glBindTexture(GL_TEXTURE_2D, tex);
  glCheckFramebuffer();

  glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glCheck();

  // Triangle init
  float vertices[]{
    -.5f, -.5f, 0,
    .5f, -.5f, 0,
    0, .5f, 0
  };

  u32 vaoTriangle;
  glGenVertexArrays(1, &vaoTriangle);
  glBindVertexArray(vaoTriangle);

  u32 vboTriangle;
  glGenBuffers(1, &vboTriangle);
  glBindBuffer(GL_ARRAY_BUFFER, vboTriangle);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  glVertexAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(float), nullptr);
  glEnableVertexAttribArray(0);
  glBindVertexArray(0);
  glCheck();

#if PLATFORM_MACOS
# define GLSL_VERSION "#version 410 core\n"
#else
# define GLSL_VERSION "#version 300 es\n" "precision mediump float;\n"
#endif

  auto triangleVertexSource =
  GLSL_VERSION
  "layout (location = 0) in vec3 pos;\n"
  "void main() {\n"
  "  gl_Position = vec4(pos, 1);\n"
  "}\n";

  auto triangleFragmentSource =
  GLSL_VERSION
  "layout (location = 0) out vec4 col;\n"
  "void main() {\n"
  "  col = vec4(.4, .5, .6, 1);\n"
  "}\n";

  auto triangleVert{glCreateShader(GL_VERTEX_SHADER)};
  auto triangleFrag{glCreateShader(GL_FRAGMENT_SHADER)};
  glShaderSource(triangleVert, 1, &triangleVertexSource,   nullptr);
  glShaderSource(triangleFrag, 1, &triangleFragmentSource, nullptr);
  glCompileShader(triangleVert);
  glCompileShader(triangleFrag);
  glCheckShader(triangleVert);
  glCheckShader(triangleFrag);

  auto triangleProg{glCreateProgram()};
  glAttachShader(triangleProg, triangleVert);
  glAttachShader(triangleProg, triangleFrag);
  glLinkProgram(triangleProg);
  glCheckProgram(triangleProg);

  // ImGui Init
  auto& io{ImGui::GetIO()};

  auto vertexSource =
  GLSL_VERSION
  "layout (location = 0) in vec2 pos;\n"
  "layout (location = 1) in vec2 uv;\n"
  "layout (location = 2) in vec4 col;\n"
  "uniform mat4 proj;\n"
  "out vec2 frag_uv;\n"
  "out vec4 frag_col;\n"
  "void main() {\n"
  "  gl_Position = proj * vec4(pos, 0, 1);\n"
  "  frag_uv = uv;\n"
  "  frag_col = col;\n"
  "}\n";

  auto fragmentSource =
  GLSL_VERSION
  "in vec2 frag_uv;\n"
  "in vec4 frag_col;\n"
  "uniform sampler2D tex;\n"
  "layout (location = 0) out vec4 out_col;\n"
  "void main() {\n"
  "  out_col = frag_col * texture(tex, frag_uv);\n"
  "}\n";

  auto vert{glCreateShader(GL_VERTEX_SHADER)};
  auto frag{glCreateShader(GL_FRAGMENT_SHADER)};
  glShaderSource(vert, 1, &vertexSource, nullptr);
  glShaderSource(frag, 1, &fragmentSource, nullptr);
  glCompileShader(vert);
  glCompileShader(frag);
  glCheckShader(vert);
  glCheckShader(frag);

  auto prog{glCreateProgram()};
  glAttachShader(prog, vert);
  glAttachShader(prog, frag);
  glLinkProgram(prog);
  glCheckProgram(prog);

  auto texLoc{glGetUniformLocation(prog, "tex")};
  auto projLoc{glGetUniformLocation(prog, "proj")};

  u32 vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  u32 bufs[2];
  glGenBuffers(2, bufs);
  glBindBuffer(GL_ARRAY_BUFFER, bufs[0]);
  glVertexAttribPointer(0, 2, GL_FLOAT,         false, sizeof(ImDrawVert), reinterpret_cast<void*>(IM_OFFSETOF(ImDrawVert, pos)));
  glVertexAttribPointer(1, 2, GL_FLOAT,         false, sizeof(ImDrawVert), reinterpret_cast<void*>(IM_OFFSETOF(ImDrawVert, uv)));
  glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, true,  sizeof(ImDrawVert), reinterpret_cast<void*>(IM_OFFSETOF(ImDrawVert, col)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glBindVertexArray(0);
  glCheck();

  // ImGui init
  GLuint fontTexture;
  glGenTextures(1, &fontTexture);
  glBindTexture(GL_TEXTURE_2D, fontTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  {
    u8* pixels;
    i32 width, height;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  }
  glCheck();
  io.Fonts->TexID = reinterpret_cast<ImTextureID>(static_cast<usize>(fontTexture));

  // ImGui state
  float r{0}, g{0}, b{0};

  glDisable(GL_DEPTH_TEST);
  glDepthMask(false);

  while (true) {
    r += 0.001;
    g += 0.0025;
    b += 0.0005;
    if (r > 1) r = 0;
    if (g > 1) g = 0;
    if (b > 1) b = 0;

    io.DisplaySize = ImVec2{gl->width, gl->height};
    io.DisplayFramebufferScale = ImVec2{gl->dpi, gl->dpi};

    io.DeltaTime = gl->getDeltaTime();
#if 1
    io.DeltaTime = 0.016;
#endif
    // TODO update cursor

    bool show = true;
    ImGui::NewFrame();
    ImGui::ShowDemoWindow(&show);

    ImGui::Begin("Hello");
    ImGui::Text("%.3f ms | %.1f FPS", 1000.f / io.Framerate, io.Framerate);
    ImGui::End();

    ImGui::Render();

    auto drawData{ImGui::GetDrawData()};
    // TODO check totalVtxCount

    auto offset{drawData->DisplayPos};
    auto scale {drawData->FramebufferScale};
    auto width {static_cast<GLsizei>(drawData->DisplaySize.x * scale.x)};
    auto height{static_cast<GLsizei>(drawData->DisplaySize.y * scale.y)};

#if 0
    gl->renderReady.wait();
    LOG(GfxTest, Info, "RENDER READY");
#if PLATFORM_ANDROID
    gl->makeCurrent();
#endif
#endif

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearColor(r, g, b, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glCheck();

    glViewport(0, 0, width, height);

    glUseProgram(triangleProg);
    glBindVertexArray(vaoTriangle);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glCheck();

    {
      auto l{drawData->DisplayPos.x};
      auto r{drawData->DisplaySize.x + l};
      auto t{drawData->DisplayPos.y};
      auto b{drawData->DisplaySize.y + t};
      f32 proj[4][4]{
        {2.f/(r-l), 0, 0, 0},
        {0, 2.f/(t-b), 0, 0},
        {0, 0, -1, 0},
        {(r+l)/(l-r), (t+b)/(b-t), 0, 1}
      };
      glUseProgram(prog);
      glUniform1i(texLoc, 0);
      glUniformMatrix4fv(projLoc, 1, false, &proj[0][0]);
      glCheck();
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,         bufs[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufs[1]);
#if 0
    glBufferData(GL_ARRAY_BUFFER,         drawData->TotalVtxCount * sizeof(ImDrawVert), NULL, GL_STREAM_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, drawData->TotalIdxCount * sizeof(ImDrawIdx),  NULL, GL_STREAM_DRAW);
    auto vtxOffset{0u};
    auto idxOffset{0u};
    for (auto n{0}; n < drawData->CmdListsCount; n++) {
      auto cmd{drawData->CmdLists[n]};
      auto vtx{&cmd->VtxBuffer};
      auto idx{&cmd->IdxBuffer};
      auto vtxSize{vtx->size_in_bytes()};
      auto idxSize{idx->size_in_bytes()};
      glBufferSubData(GL_ARRAY_BUFFER,         vtxOffset, vtxSize, vtx->Data);
      glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, idxOffset, idxSize, idx->Data);
      vtxOffset += vtxSize;
      idxOffset += idxSize;
    }
    glCheck();
    ASSERT(vtxOffset / sizeof(ImDrawVert) == drawData->TotalVtxCount);
    ASSERT(idxOffset / sizeof(ImDrawIdx)  == drawData->TotalIdxCount);
#endif

    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glEnable(GL_SCISSOR_TEST);
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

#if 0
    vtxOffset = 0;
    idxOffset = 0;
#else
    int vtxOffset = 0, idxOffset = 0;
#endif
#if 1 || !PLATFORM_MACOS
    for (auto n{0}; n < drawData->CmdListsCount; n++) {
      auto cmd{drawData->CmdLists[n]};
      glBufferData(GL_ARRAY_BUFFER, cmd->VtxBuffer.Size * sizeof(ImDrawVert), cmd->VtxBuffer.Data, GL_STREAM_DRAW);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, cmd->IdxBuffer.Size * sizeof(ImDrawIdx), cmd->IdxBuffer.Data, GL_STREAM_DRAW);
      for (auto i{0}; i < cmd->CmdBuffer.Size; i++) {
        auto draw{&cmd->CmdBuffer[i]};
        ASSERT(!draw->UserCallback, "TODO");

        auto clipX{(draw->ClipRect.x - offset.x) * scale.x};
        auto clipY{(draw->ClipRect.y - offset.y) * scale.y};
        auto clipZ{(draw->ClipRect.z - offset.x) * scale.x};
        auto clipW{(draw->ClipRect.w - offset.y) * scale.y};

        if (false && clipX < width && clipY < height && clipZ >= 0 && clipW >= 0) {
          glScissor(clipX, height - clipW, clipZ - clipX, clipW - clipY);
          glBindTexture(GL_TEXTURE_2D, static_cast<u32>(reinterpret_cast<usize>(draw->TextureId)));
          glDrawElements/*BaseVertex*/(GL_TRIANGLES, draw->ElemCount,
                                       sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                       reinterpret_cast<void*>(draw->IdxOffset * sizeof(ImDrawIdx) + idxOffset)/*,
                                                                                                                draw->VtxOffset + vtxOffset*/);
          glCheck();
          LOG(GfxTest, Info, "DRAW %d.%d", n, i);
        }
      }

#if 0
      vtxOffset += cmd->VtxBuffer.size();
      idxOffset += cmd->IdxBuffer.size_in_bytes();
#endif
    }
#endif

    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glBindVertexArray(0);
    glUseProgram(0);

    //glBindFramebuffer(GL_FRAMEBUFFER, 0);
    //glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, 0, 0, 800, 600, 800, 600, GL_COLOR_BUFFER_BIT, GL_NEAREST);

#if 0
#if PLATFORM_ANDROID
    gl->clearCurrent();
#endif
    gl->presentReady.set();
#else
    gl->present();
#endif
  }

  //glDeleteTextures(1, &fontTexture);
  io.Fonts->TexID = 0;

  return nullptr;
}
