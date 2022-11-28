#include "app/Main.hpp"
#include "app/GfxTest.hpp"
#include "core/CoreString.hpp"

#include "rtm/qvvf.h"
#include "Box2D/Box2D.h"
#include "imgui.h"

#if BUILD_EDITOR
#include "Compressonator.h"
#endif

#if PLATFORM_ANDROID
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
# include <gl/GL.h>
# include <GL/glcorearb.h>
# include <GL/glext.h>
#elif PLATFORM_HTML5
# include <GLES3/gl3.h>
#else
# error "Don't know how to include OpenGL for the target platform"
#endif

#if PLATFORM_MACOS
# include <unistd.h>
#endif

#if PLATFORM_POSIX
# include <pthread.h>
#endif

#include <random>

DECL_LOG_SOURCE(Test, Info);

constexpr u16 glMakeVersion(i32 major, i32 minor) {
  ASSERT(major > 0 && minor >= 0 && major < 10 && minor < 10);
  return static_cast<u16>((major << 8) | minor);
}

constexpr u16 GL2_0 = glMakeVersion(2, 0);
constexpr u16 GL2_1 = glMakeVersion(2, 1);
constexpr u16 GL3_0 = glMakeVersion(3, 0);
constexpr u16 GL3_1 = glMakeVersion(3, 1);
constexpr u16 GL3_2 = glMakeVersion(3, 2);
constexpr u16 GL3_3 = glMakeVersion(3, 3);
constexpr u16 GL4_0 = glMakeVersion(4, 0);
constexpr u16 GL4_1 = glMakeVersion(4, 1);
constexpr u16 GL4_2 = glMakeVersion(4, 2);
constexpr u16 GL4_3 = glMakeVersion(4, 3);
constexpr u16 GL4_4 = glMakeVersion(4, 4);
constexpr u16 GL4_5 = glMakeVersion(4, 5);
constexpr u16 GL4_6 = glMakeVersion(4, 6);

constexpr u16 GLES2_0 = glMakeVersion(2, 0);
constexpr u16 GLES3_0 = glMakeVersion(3, 0);

void jank_imgui_init();
void jank_imgui_newFrame();
//void jank_imgui_setCursor(ImGuiMouseCursor);

char const* jank_imgui_getClipboardText(void*);
void jank_imgui_setClipboardText(void*, char const*);

#if !PLATFORM_HTML5 //&& !PLATFORM_WINDOWS
void jank_imgui_init() {}
void jank_imgui_newFrame() {}
void jank_imgui_setCursor(ImGuiMouseCursor) {}

char const* jank_imgui_getClipboardText(void*) { return ""; }
void jank_imgui_setClipboardText(void*, char const*) {}
#endif

#if BUILD_EDITOR
std::string testMeshImport(char const* filename,
                           Material** materials = nullptr,
                           u32* numMaterials = nullptr,
                           Skeleton* skeleton = nullptr,
                           Animation* animation = nullptr);
std::string testMeshImportSimple(char const* filename);
u32 importTextureHDR(char const* filename);
#endif


// Dynamic Link Setup
// -----------------------------------------------------------------------------

#if PLATFORM_LINUX || PLATFORM_WINDOWS

// TODO: looks like windows has GL1.1 and linux has GL1.4 we can directly link against
# if PLATFORM_WINDOWS
#  define GL1_2_PROCS \
  GL(TEXIMAGE3D, TexImage3D); \
  GL(TEXSUBIMAGE3D, TexSubImage3D)

#  define GL1_3_PROCS \
  GL(ACTIVETEXTURE, ActiveTexture); \
  GL(COMPRESSEDTEXIMAGE1D, CompressedTexImage1D); \
  GL(COMPRESSEDTEXIMAGE2D, CompressedTexImage2D); \
  GL(COMPRESSEDTEXIMAGE3D, CompressedTexImage3D); \
  GL(COMPRESSEDTEXSUBIMAGE1D, CompressedTexSubImage1D); \
  GL(COMPRESSEDTEXSUBIMAGE2D, CompressedTexSubImage2D); \
  GL(COMPRESSEDTEXSUBIMAGE3D, CompressedTexSubImage3D)

#  define GL1_4_PROCS \
  GL(BLENDEQUATION, BlendEquation)
# endif

# define GL1_5_PROCS \
  GL(BINDBUFFER, BindBuffer); \
  GL(BUFFERDATA, BufferData); \
  GL(BUFFERSUBDATA, BufferSubData); \
  GL(DELETEBUFFERS, DeleteBuffers); \
  GL(GENBUFFERS, GenBuffers)

# define GL2_0_ARB \
  GL(GENFRAMEBUFFERSARB, GenFramebuffersARB)

# define GL2_0_PROCS \
  GL(ATTACHSHADER, AttachShader); \
  GL(COMPILESHADER, CompileShader); \
  GL(CREATEPROGRAM, CreateProgram); \
  GL(CREATESHADER, CreateShader); \
  GL(DELETEPROGRAM, DeleteProgram); \
  GL(DELETESHADER, DeleteShader); \
  GL(DRAWBUFFERS, DrawBuffers); \
  GL(ENABLEVERTEXATTRIBARRAY, EnableVertexAttribArray); \
  GL(GETPROGRAMINFOLOG, GetProgramInfoLog); \
  GL(GETPROGRAMIV, GetProgramiv); \
  GL(GETSHADERINFOLOG, GetShaderInfoLog); \
  GL(GETSHADERIV, GetShaderiv); \
  GL(GETUNIFORMLOCATION, GetUniformLocation); \
  GL(LINKPROGRAM, LinkProgram); \
  GL(SHADERSOURCE, ShaderSource); \
  GL(UNIFORM1F, Uniform1f); \
  GL(UNIFORM1I, Uniform1i); \
  GL(UNIFORM2FV, Uniform2fv); \
  GL(UNIFORM3FV, Uniform3fv); \
  GL(UNIFORMMATRIX3FV, UniformMatrix3fv); \
  GL(UNIFORMMATRIX4FV, UniformMatrix4fv); \
  GL(USEPROGRAM, UseProgram); \
  GL(VERTEXATTRIBPOINTER, VertexAttribPointer)

# define GL3_0_PROCS \
  GL(BINDBUFFERRANGE, BindBufferRange); \
  GL(BINDRENDERBUFFER, BindRenderbuffer); \
  GL(BINDFRAMEBUFFER, BindFramebuffer); \
  GL(BINDVERTEXARRAY, BindVertexArray); \
  GL(BLITFRAMEBUFFER, BlitFramebuffer); \
  GL(CHECKFRAMEBUFFERSTATUS, CheckFramebufferStatus); \
  GL(DELETEFRAMEBUFFERS, DeleteFramebuffers); \
  GL(DELETERENDERBUFFERS, DeleteRenderbuffers); \
  GL(DELETEVERTEXARRAYS, DeleteVertexArrays); \
  GL(FRAMEBUFFERRENDERBUFFER, FramebufferRenderbuffer); \
  GL(FRAMEBUFFERTEXTURE2D, FramebufferTexture2D); \
  GL(GENERATEMIPMAP, GenerateMipmap); \
  GL(GENFRAMEBUFFERS, GenFramebuffers); \
  GL(GENRENDERBUFFERS, GenRenderbuffers); \
  GL(GENVERTEXARRAYS, GenVertexArrays); \
  GL(GETSTRINGI, GetStringi); \
  GL(RENDERBUFFERSTORAGE, RenderbufferStorage); \
  GL(VERTEXATTRIBIPOINTER, VertexAttribIPointer)

# define GL3_1_PROCS \
  GL(GETUNIFORMBLOCKINDEX, GetUniformBlockIndex); \
  GL(UNIFORMBLOCKBINDING, UniformBlockBinding)

# define GL3_2_PROCS \
  GL(DRAWELEMENTSBASEVERTEX, DrawElementsBaseVertex)

# define GL3_3_PROCS \
  GL(DELETESAMPLERS, DeleteSamplers); \
  GL(GENSAMPLERS, GenSamplers)

# define GL4_3_PROCS \
  GL(DEBUGMESSAGECALLBACK, DebugMessageCallback); \
  GL(DEBUGMESSAGEINSERT, DebugMessageInsert); \
  GL(OBJECTLABEL, ObjectLabel); \
  GL(OBJECTPTRLABEL, ObjectPtrLabel); \
  GL(POPDEBUGGROUP, PopDebugGroup); \
  GL(PUSHDEBUGGROUP, PushDebugGroup)

// TODO GL4

# define GL(type, name) static PFNGL##type##PROC gl##name
# if PLATFORM_WINDOWS
GL1_2_PROCS;
GL1_3_PROCS;
GL1_4_PROCS;
# endif
GL1_5_PROCS;
GL2_0_PROCS;
GL3_0_PROCS;
GL3_1_PROCS;
GL3_2_PROCS;
GL3_3_PROCS;
GL4_3_PROCS;
# undef GL
#endif

void __cdecl debugCallback(GLenum source, GLenum type, GLuint id,
                           GLenum severity, GLsizei length, GLchar const* message,
                           void const* userParam UNUSED)
{
  switch (severity) {
  case GL_DEBUG_SEVERITY_HIGH:
  case GL_DEBUG_SEVERITY_MEDIUM:
  case GL_DEBUG_SEVERITY_LOW: break;
  case GL_DEBUG_SEVERITY_NOTIFICATION: return;
  }
  LogLevel level;
  switch (type) {
  case GL_DEBUG_TYPE_OTHER: level = LogLevel::Info; break;
  case GL_DEBUG_TYPE_ERROR:
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: level = LogLevel::Error; break;
  case GL_DEBUG_TYPE_PERFORMANCE:
  case GL_DEBUG_TYPE_PORTABILITY:
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: level = LogLevel::Warn; break;
  case GL_DEBUG_TYPE_MARKER:
  case GL_DEBUG_TYPE_PUSH_GROUP:
  case GL_DEBUG_TYPE_POP_GROUP: return;
  default: ASSERT(0, "Unknown GL message type: 0x%04x", type); UNREACHABLE;
  }
  std::string_view sourceStr;
  switch (source) {
  case GL_DEBUG_SOURCE_API: sourceStr = "API"sv; break;
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM: sourceStr = "WindowSystem"sv; break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER: sourceStr = "ShaderCompiler"sv; break;
  case GL_DEBUG_SOURCE_THIRD_PARTY: sourceStr = "ThirdParty"sv; break;
  case GL_DEBUG_SOURCE_APPLICATION: sourceStr = "Application"sv; break;
  case GL_DEBUG_SOURCE_OTHER: sourceStr = "Other"sv; break;
  }
  log(LOG_SOURCE_ARGS(Test), level, "[%.*s] %.*s",
      static_cast<u32>(sourceStr.size()), sourceStr.data(),
      static_cast<u32>(length), message);
}

// OpenGL Helpers
// -----------------------------------------------------------------------------

#if BUILD_DEVELOPMENT
# define VA_FMT_STR() \
  char buf[1024]; \
  va_list args; \
  va_start(args, fmt); \
  auto length{ vsnprintf(buf, _countof(buf), fmt, args) }; \
  va_end(args); \
  ASSERT_EX(length < _countof(buf)); \
  std::string_view str{ buf, static_cast<usize>(length) }

static usize maxDebugMessageLength;

static void beginGroup(std::string_view const msg) {
  glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0,
                   std::min(maxDebugMessageLength, msg.size()), msg.data());
}
static void beginGroup(char const* fmt, ...) {
  VA_FMT_STR();
  beginGroup(str);
}
static void endGroup() {
  glPopDebugGroup();
}

static void marker(std::string_view const label) {
  glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION,
                       GL_DEBUG_TYPE_MARKER,
                       0,
                       GL_DEBUG_SEVERITY_NOTIFICATION,
                       std::min(maxDebugMessageLength, label.size()),
                       label.data());
}
static void marker(char const* fmt, ...) {
  VA_FMT_STR();
  marker(str);
}
static void label(GLenum identifier, GLuint name, std::string_view const label) {
  glObjectLabel(identifier, name, label.size(), label.data());
}
static void label(GLenum identifier, GLuint name, char const* fmt, ...) {
  VA_FMT_STR();
  label(identifier, name, str);
}

# undef VA_FMT_STR

# define glCheck() glCheckImpl(__FILE__, __LINE__)

static void glCheckImpl(char const* file, u32 line) {
  if (auto e{ glGetError() }; UNLIKELY(e != GL_NO_ERROR)) {
    char const* str;
    switch (e) {
# define E(x) case GL_##x: str = #x; break
      E(INVALID_ENUM);
      E(INVALID_VALUE);
      E(INVALID_OPERATION);
      E(OUT_OF_MEMORY);
      E(INVALID_FRAMEBUFFER_OPERATION);
# undef E
    default: ASSERT(0, "Unknown GL Error: 0x%04x", e); UNREACHABLE;
    }
    assertFailure(file, line, str);
  }
}

static void glCheckFramebuffer() {
  if (auto e{ glCheckFramebufferStatus(GL_FRAMEBUFFER) }; UNLIKELY(e != GL_FRAMEBUFFER_COMPLETE)) {
    char const* str;
    switch (e) {
# define E(x) case GL_FRAMEBUFFER_##x: str = #x; break;
      E(INCOMPLETE_ATTACHMENT);
      //E(INCOMPLETE_DIMENSIONS);
      E(INCOMPLETE_MISSING_ATTACHMENT);
      E(UNSUPPORTED);
    default: ASSERT(0, "Unknown GL Framebuffer Error: %04x", e); UNREACHABLE;
# undef E
    }
    ASSERT(0, str);
  }
}

# define GL_CHECK_INFO_LOG(name, get, getInfoLog, status, action) \
  static void name(u32 object) { \
    i32 success; \
    get(object, status, &success); \
    if UNLIKELY(!success) { \
      i32 size; \
      get(object, GL_INFO_LOG_LENGTH, &size); \
      std::string s; \
      s.resize(size); \
      getInfoLog(object, size, nullptr, s.data()); \
      LOG(Test, Error, "Failed to " action ":\n%.*s", size, s.data()); \
      ASSERT(0); \
    } \
  }
GL_CHECK_INFO_LOG(glCheckShader,  glGetShaderiv,  glGetShaderInfoLog,  GL_COMPILE_STATUS, "compile shader")
GL_CHECK_INFO_LOG(glCheckProgram, glGetProgramiv, glGetProgramInfoLog, GL_LINK_STATUS,    "link program")
# undef GL_CHECK_INFO_LOG
#else
# define beginGroup(fmt, ...)
# define endGroup()
# define marker(fmt, ...)
# define glCheck()
# define glCheckFramebuffer()
# define glCheckShader(shader)
# define glCheckProgram(program)
#endif

static bool isGLES;
static std::string glslHeader;

class ShaderBuilder {
  char const* sources[16];
  u32 numSources;

public:
  ShaderBuilder() : numSources(0) {
    push(glslHeader.c_str());
  }

  void push(char const* source) {
    ASSERT(numSources < _countof(sources));
    sources[numSources++] = source;
  }

  u32 compile(u32 stage) {
#if BUILD_DEBUG
    printf("================================================================================\n");
    for (auto n{ 0 }; n < numSources; n++) {
      printf("%s", sources[n]);
    }
#endif
    auto shader{ glCreateShader(stage) };
    glShaderSource(shader, numSources, sources, nullptr);
    glCompileShader(shader);
    glCheckShader(shader);
    return shader;
  }
};

template<typename ...N>
static u32 glBuildShader(u32 stage, N... sources) {
  ShaderBuilder b;
  for (auto s : { sources... }) b.push(s);
  return b.compile(stage);
}

template<typename ...N>
static u32 glBuildProgram(N... shaders) {
  auto prog{ glCreateProgram() };
  for (auto s : { shaders... }) glAttachShader(prog, s);
  glLinkProgram(prog);
  glCheckProgram(prog);
  glUseProgram(prog);
  return prog;
}


// OpenGL Extension Setup
// -----------------------------------------------------------------------------

static struct {
  usize ARB_ES2_compatibility:                          1;
  usize ARB_ES3_compatibility:                          1;
  usize ARB_ES3_1_compatibility:                        1;
  usize ARB_ES3_2_compatibility:                        1;
  usize ARB_arrays_of_arrays:                           1;
  usize ARB_base_instance:                              1;
  usize ARB_bindless_texture:                           1;
  usize ARB_blend_func_extended:                        1;
  usize ARB_buffer_storage:                             1;
  usize ARB_clear_buffer_object:                        1;
  usize ARB_clear_texture:                              1;
  usize ARB_clip_control:                               1;
  usize ARB_color_buffer_float:                         1;
  usize ARB_compressed_texture_pixel_storage:           1;
  usize ARB_compute_shader:                             1;
  usize ARB_compute_variable_group_size:                1;
  usize ARB_conditional_render_inverted:                1;
  usize ARB_conservative_depth:                         1;
  usize ARB_copy_buffer:                                1;
  usize ARB_copy_image:                                 1;
  usize ARB_cull_distance:                              1;
  usize ARB_debug_output:                               1;
  usize ARB_depth_buffer_float:                         1;
  usize ARB_depth_clamp:                                1;
  usize ARB_depth_texture:                              1;
  usize ARB_derivative_control:                         1;
  usize ARB_direct_state_access:                        1;
  usize ARB_draw_buffers_blend:                         1;
  usize ARB_draw_elements_base_vertex:                  1;
  usize ARB_draw_indirect:                              1;
  usize ARB_draw_instanced:                             1;
  usize ARB_enhanced_layouts:                           1;
  usize ARB_explicit_attrib_location:                   1;
  usize ARB_explicit_uniform_location:                  1;
  usize ARB_fragment_coord_conventions:                 1;
  usize ARB_fragment_layer_viewport:                    1;
  usize ARB_fragment_shader_interlock:                  1;
  usize ARB_framebuffer_no_attachments:                 1;
  usize ARB_framebuffer_object:                         1;
  usize ARB_framebuffer_sRGB:                           1;
  usize ARB_geometry_shader4:                           1;
  usize ARB_get_program_binary:                         1;
  usize ARB_get_texture_sub_image:                      1;
  usize ARB_gl_spirv:                                   1;
  usize ARB_gpu_shader5:                                1;
  usize ARB_gpu_shader_fp64:                            1;
  usize ARB_gpu_shader_int64:                           1;
  usize ARB_half_float_pixel:                           1;
  usize ARB_half_float_vertex:                          1;
  usize ARB_imaging:                                    1;
  usize ARB_indirect_parameters:                        1;
  usize ARB_instanced_arrays:                           1;
  usize ARB_internalformat_query:                       1;
  usize ARB_internalformat_query2:                      1;
  usize ARB_invalidate_subdata:                         1;
  usize ARB_map_buffer_alignment:                       1;
  usize ARB_map_buffer_range:                           1;
  usize ARB_multi_bind:                                 1;
  usize ARB_multi_draw_indirect:                        1;
  usize ARB_multisample:                                1;
  usize ARB_occlusion_query2:                           1;
  usize ARB_parallel_shader_compile:                    1;
  usize ARB_pipeline_statistics_query:                  1;
  usize ARB_polygon_offset_clamp:                       1;
  usize ARB_post_depth_coverage:                        1;
  usize ARB_program_interface_query:                    1;
  usize ARB_provoking_vertex:                           1;
  usize ARB_query_buffer_object:                        1;
  usize ARB_robust_buffer_access_behavior:              1;
  usize ARB_robustness:                                 1;
  usize ARB_sample_locations:                           1;
  usize ARB_sample_shading:                             1;
  usize ARB_sampler_objects:                            1;
  usize ARB_seamless_cube_map:                          1;
  usize ARB_seamless_cubemap_per_texture:               1;
  usize ARB_separate_shader_objects:                    1;
  usize ARB_shader_atomic_counters:                     1;
  usize ARB_shader_atomic_counter_ops:                  1;
  usize ARB_shader_ballot:                              1;
  usize ARB_shader_bit_encoding:                        1;
  usize ARB_shader_clock:                               1;
  usize ARB_shader_draw_parameters:                     1;
  usize ARB_shader_group_vote:                          1;
  usize ARB_shader_image_load_store:                    1;
  usize ARB_shader_image_size:                          1;
  usize ARB_shader_precision:                           1;
  usize ARB_shader_storage_buffer_object:               1;
  usize ARB_shader_subroutine:                          1;
  usize ARB_shader_texture_lod:                         1;
  usize ARB_shader_texture_image_samples:               1;
  usize ARB_shader_viewport_layer_array:                1;
  usize ARB_shading_language_420pack:                   1;
  usize ARB_shading_language_include:                   1;
  usize ARB_shading_language_packing:                   1;
  usize ARB_sparse_buffer:                              1;
  usize ARB_sparse_texture:                             1;
  usize ARB_sparse_texture2:                            1;
  usize ARB_sparse_texture_clamp:                       1;
  usize ARB_spirv_extensions:                           1;
  usize ARB_stencil_texturing:                          1;
  usize ARB_sync:                                       1;
  usize ARB_tessellation_shader:                        1;
  usize ARB_texture_barrier:                            1;
  usize ARB_texture_buffer_object:                      1;
  usize ARB_texture_buffer_object_rgb32:                1;
  usize ARB_texture_buffer_range:                       1;
  usize ARB_texture_compression_bptc:                   1;
  usize ARB_texture_compression_rgtc:                   1;
  usize ARB_texture_cube_map_array:                     1;
  usize ARB_texture_filter_anisotropic:                 1;
  usize ARB_texture_filter_minmax:                      1;
  usize ARB_texture_float:                              1;
  usize ARB_texture_gather:                             1;
  usize ARB_texture_mirror_clamp_to_edge:               1;
  usize ARB_texture_multisample:                        1;
  usize ARB_texture_query_levels:                       1;
  usize ARB_texture_query_lod:                          1;
  usize ARB_texture_rectangle:                          1;
  usize ARB_texture_rg:                                 1;
  usize ARB_texture_rgb10_a2ui:                         1;
  usize ARB_texture_stencil8:                           1;
  usize ARB_texture_storage:                            1;
  usize ARB_texture_storage_multisample:                1;
  usize ARB_texture_swizzle:                            1;
  usize ARB_texture_view:                               1;
  usize ARB_timer_query:                                1;
  usize ARB_transform_feedback2:                        1;
  usize ARB_transform_feedback3:                        1;
  usize ARB_transform_feedback_instanced:               1;
  usize ARB_transform_feedback_overflow_query:          1;
  usize ARB_uniform_buffer_object:                      1;
  usize ARB_vertex_array_bgra:                          1;
  usize ARB_vertex_array_object:                        1;
  usize ARB_vertex_attrib_64bit:                        1;
  usize ARB_vertex_attrib_binding:                      1;
  usize ARB_vertex_type_10f_11f_11f_rev:                1;
  usize ARB_vertex_type_2_10_10_10_rev:                 1;
  usize ARB_viewport_array:                             1;
  usize ARB_window_pos:                                 1;
  usize EXT_Cg_shader:                                  1;
  usize EXT_EGL_image_array:                            1;
  usize EXT_EGL_image_external_wrap_modes:              1;
  usize EXT_EGL_image_storage:                          1;
  usize EXT_YUV_target:                                 1;
  usize EXT_abgr:                                       1;
  usize EXT_bindable_uniform:                           1;
  usize EXT_blend_func_extended:                        1;
  usize EXT_blit_framebuffer_params:                    1;
  usize EXT_buffer_storage:                             1;
  usize EXT_clip_control:                               1;
  usize EXT_clip_cull_distance:                         1;
  usize EXT_color_buffer_float:                         1;
  usize EXT_color_buffer_half_float:                    1;
  usize EXT_copy_image:                                 1;
  usize EXT_debug_label:                                1;
  usize EXT_debug_marker:                               1;
  usize EXT_direct_state_access:                        1;
  usize EXT_discard_framebuffer:                        1;
  usize EXT_disjoint_timer_query:                       1;
  usize EXT_draw_buffers2:                              1;
  usize EXT_draw_buffers_indexed:                       1;
  usize EXT_draw_instanced:                             1;
  usize EXT_external_buffer:                            1;
  usize EXT_fragment_invocation_density:                1;
  usize EXT_framebuffer_blit:                           1;
  usize EXT_framebuffer_multisample:                    1;
  usize EXT_framebuffer_object:                         1;
  usize EXT_framebuffer_sRGB:                           1;
  usize EXT_geometry_shader:                            1;
  usize EXT_geometry_shader4:                           1;
  usize EXT_gpu_shader4:                                1;
  usize EXT_gpu_shader5:                                1;
  usize EXT_import_sync_object:                         1;
  usize EXT_memory_object:                              1;
  usize EXT_memory_object_fd:                           1;
  usize EXT_multisampled_render_to_texture:             1;
  usize EXT_multisampled_render_to_texture2:            1;
  usize EXT_packed_depth_stencil:                       1;
  usize EXT_packed_float:                               1;
  usize EXT_polygon_offset_clamp:                       1;
  usize EXT_post_depth_coverage:                        1;
  usize EXT_primitive_bounding_box:                     1;
  usize EXT_protected_textures:                         1;
  usize EXT_provoking_vertex:                           1;
  usize EXT_pvrtc_sRGB:                                 1;
  usize EXT_raster_multisample:                         1;
  usize EXT_read_format_bgra:                           1;
  usize EXT_robustness:                                 1;
  usize EXT_sRGB:                                       1;
  usize EXT_sRGB_write_control:                         1;
  usize EXT_semaphore:                                  1;
  usize EXT_separate_shader_objects:                    1;
  usize EXT_shader_framebuffer_fetch:                   1;
  usize EXT_shader_image_load_formatted:                1;
  usize EXT_shader_image_load_store:                    1;
  usize EXT_shader_integer_mix:                         1;
  usize EXT_shader_io_blocks:                           1;
  usize EXT_shader_non_constant_global_initializers:    1;
  usize EXT_shader_texture_lod:                         1;
  usize EXT_shadow_samplers:                            1;
  usize EXT_sparse_texture2:                            1;
  usize EXT_tessellation_shader:                        1;
  usize EXT_texture_array:                              1;
  usize EXT_texture_border_clamp:                       1;
  usize EXT_texture_buffer:                             1;
  usize EXT_texture_buffer_object:                      1;
  usize EXT_texture_compression_dxt1:                   1;
  usize EXT_texture_compression_latc:                   1;
  usize EXT_texture_compression_rgtc:                   1;
  usize EXT_texture_compression_s3tc:                   1;
  usize EXT_texture_cube_map_array:                     1;
  usize EXT_texture_filter_anisotropic:                 1;
  usize EXT_texture_filter_minmax:                      1;
  usize EXT_texture_format_BGRA8888:                    1;
  usize EXT_texture_format_sRGB_override:               1;
  usize EXT_texture_integer:                            1;
  usize EXT_texture_lod:                                1;
  usize EXT_texture_mirror_clamp:                       1;
  usize EXT_texture_norm16:                             1;
  usize EXT_texture_rectangle:                          1;
  usize EXT_texture_sRGB_decode:                        1;
  usize EXT_texture_sRGB_R8:                            1;
  usize EXT_texture_shared_exponent:                    1;
  usize EXT_texture_storage:                            1;
  usize EXT_texture_swizzle:                            1;
  usize EXT_texture_type_2_10_10_10_REV:                1;
  usize EXT_timer_query:                                1;
  usize EXT_transform_feedback2:                        1;
  usize EXT_vertex_array_bgra:                          1;
  usize EXT_vertex_attrib_64bit:                        1;
  usize EXT_window_rectangles:                          1;
  usize EXTX_framebuffer_mixed_formats:                 1;
  usize KHR_blend_equation_advanced:                    1;
  usize KHR_blend_equation_advanced_coherent:           1;
  usize KHR_debug:                                      1;
  usize KHR_context_flush_control:                      1;
  usize KHR_no_error:                                   1;
  usize KHR_parallel_shader_compile:                    1;
  usize KHR_robust_buffer_access_behavior:              1;
  usize KHR_robustness:                                 1;
  usize KHR_texture_compression_astc_ldr:               1;
  usize KHR_texture_compression_astc_hdr:               1;
  usize KTX_buffer_region:                              1;
  usize OES_EGL_image:                                  1;
  usize OES_EGL_image_external:                         1;
  usize OES_EGL_image_external_essl3:                   1;
  usize OES_EGL_sync:                                   1;
  usize OES_compressed_ETC1_RGB8_texture:               1;
  usize OES_depth24:                                    1;
  usize OES_depth_texture:                              1;
  usize OES_depth_texture_cube_map:                     1;
  usize OES_element_index_uint:                         1;
  usize OES_framebuffer_object:                         1;
  usize OES_get_program_binary:                         1;
  usize OES_packed_depth_stencil:                       1;
  usize OES_read_format:                                1;
  usize OES_rgb8_rgba8:                                 1;
  usize OES_shader_image_atomic:                        1;
  usize OES_sample_shading:                             1;
  usize OES_sample_variables:                           1;
  usize OES_shader_multisample_interpolation:           1;
  usize OES_surfaceless_context:                        1;
  usize OES_standard_derivatives:                       1;
  usize OES_texture_3D:                                 1;
  usize OES_texture_compression_astc:                   1;
  usize OES_texture_float:                              1;
  usize OES_texture_float_linear:                       1;
  usize OES_texture_format_sRGB_override:               1;
  usize OES_texture_half_float:                         1;
  usize OES_texture_half_float_linear:                  1;
  usize OES_texture_npot:                               1;
  usize OES_texture_stencil8:                           1;
  usize OES_texture_storage_multisample_2d_array:       1;
  usize OES_texture_type_2_10_10_10_REV:                1;
  usize OES_texture_view:                               1;
  usize OES_vertex_array_object:                        1;
  usize OES_vertex_half_float:                          1;
  usize AMD_compressed_ATC_texture:                     1;
  usize AMD_multi_draw_indirect:                        1;
  usize AMD_seamless_cubemap_per_texture:               1;
  usize AMD_shader_trinary_minmax:                      1;
  usize AMD_vertex_shader_viewport_index:               1;
  usize AMD_vertex_shader_layer:                        1;
  usize ANDROID_extension_pack_es31a:                   1;
  usize ANGLE_texture_compression_dxt3:                 1;
  usize ANGLE_texture_compression_dxt5:                 1;
  usize APPLE_clip_distance:                            1;
  usize APPLE_color_buffer_packed_float:                1;
  usize APPLE_copy_texture_levels:                      1;
  usize APPLE_rgb_422:                                  1;
  usize APPLE_texture_format_BGRA8888:                  1;
  usize ARM_shader_framebuffer_fetch_depth_stencil:     1;
  usize ATI_texture_float:                              1;
  usize ATI_texture_mirror_once:                        1;
  usize IMG_read_format:                                1;
  usize IMG_texture_compression_pvrtc:                  1;
  usize MESA_window_pos:                                1;
  usize NV_ES1_1_compatibility:                         1;
  usize NV_ES3_1_compatibility:                         1;
  usize NV_bindless_multi_draw_indirect:                1;
  usize NV_bindless_multi_draw_indirect_count:          1;
  usize NV_bindless_texture:                            1;
  usize NV_blend_equation_advanced:                     1;
  usize NV_blend_equation_advanced_coherent:            1;
  usize NV_compute_program5:                            1;
  usize NV_compute_shader_derivatives:                  1;
  usize NV_conditional_render:                          1;
  usize NV_conservative_raster:                         1;
  usize NV_conservative_raster_dilate:                  1;
  usize NV_conservative_raster_pre_snap:                1;
  usize NV_conservative_raster_pre_snap_triangles:      1;
  usize NV_conservative_raster_underestimation:         1;
  usize NV_draw_texture:                                1;
  usize NV_draw_vulkan_image:                           1;
  usize NV_feature_query:                               1;
  usize NV_fence:                                       1;
  usize NV_geometry_shader4:                            1;
  usize NV_geometry_shader_passthrough:                 1;
  usize NV_gpu_program4:                                1;
  usize NV_gpu_program4_1:                              1;
  usize NV_gpu_program5:                                1;
  usize NV_gpu_program5_mem_extended:                   1;
  usize NV_gpu_program_fp64:                            1;
  usize NV_gpu_shader5:                                 1;
  usize NV_half_float:                                  1;
  usize NV_memory_attachment:                           1;
  usize NV_mesh_shader:                                 1;
  usize NV_packed_depth_stencil:                        1;
  usize NV_parameter_buffer_object:                     1;
  usize NV_parameter_buffer_object2:                    1;
  usize NV_path_rendering:                              1;
  usize NV_path_rendering_shared_edge:                  1;
  usize NV_primitive_restart:                           1;
  usize NV_shader_atomic_counters:                      1;
  usize NV_shader_atomic_float:                         1;
  usize NV_shader_atomic_float64:                       1;
  usize NV_shader_atomic_fp16_vector:                   1;
  usize NV_shader_atomic_int64:                         1;
  usize NV_shader_buffer_load:                          1;
  usize NV_shader_noperspective_interpolation:          1;
  usize NV_shader_storage_buffer_object:                1;
  usize NV_shader_texture_footprint:                    1;
  usize NV_shader_thread_group:                         1;
  usize NV_shader_thread_shuffle:                       1;
  usize NV_stereo_view_rendering:                       1;
  usize NV_texture_barrier:                             1;
  usize NV_texture_compression_vtc:                     1;
  usize NV_texture_rectangle:                           1;
  usize NV_texture_shader:                              1;
  usize NV_texture_shader2:                             1;
  usize NV_texture_shader3:                             1;
  usize NV_transform_feedback:                          1;
  usize NV_transform_feedback2:                         1;
  usize NV_uniform_buffer_unified_memory:               1;
  usize NV_vertex_array_range:                          1;
  usize NV_vertex_array_range2:                         1;
  usize NV_vertex_attrib_64bit:                         1;
  usize NV_vertex_buffer_unified_memory:                1;
  usize NVX_blend_equation_advanced_multi_draw_buffers: 1;
  usize NVX_conditional_render:                         1;
  usize NVX_gpu_memory_info:                            1;
  usize NVX_multigpu_info:                              1;
  usize NVX_nvenc_interop:                              1;
  usize OVR_multiview:                                  1;
  usize OVR_multiview2:                                 1;
  usize OVR_multiview_multisampled_render_to_texture:   1;
  usize QCOM_alpha_test:                                1;
  usize QCOM_texture_foveated:                          1;
  usize QCOM_tiled_rendering:                           1;
  usize QCOM_shader_framebuffer_fetch_noncoherent:      1;
  usize QCOM_shader_framebuffer_fetch_rate:             1;
  usize S3_s3tc:                                        1;
  usize SGIS_generate_mipmap:                           1;
#if PLATFORM_WINDOWS
  usize EXT_memory_object_win32:                        1;
  usize EXT_semaphore_win32:                            1;
  usize EXT_win32_keyed_mutex:                          1;
  usize WIN_swap_hint:                                  1;
#elif PLATFORM_HTML5
  usize WEBGL_compressed_texture_etc: 1;
  usize WEBGL_compressed_texture_etc1: 1;
  usize WEBGL_compressed_texture_s3tc: 1;
  usize WEBGL_debug_renderer_info: 1;
  usize WEBGL_debug_shaders: 1;
  usize WEBGL_lose_context: 1;
  usize WEBGL_multi_draw: 1;
  usize WEBGL_multi_draw_instanced: 1;
  usize WEBGL_video_texture: 1;
#endif
} exts;

#define TEST(name, body) else if ("GL_"#name##sv == ext) {  \
    exts.name = true;                                       \
    body                                                    \
    return true;                                            \
  }

static bool setupExtensionGL3(std::string_view ext) {
  if constexpr (false) {}
  TEST(ARB_blend_func_extended, { // 3.3

  })
  TEST(ARB_copy_buffer, { // 3.0
  })
  TEST(ARB_depth_buffer_float, { // 3.0

  })
  TEST(ARB_depth_clamp, {}) // 3.2
  TEST(ARB_draw_elements_base_vertex, { // 3.2
  })
  TEST(ARB_draw_instanced, { // 3.1

  })
  TEST(ARB_explicit_attrib_location, { // 3.3
  })
  TEST(ARB_framebuffer_object, { // 3.0
  })
  TEST(ARB_framebuffer_sRGB, { // 3.0
  })
  TEST(ARB_geometry_shader4, { // 3.2

  })
  TEST(ARB_half_float_pixel, { // 3.1
  })
  TEST(ARB_half_float_vertex, { // 3.1
  })
  TEST(ARB_imaging, {})
  TEST(ARB_instanced_arrays, { // 3.3

  })
  TEST(ARB_map_buffer_range, { // 3.0
  })
  TEST(ARB_multisample, { // 3.0
  })
  TEST(ARB_occlusion_query2, { // 3.3
  })
  TEST(ARB_provoking_vertex, { // 3.2
  })
  TEST(ARB_seamless_cube_map, {}) // 3.2
  TEST(ARB_seamless_cubemap_per_texture, {}) // 3.2
  TEST(ARB_sampler_objects, { // 3.3
  })
  TEST(ARB_shader_bit_encoding, { // 3.3

  })
  TEST(ARB_sync, { // 3.2

  })
  TEST(ARB_texture_buffer_object, { // 3.1

  })
  TEST(ARB_texture_float, { // 3.0
  })
  TEST(ARB_texture_multisample, { // 3.2

  })
  TEST(ARB_texture_rectangle, { // 3.1
  })
  TEST(ARB_texture_rg, {}) // 3.0
  TEST(ARB_texture_rgb10_a2ui, { // 3.3

  })
  TEST(ARB_texture_compression_rgtc, {}) // 3.0
  TEST(ARB_texture_swizzle, { // 3.3
  })
  TEST(ARB_timer_query, { // 3.3

  })
  TEST(ARB_uniform_buffer_object, {}) // 3.1
  TEST(ARB_vertex_array_object, { // 3.0
  })
  TEST(ARB_vertex_type_2_10_10_10_rev, { // 3.3

  })
  TEST(ARB_vertex_array_bgra, { // 3.2
  })
  else {
    return false;
  }
}

static bool setupExtensionGL4(std::string_view ext) {
  if constexpr (false) {}
  TEST(ARB_ES2_compatibility, { // 4.1
  })
  TEST(ARB_ES3_compatibility, { // 4.3

  })
  TEST(ARB_ES3_1_compatibility, { // 4.5

  })
  TEST(ARB_ES3_2_compatibility, {

  })
  TEST(ARB_buffer_storage, { // 4.4

  })
  TEST(ARB_clear_buffer_object, { // 4.3
  })
  TEST(ARB_clear_texture, { // 4.4

  })
  TEST(ARB_clip_control, { // 4.5

  })
  TEST(ARB_compressed_texture_pixel_storage, { // 4.2
  })
  TEST(ARB_compute_shader, { // 4.3

  })
  TEST(ARB_conditional_render_inverted, { // 4.5

  })
  TEST(ARB_copy_image, { // 4.3

  })
  TEST(ARB_cull_distance, { // 4.5

  })
  TEST(ARB_debug_output, { // 4.3
  })
  TEST(ARB_derivative_control, {}) // 4.5
  TEST(ARB_direct_state_access, { // 4.5

  })
  TEST(ARB_draw_buffers_blend, { // 4.0

  })
  TEST(ARB_draw_indirect, { // 4.3

  })
  TEST(ARB_enhanced_layouts, {}) // 4.4
  TEST(ARB_explicit_uniform_location, { // 4.3
  })
  TEST(ARB_fragment_layer_viewport, {}) // 4.3
  TEST(ARB_framebuffer_no_attachments, {}) // 4.3
  TEST(ARB_get_program_binary, { // 4.1
  })
  TEST(ARB_get_texture_sub_image, { // 4.5
  })
  TEST(ARB_gl_spirv, { // 4.6

  })
  TEST(ARB_gpu_shader5, { // 4.0

  })
  TEST(ARB_gpu_shader_fp64, { // 4.0

  })
  TEST(ARB_indirect_parameters, { // 4.6

  })
  TEST(ARB_internalformat_query, { // 4.2
  })
  TEST(ARB_internalformat_query2, { // 4.3
  })
  TEST(ARB_invalidate_subdata, { // 4.3
  })
  TEST(ARB_map_buffer_alignment, { // 4.2
  })
  TEST(ARB_multi_bind, { // 4.4
  })
  TEST(ARB_multi_draw_indirect, { // 4.3

  })
  TEST(ARB_pipeline_statistics_query, {}) // 4.6
  TEST(ARB_polygon_offset_clamp, {}) // 4.6
  TEST(ARB_program_interface_query, { // 4.3
  })
  TEST(ARB_query_buffer_object, {}) // 4.4
  TEST(ARB_robustness, { // 4.5
  })
  TEST(ARB_sample_shading, { // 4.0

  })
  TEST(ARB_separate_shader_objects, { // 4.1
  })
  TEST(ARB_shader_atomic_counters, {}) // 4.2
  TEST(ARB_shader_draw_parameters, {}) // 4.6
  TEST(ARB_shader_image_load_store, { // 4.2

  })
  TEST(ARB_shader_image_size, {}) // 4.3
  TEST(ARB_shader_storage_buffer_object, { // 4.3

  })
  TEST(ARB_shader_subroutine, { // 4.0

  })
  TEST(ARB_shader_texture_image_samples, {}) // 4.5
  TEST(ARB_shader_viewport_layer_array, {}) // 4.3
  TEST(ARB_shading_language_420pack, {}) // 4.2
  TEST(ARB_shading_language_packing, {}) // 4.0
  TEST(ARB_spirv_extensions, {}) // 4.6
  TEST(ARB_stencil_texturing, {}) // 4.3
  TEST(ARB_tessellation_shader, { // 4.0

  })
  TEST(ARB_texture_barrier, {}) // 4.5
  TEST(ARB_texture_buffer_range, { // 4.3

  })
  TEST(ARB_texture_compression_bptc, {}) // 4.2
  TEST(ARB_texture_cube_map_array, { // 4.0

  })
  TEST(ARB_texture_filter_anisotropic, { // 4.6

  })
  TEST(ARB_texture_gather, { // 4.0

  })
  TEST(ARB_texture_mirror_clamp_to_edge, {}) // 4.4
  TEST(ARB_texture_query_lod, { // 4.0

  })
  TEST(ARB_texture_query_levels, {}) // 4.3
  TEST(ARB_texture_query_lod, {}) // 4.0
  TEST(ARB_texture_storage, { // 4.2
  })
  TEST(ARB_texture_storage_multisample, { // 4.3

  })
  TEST(ARB_texture_view, { // 4.3

  })
  TEST(ARB_transform_feedback2, { // 4.0

  })
  TEST(ARB_transform_feedback3, { // 4.0

  })
  TEST(ARB_vertex_attrib_64bit, { // 4.1

  })
  TEST(ARB_vertex_attrib_binding, { // 4.3
  })
  TEST(ARB_viewport_array, { // 4.1

  })
  else {
    return false;
  }
}

static bool setupExtensionNonCore(std::string_view ext) {
  if constexpr (false) {}
  TEST(ARB_arrays_of_arrays, {})
  TEST(ARB_base_instance, {

  })
  TEST(ARB_bindless_texture, {

  })
  TEST(ARB_color_buffer_float, {
  })
  TEST(ARB_compute_variable_group_size, {

  })
  TEST(ARB_conservative_depth, {})
  TEST(ARB_fragment_coord_conventions, {
  })
  TEST(ARB_fragment_shader_interlock, {})
  TEST(ARB_gpu_shader_int64, {

  })
  TEST(ARB_parallel_shader_compile, {})
  TEST(ARB_post_depth_coverage, {})
  TEST(ARB_robust_buffer_access_behavior, {})
  TEST(ARB_sample_locations, {

  })
  TEST(ARB_shader_atomic_counter_ops, {})
  TEST(ARB_shader_ballot, {})
  TEST(ARB_shader_clock, {})
  TEST(ARB_shader_group_vote, {})
  TEST(ARB_shader_precision, {})
  TEST(ARB_shader_texture_lod, {
  })
  TEST(ARB_shading_language_include, {

  })
  TEST(ARB_sparse_buffer, {

  })
  TEST(ARB_sparse_texture, {

  })
  TEST(ARB_sparse_texture2, {})
  TEST(ARB_sparse_texture_clamp, {})
  TEST(ARB_texture_buffer_object_rgb32, {

  })
  TEST(ARB_texture_filter_minmax, {})
  TEST(ARB_texture_stencil8, {})
  TEST(ARB_transform_feedback_instanced, {})
  TEST(ARB_transform_feedback_overflow_query, {})
  TEST(ARB_vertex_type_10f_11f_11f_rev, {})
  TEST(ARB_window_pos, {
  })
  else {
    return false;
  }
}

static bool setupExtensionEXT(std::string_view ext) {
  if constexpr (false) {}
  TEST(EXT_Cg_shader, {})
  TEST(EXT_EGL_image_array, {})
  TEST(EXT_EGL_image_external_wrap_modes, {})
  TEST(EXT_EGL_image_storage, {})
  TEST(EXT_YUV_target, {})
  TEST(EXT_abgr, {})
  TEST(EXT_bindable_uniform, {})
  TEST(EXT_blend_func_extended, {})
  TEST(EXT_blit_framebuffer_params, {})
  TEST(EXT_buffer_storage, {})
  TEST(EXT_clip_control, {})
  TEST(EXT_clip_cull_distance, {})
  TEST(EXT_color_buffer_float, {})
  TEST(EXT_color_buffer_half_float, {})
  TEST(EXT_copy_image, {})
  TEST(EXT_debug_label, {

  })
  TEST(EXT_debug_marker, {

  })
  TEST(EXT_direct_state_access, {

  })
  TEST(EXT_discard_framebuffer, {})
  TEST(EXT_disjoint_timer_query, {})
  TEST(EXT_draw_buffers2, {})
  TEST(EXT_draw_buffers_indexed, {})
  TEST(EXT_draw_instanced, {})
  TEST(EXT_external_buffer, {})
  TEST(EXT_fragment_invocation_density, {})
  TEST(EXT_framebuffer_blit, { // 3.0

  })
  TEST(EXT_framebuffer_multisample, {})
  TEST(EXT_framebuffer_object, { // 3.0

  })
  TEST(EXT_framebuffer_sRGB, { // 3.0

  })
  TEST(EXT_geometry_shader, {})
  TEST(EXT_geometry_shader4, {})
  TEST(EXT_gpu_shader4, {})
  TEST(EXT_gpu_shader5, {})
  TEST(EXT_import_sync_object, {})
  TEST(EXT_memory_object, {})
  TEST(EXT_memory_object_fd, {})
  TEST(EXT_multisampled_render_to_texture, {})
  TEST(EXT_multisampled_render_to_texture2, {})
  TEST(EXT_packed_depth_stencil, { // 3.0

  })
  TEST(EXT_packed_float, {})
  TEST(EXT_polygon_offset_clamp, {})
  TEST(EXT_post_depth_coverage, {})
  TEST(EXT_primitive_bounding_box, {})
  TEST(EXT_protected_textures, {})
  TEST(EXT_provoking_vertex, { // 3.2

  })
  TEST(EXT_pvrtc_sRGB, {})
  TEST(EXT_raster_multisample, {})
  TEST(EXT_read_format_bgra, {})
  TEST(EXT_robustness, {})
  TEST(EXT_sRGB, {})
  TEST(EXT_sRGB_write_control, {})
  TEST(EXT_semaphore, {})
  TEST(EXT_separate_shader_objects, {})
  TEST(EXT_shader_framebuffer_fetch, {})
  TEST(EXT_shader_image_load_formatted, {})
  TEST(EXT_shader_image_load_store, {})
  TEST(EXT_shader_integer_mix, {})
  TEST(EXT_shader_io_blocks, {})
  TEST(EXT_shader_non_constant_global_initializers, {})
  TEST(EXT_shader_texture_lod, {})
  TEST(EXT_shadow_samplers, {})
  TEST(EXT_sparse_texture2, {})
  TEST(EXT_tessellation_shader, {})
  TEST(EXT_texture_array, {})
  TEST(EXT_texture_border_clamp, {})
  TEST(EXT_texture_buffer, {})
  TEST(EXT_texture_buffer_object, {})
  TEST(EXT_texture_compression_dxt1, {})
  TEST(EXT_texture_compression_latc, {})
  TEST(EXT_texture_compression_rgtc, {})
  TEST(EXT_texture_compression_s3tc, {})
  TEST(EXT_texture_cube_map_array, {})
  TEST(EXT_texture_filter_anisotropic, {})
  TEST(EXT_texture_filter_minmax, {})
  TEST(EXT_texture_format_BGRA8888, {})
  TEST(EXT_texture_format_sRGB_override, {})
  TEST(EXT_texture_integer, {})
  TEST(EXT_texture_lod, {})
  TEST(EXT_texture_mirror_clamp, {

  })
  TEST(EXT_texture_norm16, {})
  TEST(EXT_texture_rectangle, { // 3.1

  })
  TEST(EXT_texture_sRGB_decode, {})
  TEST(EXT_texture_sRGB_R8, {})
  TEST(EXT_texture_shared_exponent, {})
  TEST(EXT_texture_storage, {})
  TEST(EXT_texture_swizzle, { // 3.3

  })
  TEST(EXT_texture_type_2_10_10_10_REV, {})
  TEST(EXT_timer_query, {})
  TEST(EXT_transform_feedback2, {})
  TEST(EXT_vertex_array_bgra, { // 3.2

  })
  TEST(EXT_vertex_attrib_64bit, {})
  TEST(EXT_window_rectangles, {})
#if PLATFORM_WINDOWS
  TEST(EXT_memory_object_win32, {})
  TEST(EXT_semaphore_win32, {})
  TEST(EXT_win32_keyed_mutex, {})
#endif
  else {
    return false;
  }
}

static bool setupExtensionMisc(std::string_view ext) {
  if constexpr (false) {}
  TEST(EXTX_framebuffer_mixed_formats, {})
  TEST(KHR_blend_equation_advanced, {})
  TEST(KHR_blend_equation_advanced_coherent, {})
  TEST(KHR_context_flush_control, {
  })
  TEST(KHR_debug, {
  })
  TEST(KHR_no_error, {
  })
  TEST(KHR_parallel_shader_compile, {})
  TEST(KHR_robust_buffer_access_behavior, {})
  TEST(KHR_robustness, {})
  TEST(KHR_texture_compression_astc_ldr, {})
  TEST(KHR_texture_compression_astc_hdr, {})
  TEST(KTX_buffer_region, {})
  TEST(OES_EGL_image, {})
  TEST(OES_EGL_image_external, {})
  TEST(OES_EGL_image_external_essl3, {})
  TEST(OES_EGL_sync, {})
  TEST(OES_compressed_ETC1_RGB8_texture, {})
  TEST(OES_depth24, {})
  TEST(OES_depth_texture, {})
  TEST(OES_depth_texture_cube_map, {})
  TEST(OES_element_index_uint, {})
  TEST(OES_framebuffer_object, {})
  TEST(OES_get_program_binary, {})
  TEST(OES_packed_depth_stencil, {})
  TEST(OES_read_format, { // 4.1

  })
  TEST(OES_rgb8_rgba8, {})
  TEST(OES_sample_shading, {})
  TEST(OES_sample_variables, {})
  TEST(OES_shader_image_atomic, {})
  TEST(OES_shader_multisample_interpolation, {})
  TEST(OES_standard_derivatives, {})
  TEST(OES_surfaceless_context, {})
  TEST(OES_texture_3D, {})
  TEST(OES_texture_compression_astc, {})
  TEST(OES_texture_float, {})
  TEST(OES_texture_float_linear, {})
  TEST(OES_texture_format_sRGB_override, {})
  TEST(OES_texture_half_float, {})
  TEST(OES_texture_half_float_linear, {})
  TEST(OES_texture_npot, {})
  TEST(OES_texture_type_2_10_10_10_REV, {})
  TEST(OES_texture_stencil8, {})
  TEST(OES_texture_storage_multisample_2d_array, {})
  TEST(OES_texture_view, {})
  TEST(OES_vertex_array_object, {})
  TEST(OES_vertex_half_float, {})
  TEST(AMD_compressed_ATC_texture, {})
  TEST(AMD_multi_draw_indirect, {

  })
  TEST(AMD_seamless_cubemap_per_texture, {})
  TEST(AMD_shader_trinary_minmax, {})
  TEST(AMD_vertex_shader_layer, {})
  TEST(AMD_vertex_shader_viewport_index, {})
  TEST(ANDROID_extension_pack_es31a, {})
  TEST(ANGLE_texture_compression_dxt3, {})
  TEST(ANGLE_texture_compression_dxt5, {})
  TEST(APPLE_clip_distance, {})
  TEST(APPLE_color_buffer_packed_float, {})
  TEST(APPLE_copy_texture_levels, {})
  TEST(APPLE_rgb_422, {})
  TEST(APPLE_texture_format_BGRA8888, {})
  TEST(ARM_shader_framebuffer_fetch_depth_stencil, {})
  TEST(ATI_texture_float, {}) // 3.0
  TEST(ATI_texture_mirror_once, {})
  TEST(IMG_read_format, {})
  TEST(IMG_texture_compression_pvrtc, {})
  TEST(MESA_window_pos, {
  })
  TEST(OVR_multiview, {})
  TEST(OVR_multiview2, {})
  TEST(OVR_multiview_multisampled_render_to_texture, {})
  TEST(QCOM_alpha_test, {})
  TEST(QCOM_tiled_rendering, {})
  TEST(QCOM_texture_foveated, {})
  TEST(QCOM_shader_framebuffer_fetch_noncoherent, {})
  TEST(QCOM_shader_framebuffer_fetch_rate, {})
  TEST(S3_s3tc, {})
  TEST(SGIS_generate_mipmap, { // 3.0
  })
#if PLATFORM_HTML5
  TEST(WEBGL_compressed_texture_etc, {})
  TEST(WEBGL_compressed_texture_etc1, {})
  TEST(WEBGL_compressed_texture_s3tc, {})
  TEST(WEBGL_debug_renderer_info, {})
  TEST(WEBGL_debug_shaders, {})
  TEST(WEBGL_lose_context, {})
  TEST(WEBGL_multi_draw, {})
  TEST(WEBGL_multi_draw_instanced, {})
  TEST(WEBGL_video_texture, {})
#endif
  else {
    return false;
  }
}

static bool setupExtensionNV(std::string_view ext) {
  if constexpr (false) {}
  TEST(NV_packed_depth_stencil, {}) // 3.0
  TEST(NV_primitive_restart, {
  })
  TEST(NV_shader_noperspective_interpolation, {})
  TEST(NV_texture_barrier, { // 4.5

  })
  TEST(NV_texture_rectangle, { // 3.1
  })
  else {
    return false;
  }
}
#undef TEST

#if BUILD_DEVELOPMENT
static void setupExtensionMissing(std::string_view ext) {
# define NONE(name) else if ("GL_"#name##sv == ext) {}
  if (false) {}
  NONE(ARB_multitexture)             // 1.2
  NONE(ARB_texture_border_clamp)     // 1.3
  NONE(ARB_texture_compression)      // 1.3
  NONE(ARB_texture_cube_map)         // 1.3
  NONE(ARB_depth_texture)            // 1.4
  NONE(ARB_point_parameters)         // 1.4
  NONE(ARB_texture_mirrored_repeat)  // 1.4
  NONE(ARB_occlusion_query)          // 1.5
  NONE(ARB_vertex_buffer_object)     // 1.5
  NONE(ARB_draw_buffers)             // 2.0
  NONE(ARB_fragment_shader)          // 2.0
  NONE(ARB_shader_objects)           // 2.0
  NONE(ARB_shading_language_100)     // 2.0
  NONE(ARB_texture_non_power_of_two) // 2.0
  NONE(ARB_vertex_shader)            // 2.0
  NONE(ARB_pixel_buffer_object)      // 2.1
  NONE(EXT_copy_texture)             // 1.1
  NONE(EXT_subtexture)               // 1.1
  NONE(EXT_texture)                  // 1.1
  NONE(EXT_texture_object)           // 1.1
  NONE(EXT_vertex_array)             // 1.1
  NONE(EXT_bgra)                     // 1.2
  NONE(EXT_draw_range_elements)      // 1.2
  NONE(EXT_packed_pixels)            // 1.2
  NONE(EXT_texture3D)                // 1.2
  NONE(EXT_texture_edge_clamp)       // 1.2
  NONE(EXT_texture_cube_map)         // 1.3
  NONE(EXT_blend_color)              // 1.3
  NONE(EXT_blend_func_separate)      // 1.4
  NONE(EXT_blend_minmax)             // 1.4
  NONE(EXT_blend_subtract)           // 1.4
  NONE(EXT_multi_draw_arrays)        // 1.4
  NONE(EXT_point_parameters)         // 1.4
  NONE(EXT_stencil_wrap)             // 1.4
  NONE(EXT_texture_lod_bias)         // 1.4
  NONE(EXT_blend_equation_separate)  // 2.0
  NONE(EXT_pixel_buffer_object)      // 2.1
  NONE(EXT_texture_sRGB)             // 2.1
  NONE(APPLE_packed_pixels)          // 1.2
  NONE(ATI_blend_equation_separate)  // 2.0
  NONE(ATI_separate_stencil)         // 2.0
  NONE(IBM_texture_mirrored_repeat)  // 1.4
  NONE(INGR_blend_func_separate)     // 1.4
  NONE(SGIS_texture_edge_clamp)      // 1.2
  NONE(SGIS_texture_lod)             // 1.2
  NONE(SGIS_texture_border_clamp)    // 1.3
  NONE(SUN_multi_draw_arrays)        // 1.4
  // Deprecated / Misc. Ignored
  NONE(ARB_fragment_program)
  NONE(ARB_fragment_program_shadow)
  NONE(ARB_point_sprite)
  NONE(ARB_shadow)
  NONE(ARB_texture_env_add)
  NONE(ARB_texture_env_combine)
  NONE(ARB_texture_env_crossbar)
  NONE(ARB_texture_env_dot3)
  NONE(ARB_transpose_matrix)
  NONE(ARB_vertex_program)
  NONE(EXT_compiled_vertex_array)
  NONE(EXT_depth_bounds_test)
  NONE(EXT_framebuffer_multisample_blit_scaled)
  NONE(EXT_fog_coord)
  NONE(EXT_gpu_program_parameters)
  NONE(EXT_rescale_normal)
  NONE(EXT_texture_env_add)
  NONE(EXT_texture_env_combine)
  NONE(EXT_texture_env_dot3)
  NONE(EXT_texture_shadow_funcs)
  NONE(EXT_secondary_color)
  NONE(EXT_separate_specular_color)
  NONE(EXT_shadow_funcs)
  NONE(EXT_stencil_two_side)
  NONE(APPLE_client_storage)
  NONE(APPLE_container_object_shareable)
  NONE(APPLE_flush_render)
  NONE(APPLE_row_bytes)
  NONE(APPLE_texture_range)
  NONE(ATI_draw_buffers)
  NONE(ATI_texture_env_combine3)
  NONE(ATI_fragment_shader)
  NONE(IBM_multimode_draw_arrays)
  NONE(IBM_rasterpos_clip)
  NONE(MESA_pack_invert)
  NONE(NV_blend_square)
  NONE(NV_fog_distance)
  NONE(NV_light_max_exponent)
  NONE(NV_texgen_reflection)
  NONE(NV_texture_env_combine4)
  NONE(NV_register_combiners)
  NONE(NV_register_combiners2)
  else {
    LOG(Test, Warn, "Unknown GL Extension: %.*s", static_cast<u32>(ext.size()), ext.data());
  }
# undef NONE
}
#else
static void setupExtensionMissing(std::string_view ext) {}
#endif

static void setupExtension(std::string_view ext) {
  if UNLIKELY(ext.empty()) {} // The GL_EXTENSIONS string can end with a space.
  else if (strncmp("GL_ARB_", ext.data(), 7) == 0) {
    if (!setupExtensionGL3(ext) && !setupExtensionGL4(ext) && !setupExtensionNonCore(ext)) {
      setupExtensionMissing(ext);
    }
  }
  else if (strncmp("GL_EXT_", ext.data(), 7) == 0) {
    if (!setupExtensionEXT(ext)) {
      setupExtensionMissing(ext);
    }
  }
  else if (strncmp("GL_NV_", ext.data(), 6) == 0) {
    if (!setupExtensionNV(ext)) {
      setupExtensionMissing(ext);
    }
  }
  else {
    if (!setupExtensionMisc(ext)) {
      setupExtensionMissing(ext);
    }
  }
}


// OpenGL + ImGui test
// -----------------------------------------------------------------------------

static char const* baseName(char const* filename) {
#if PLATFORM_WINDOWS
  {
    auto p{ strrchr(filename, '\\') };
    if (p) return p + 1;
  }
#endif
  auto p{ strrchr(filename, '/') };
  return p ? p + 1 : filename;
}

struct Model {
  u32 vao;
  u32 vbo;
  u32 ibo;
  u32 subMeshesCount;
  MeshHeader::SubMesh* subMeshes;
  u32 numMaterials;
  Material* materials;
  u32* progsForward;
  u32* progsDeferred;
  u32 shadowProg;
  bool isSkinned;
  char const* baseName;
};

void testMeshLoad(char const* filename, char const* data, Model* model) {
#if BUILD_DEVELOPMENT
  model->baseName = baseName(filename);
#endif

  auto meshHeader = std::launder(reinterpret_cast<MeshHeader const*>(data));

  beginGroup("Mesh %s", model->baseName);
  glGenVertexArrays(1, &model->vao);
  glBindVertexArray(model->vao);
  glGenBuffers(1, &model->vbo);
  glGenBuffers(1, &model->ibo);
  glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);
  label(GL_VERTEX_ARRAY, model->vao, "%s VAO", model->baseName);
  label(GL_BUFFER, model->vbo, "%s VBO", model->baseName);
  label(GL_BUFFER, model->ibo, "%s IBO", model->baseName);

  model->subMeshes = new MeshHeader::SubMesh[meshHeader->subMeshCount];
  model->subMeshesCount = meshHeader->subMeshCount;
  auto const subMeshesSize = sizeof(MeshHeader::SubMesh) * meshHeader->subMeshCount;
  memcpy(model->subMeshes, data + sizeof(MeshHeader), subMeshesSize);

  auto const indicesOffset = sizeof(MeshHeader) + subMeshesSize;
  auto const indicesSize = meshHeader->numIndices * 4;
  auto const verticesOffset = indicesOffset + indicesSize;
  auto verticesSize = 0;

  if (meshHeader->flags & MeshHeader::HasPositions) {
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, nullptr);
    verticesSize += meshHeader->numVertices * 12;
  }
  if (meshHeader->flags & MeshHeader::HasNormals) {
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, reinterpret_cast<void*>(verticesSize));
    verticesSize += meshHeader->numVertices * 12;
  }
  if (meshHeader->flags & MeshHeader::HasTangents) {
  #if 0
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, reinterpret_cast<void*>(verticesSize));
    verticesSize += meshHeader->numVertices * 12;
  #endif
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, false, 0, reinterpret_cast<void*>(verticesSize));
    verticesSize += meshHeader->numVertices * 12;
  }
  if (meshHeader->flags & MeshHeader::HasTexCoords) {
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, false, 0, reinterpret_cast<void*>(verticesSize));
    verticesSize += meshHeader->numVertices * 8;
  }
  if (meshHeader->flags & MeshHeader::HasBones) {
    model->isSkinned = true;
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, false, 0, reinterpret_cast<void*>(verticesSize));
    verticesSize += meshHeader->numVertices * 16;
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 4, GL_UNSIGNED_BYTE, 0, reinterpret_cast<void*>(verticesSize));
    verticesSize += meshHeader->numVertices * 4;
  }
  glBufferData(GL_ARRAY_BUFFER, verticesSize, data + verticesOffset, GL_STATIC_DRAW);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize, data + indicesOffset, GL_STATIC_DRAW);
  glBindVertexArray(0);
  endGroup();
  glCheck();
}

static void setupTexture(u32 wrap, u32 filter, bool aniso) {
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

  if (aniso) glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 16);
}
static void setupTexture3(u32 target, u32 wrap, u32 filterMag, u32 filterMin) {
  glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap);
  glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap);
  glTexParameteri(target, GL_TEXTURE_WRAP_R, wrap);
  glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filterMin);
  glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filterMag);
}

#if BUILD_EDITOR
u32 importTexture(char const* filename, TextureType type);

u32 loadImage(char const* filename, CMP_Texture const& tex, TextureType type) {
#if BUILD_DEVELOPMENT
  std::string_view name{ baseName(filename) };
#endif

  u32 id;
  beginGroup(name);
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  setupTexture(GL_REPEAT, GL_LINEAR, true);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  switch (type) {
  case TextureType::Base:
    glCompressedTexImage2D(GL_TEXTURE_2D, 0,
                           GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT,
                           tex.dwWidth, tex.dwHeight, 0, tex.dwDataSize, tex.pData);
    break;
  case TextureType::Normal:
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, tex.dwWidth, tex.dwHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, tex.pData);
    break;
  default:
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, tex.dwWidth, tex.dwHeight, 0, GL_RED, GL_UNSIGNED_BYTE, tex.pData);
  }
  glGenerateMipmap(GL_TEXTURE_2D);
  label(GL_TEXTURE, id, name);
  endGroup();
  glCheck();
  return id;
}
u32 loadImageHDR(char const* filename, void const* data, u32 w, u32 h) {
#if BUILD_DEVELOPMENT
  std::string_view name{ baseName(filename) };
#endif

  u32 id;
  beginGroup(name);
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  setupTexture(GL_CLAMP_TO_EDGE, GL_LINEAR, true);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, data);
  label(GL_TEXTURE, id, name);
  endGroup();
  glCheck();
  return id;
}
#endif

#define DEBUG_VIEWS() \
  E(None), \
  E(Depth), \
  E(Stencil), \
  E(Normals), \
  E(Albedo), \
  E(Metallic), \
  E(Roughness), \
  E(AO), \
  E(SSAO), \
  E(Bloom)

enum class DebugView : i32 {
#define E(x) x
  DEBUG_VIEWS()
#undef E
};

static char const* debugViews[]{
#define E(x) #x
  DEBUG_VIEWS()
#undef E
};

static DebugView debugView;
static u32 debugViewProgR;
static u32 debugViewProgRG;
static u32 debugViewProgRGB;
static u32 debugViewProgG;
static u32 debugViewProgB;
static u32 debugViewProgA;

static i32 maxColorAttachments;
static i32 maxDrawBuffers;
static i32 maxVertexTextures;
static i32 maxFragmentTextures;
static i32 maxVertexUBOs;
static i32 maxFragmentUBOs;
static i32 maxTextureLayers;
static i32 maxTextureSize;
static i32 maxTextureSize3D;
static i32 maxTextureSizeCube;
static i32 maxRenderbufferSize;
static i32 maxBindingsUBO;
static i32 uboMaxSize;
static i32 uboOffsetAlignment;

static u32 uboModel;
static u32 uboMaterial;

constexpr u32 numFloorMats = 1;
constexpr u32 numIblTexs = 1;

static bool drawWireframe = false;
static bool drawShadowmap = true;
static bool drawBloom = true;
static bool drawDeferred = true;
static f32 bloomFactor = 0.5;
static bool drawSSAO = true;
static bool drawGizmos = false;

static i32 floorIdx;
static i32 iblIdx;

static u32 whiteTex;
static u32 blackTex;

static bool hasInit;
static u32 vaoScreen;
static u32 progPostHDR, progPostLDR;
static u32 fboHDR, texHDR, texBright, rboDepth;
static u32 fboLDR, texLDR;
static u32 fboScratch, texScratch;
static u32 vaoTriangle;
static u32 vboTriangle;
static u32 triangleProg, prog, texLoc, projLoc;
static u32 vao;
static u32 bufs[2];
static GLuint fontTexture;
static float r{0}, g{0}, b{0};

static u32 gBufferFBO;
static u32 gBufferDepthStencil;       // Depth24_Stencil8
static u32 gBufferNormals;            // RGB10_A2
static u32 gBufferAlbedoSpecular;     // SRGBA8
static u32 gBufferMaterialProperties; // RGBA8
//static u32 gBufferIrradiance;        // R11G11B10

static u32 ssaoNoise;
static u32 ssaoProg;
static u32 ssaoFBO;
static u32 ssaoTex;
static u32 ssaoSamplesUBO;
static u32 lightingProg;
static f32 ssaoRadius = 0.5;
static f32 ssaoBias = 0.025;

f32 lerp(f32 a, f32 b, f32 f) {
  return a + f * (b - a);
}

struct alignas(256) CameraBuffer {
  rtm::vector4f position;
  float exposure;
};

struct alignas(256) MaterialBuffer {
  rtm::float3f baseColor;
  float metallic;
  rtm::float3f emissiveColor;
  float roughness;
  rtm::float3f refractiveIndex;
  float ao;
  float specular;
};

struct LightBuffer {
  rtm::vector4f position;
  rtm::vector4f color;
};

constexpr u32 cameraOffset = 0;
constexpr u32 lightsOffset = cameraOffset + 256;

static f32 targetExposure = 1;
static f32 currentExposure = 1;

#if BUILD_EDITOR
static u32 progGizmoShadow;
static u32 progGizmoForward;
static u32 progGizmoDeferred;
static u32 progGizmo;
static u32 vboGizmo;
static u32 iboGizmo;
static u32 vaoGizmo;
struct Gizmo {
  u32 indexOffset;
  u32 indexCount;
  u32 vertexStart;
};
static Gizmo boneGizmo;
static Gizmo sphereGizmo;
static Gizmo cubeGizmo;
#endif

static u32 envTex[numIblTexs];
static u32 irradianceTex[numIblTexs];
static u32 prefilterTex[numIblTexs];
static u32 brdfIntegrationTex;
static u32 skyboxProgForward;
static u32 skyboxProgDeferred;

union Mat4 {
  rtm::matrix4x4f m4x4;
  rtm::matrix3x4f m3x4;
  float a[4][4];
  float v[16];
};

constexpr i32 numModels = 1;

static u32 progBlur[2];
static u32 fboBlur[2];
static u32 texBlur[2];
static u32 fboBlurSSAO[2];
static u32 texBlurSSAO[2];
static u32 uboTest;
static u32 uboBones;
static Model models[numModels];
static Skeleton skeleton;
static Animation animation;

constexpr i32 spherePaletteSize = 10;
constexpr i32 numSpheres = 20;
constexpr i32 numCubes = 10;

static Material spherePaletteMaterials[spherePaletteSize * spherePaletteSize];
static Material sphereMaterials[numSpheres];
static Material cubeMaterials[numCubes];

struct AnimState {
  struct Layer {
    u32 rotationKey = 0;
    u32 positionKey = 0;
    u32 scaleKey = 0;
  };

  float time = 0.0;
  Layer* layers;
  Mat4* localJoints;
  Mat4* worldJoints;
  Mat4* finalJoints; // TODO: remove, map UBO and write directly
};

static AnimState animState;

constexpr bool floorUseDisp = false;
static Material floorMat[numFloorMats];
static u32 floorProgForward[numFloorMats];
static u32 floorProgDeferred[numFloorMats];
static Model floorModel;

constexpr u32 shadowSize = 4096;

static f32 time;

// Vertex Locations:
// - 0 position
// - 1 normal
// - 2 tangent
// - 3 uv
// - 4 bone weights
// - 5 bone indices
// Uniform Locations:
// - 0 localToClip
// - 1 localToWorld
// - 2 baseTex
// - 3 normalTex
// - 4 metallicTex
// - 5 roughnessTex
// - 6 aoTex
// - 7 displacementTex
// - 8 irradiance
// - 9 light matrix
// - 10 lightTex
// - 11 prefilter
// - 12 brdf lut
// Uniform Bindings:
// - 0 camera
// - 1 material
// - 2 lights
// - 3 skeleton joints

constexpr auto vertexMeshSource =
"struct MeshVertexParams {\n"
"  vec4 localPosition;\n"
"  mat3 localToWorldRotation;\n"
"};\n";

constexpr auto staticVertexSource =
"void setupVertex(out MeshVertexParams p) {\n"
"  p.localPosition = vec4(vertexPosition, 1);\n"
"  p.localToWorldRotation = mat3(localToWorld);\n"
"}\n";

constexpr auto skinnedVertexSource =
"layout (location = 4) in vec4 vertexBoneWeights;\n"
"layout (location = 5) in ivec4 vertexBoneIndices;\n"
"layout (binding = 3) uniform Skeleton {\n"
"  mat4 bones[256];\n"
"};\n"
"void setupVertex(out MeshVertexParams p) {\n"
"  mat4 boneTransform =\n"
"    bones[vertexBoneIndices[0]] * vertexBoneWeights[0] +\n"
"    bones[vertexBoneIndices[1]] * vertexBoneWeights[1] +\n"
"    bones[vertexBoneIndices[2]] * vertexBoneWeights[2] +\n"
"    bones[vertexBoneIndices[3]] * vertexBoneWeights[3];\n"
"  p.localPosition = boneTransform * vec4(vertexPosition, 1.0);\n"
"  p.localToWorldRotation = mat3(localToWorld) * mat3(boneTransform);\n"
" }\n";

constexpr auto tonemapUncharted2Source =
"vec3 tonemapUncharted2(vec3 x) {\n"
"  const float A = 0.15;\n"
"  const float B = 0.50;\n"
"  const float C = 0.10;\n"
"  const float D = 0.20;\n"
"  const float E = 0.02;\n"
"  const float F = 0.30;\n"
"  return ((x * (x * A + B * C) + D * E) / (x * (x * A + B) + D * F)) - E / F;\n"
"}\n"
"vec3 tonemap(vec3 color, float exposure) {\n"
"  return tonemapUncharted2(color * exposure) * (1.0 / tonemapUncharted2(vec3(11.2)));\n"
"}\n";

constexpr char const* texVertexSources[]{
  // Base
  "",
  // Normal
  "layout (location = 1) in vec3 vertexTangent0;\n"
  "layout (location = 2) in vec3 vertexTangent1;\n"
  "out vec3 worldTangent0;\n"
  "out vec3 worldTangent1;\n"
  "void setupNormal(MeshVertexParams p) {\n"
  "  worldTangent0 = p.localToWorldRotation * vertexTangent0;\n"
  "  worldTangent1 = p.localToWorldRotation * vertexTangent1;\n"
  "}\n",
  // Metallic
  "",
  // Roughness
  "",
  // AO
  "",
  // Displacement
  ""
};

constexpr char const* f32VertexSources[]{
  // Base
  "",
  // Normal
  "layout (location = 1) in vec3 vertexNormal;\n"
  "out vec3 worldNormal;\n"
  "void setupNormal(MeshVertexParams p) {\n"
  "  worldNormal = p.localToWorldRotation * vertexNormal;\n"
  "}\n",
  // Metallic
  "",
  // Roughness
  "",
  // AO
  "",
  // Displacement
  ""
};

constexpr char const* texFragmentSources[]{
  // Base
  "layout (location = 2) uniform sampler2D baseColorTex;\n"
  "vec3 getBaseColor() { return texture2D(baseColorTex, uv).rgb; }\n",
  // Normal
  "in vec3 worldTangent0;\n"
  "in vec3 worldTangent1;\n"
  "layout (location = 3) uniform sampler2D normalTex;\n"
  "vec3 getNormal() {\n"
  "  vec3 t0 = normalize(worldTangent0);\n"
  "  vec3 t1 = normalize(worldTangent1 - dot(worldTangent1, t0) * t0);\n"
  "  vec3 t2 = cross(t0, t1);\n"
  "  mat3 tbn = mat3(t1, t2, t0);\n"
//"  vec3 nm = vec3(texture2D(normalTex, uv).rg * 2.0 - 1.0, 0);\n"
//"  nm.z = sqrt(1.0 - dot(nm.xy, nm.xy));\n"
  "  vec3 nm = texture2D(normalTex, uv).rgb;\n"
  "  nm = nm * 2.0 - 1.0;\n"
  "  return tbn * nm;\n"
  "}\n",
  // Metallic
  "layout (location = 4) uniform sampler2D metallicTex;\n"
  "float getMetallic() { return texture2D(metallicTex, uv).r; }\n",
  // Roughness
  "layout (location = 5) uniform sampler2D roughnessTex;\n"
  "float getRoughness() { return texture2D(roughnessTex, uv).r; }\n",
  // AO
  "layout (location = 6) uniform sampler2D aoTex;\n"
  "float getAO() { return texture2D(aoTex, uv).r; }\n",
  // Displacement
  ""
};

constexpr char const* f32FragmentSources[]{
  // Base
  "vec3 getBaseColor() { return material.baseColorMetallic.rgb; }\n",
  // Normal
  "in vec3 worldNormal;\n"
  "vec3 getNormal() { return worldNormal; }\n",
  // Metallic
  "float getMetallic() { return material.baseColorMetallic.a; }\n",
  // Roughness
  "float getRoughness() { return material.emissiveColorRoughness.a; }\n",
  // AO
  "float getAO() { return material.refractiveIndexAO.a; }\n",
  // Displacement
  ""
};

constexpr auto testHullSource =
"layout (vertices = 3) out;\n"
"in vec3 worldPosition[];\n"
"in vec3 worldTangent0[];\n"
"in vec3 worldTangent1[];\n"
"in vec2 uv[];\n"
"out vec3 worldPositionTess[];\n"
"out vec3 worldTangent0Tess[];\n"
"out vec3 worldTangent1Tess[];\n"
"out vec2 uvTess[];\n"
"float getTessLevel(float distance0, float distance1) {\n"
"  float avg = (distance0 + distance1) * 0.5;\n"
"  if (avg <= 2.0) return 15.0;\n"
"  if (avg <= 5.0) return 10.0;\n"
"  if (avg <= 10.0) return 5.0;\n"
"  return 2.0;\n"
"}\n"
"void main() {\n"
"  worldPositionTess[gl_InvocationID] = worldPosition[gl_InvocationID];\n"
"  worldTangent0Tess[gl_InvocationID] = worldTangent0[gl_InvocationID];\n"
"  worldTangent1Tess[gl_InvocationID] = worldTangent1[gl_InvocationID];\n"
"  uvTess[gl_InvocationID] = uv[gl_InvocationID];\n"
"  float dist0 = distance(camera.position, worldPositionTess[0]);\n"
"  float dist1 = distance(camera.position, worldPositionTess[1]);\n"
"  float dist2 = distance(camera.position, worldPositionTess[2]);\n"
"  gl_TessLevelOuter[0] = getTessLevel(dist1, dist2);\n"
"  gl_TessLevelOuter[1] = getTessLevel(dist2, dist0);\n"
"  gl_TessLevelOuter[2] = getTessLevel(dist0, dist1);\n"
"  gl_TessLevelInner[0] = gl_TessLevelOuter[2];\n"
"}\n";

// http://www.richardssoftware.net/2013/09/bump-and-displacement-mapping-with.html
constexpr auto testDomainSource =
"layout (triangles, fractional_odd_spacing, ccw) in;\n"
"in vec3 worldPositionTess[];\n"
"in vec3 worldTangent0Tess[];\n"
"in vec3 worldTangent1Tess[];\n"
"in vec2 uvTess[];\n"
"out vec3 worldPosition;\n"
"out vec3 worldTangent0;\n"
"out vec3 worldTangent1;\n"
"out vec2 uv;\n"
"layout (location = 7) uniform sampler2D dispTex;\n" // TODO mipmap
//"layout (location = 9) uniform float dispFactor;\n"
"const float dispFactor = 0.005;\n"
"layout (location = 0) uniform mat4 localToClip;\n" // TODO works for now because floor model matrix is identity!
"vec2 interpolate2D(vec2 v0, vec2 v1, vec2 v2) {\n"
"  return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;\n"
"}\n"
"vec3 interpolate3D(vec3 v0, vec3 v1, vec3 v2) {\n"
"  return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;\n"
"}\n"
"void main() {\n"
"  worldPosition = interpolate3D(worldPositionTess[0], worldPositionTess[1], worldPositionTess[2]);\n"
"  worldTangent0 = normalize(interpolate3D(worldTangent0Tess[0], worldTangent0Tess[1], worldTangent0Tess[2]));\n"
"  worldTangent1 = interpolate3D(worldTangent1Tess[0], worldTangent1Tess[1], worldTangent1Tess[2]);\n"
"  uv = interpolate2D(uvTess[0], uvTess[1], uvTess[2]);\n"
"  float disp = texture2D(dispTex, uv).x;\n"
"  worldPosition += worldTangent0 * (disp - 1.0) * dispFactor;\n"
"  gl_Position = localToClip * vec4(worldPosition, 1.0);\n"
"}\n";

auto shadowVertexSource0 =
"layout (location = 0) in vec3 vertexPosition;\n"
"layout (location = 0) uniform mat4 localToClip;\n"
"const mat4 localToWorld = mat4(1);\n";
auto shadowVertexSource1 =
"void main() {\n"
"  MeshVertexParams p;\n"
"  setupVertex(p);\n"
"  gl_Position = localToClip * p.localPosition;\n"
"}\n";

constexpr auto baseVertexSource0 =
"layout (location = 0) in vec3 vertexPosition;\n"
"layout (location = 3) in vec2 vertexUV;\n"
"layout (location = 0) uniform mat4 localToClip;\n"
"layout (location = 1) uniform mat4 localToWorld;\n"
"layout (location = 9) uniform mat4 worldToShadow;\n"
"out vec3 worldPosition;\n"
"out vec4 shadowCoord;\n"
"out vec2 uv;\n";
constexpr auto baseVertexSource1 =
"void main() {\n"
"  MeshVertexParams p;\n"
"  setupVertex(p);\n"
"  setupNormal(p);\n"
"  worldPosition = (localToWorld * p.localPosition).xyz;\n"
"  shadowCoord = (worldToShadow * vec4(worldPosition, 1));\n"
"  gl_Position = localToClip * p.localPosition;\n"
"  uv = vertexUV;\n"
"}\n";

constexpr auto cameraBufferSource =
"layout (binding = 0, std140) uniform Camera {\n"
"  vec3 position;\n"
"  float padding0;\n"
"  float exposure;\n"
"} camera;\n";

constexpr auto lightsBufferSource =
"struct LightData {\n"
"  vec3 position;\n"
"  float padding0;\n"
"  vec3 color;\n"
"  float padding1;\n"
"};\n"
"layout (binding = 2, std140) uniform Light {\n"
" LightData lights[2];\n"
"};\n";

constexpr auto gBufferOutputs =
"layout (location = 0) out vec3 gBufferNormal;\n"
"layout (location = 1) out vec4 gBufferAlbedoSpecular;\n"
"layout (location = 2) out vec4 gBufferMaterialProperties;\n";

constexpr auto baseFragmentSource0 =
"in vec3 worldPosition;\n"
"in vec4 shadowCoord;\n"
"in vec2 uv;\n"
"layout (location = 10) uniform sampler2D shadowTex;\n"
"layout (binding = 1, std140) uniform Material {\n"
"  vec4 baseColorMetallic;\n"
"  vec4 emissiveColorRoughness;\n"
"  vec4 refractiveIndexAO;\n"
"  float specular;\n"
"} material;\n"
"float getShadow(vec3 normal) {\n"
"  float bias = max(0.05 * (1.0 - dot(normal, normalize(-lights[0].position))), 0.005);\n"
"  vec3 coord = shadowCoord.xyz / shadowCoord.w * .5 + .5;\n"
"  float depth = texture2D(shadowTex, coord.xy).r;\n"
"  return depth < (coord.z - bias) ? 0.0 : 1.0;\n"
"}\n";

constexpr auto baseFragmentSourceDeferred =
"void main() {\n"
"  vec3 normal = normalize(getNormal());\n"
"  gBufferNormal = normal * 0.5 + 0.5;\n"
"  gBufferAlbedoSpecular = vec4(getBaseColor(), 0.5);\n"
"  gBufferMaterialProperties = vec4(getMetallic(), getRoughness(), getAO(), getShadow(normal));\n"
"}\n";
constexpr auto baseFragmentSourceForward =
"void main() {\n"
"  LightingData data;\n"
"  data.worldPosition = worldPosition;\n"
"  data.worldNormal = getNormal();\n"
"  data.albedo = getBaseColor();\n"
"  data.metallic = getMetallic();\n"
"  data.roughness = getRoughness();\n"
"  data.ao = getAO();\n"
"  data.occlusion = getShadow(data.worldNormal);\n"
"  lighting(data);\n"
"}\n";

constexpr char const* lightingOutput =
"layout (location = 0) out vec3 fragColor;\n"
"layout (location = 1) out vec3 brightColor;\n"
"void setupBrightness() {\n"
"  float brightness = dot(fragColor, vec3(0.2126, 0.7152, 0.0722));\n"
"  brightColor = brightness > 1 ? fragColor : vec3(0);\n"
"}\n";

constexpr char const* distributionGGX =
"float distributionGGX(float ndoth, float roughness) {\n"
"  float a  = roughness * roughness;\n"
"  float a2 = a * a;\n"
"  float d  = ndoth * ndoth * (a2 - 1.0) + 1.0;\n"
"  return a2 / max(PI * d * d, 0.001);\n"
"}\n";

constexpr char const* geometrySmithDirect =
"const float roughnessBias = 1.0;\n"
"const float roughnessScale = 8.0;\n";
constexpr char const* geometrySmith =
"float geometrySchlickGGX(float ndotv, float k) {\n"
"  return ndotv / (ndotv * (1.0 - k) + k);\n"
"}\n"
"float geometrySmith(float ndotl, float ndotv, float roughness) {\n"
"  float r = roughness + roughnessBias;\n"
"  float k = (r * r) / roughnessScale;\n"
"  return geometrySchlickGGX(ndotl, k) * geometrySchlickGGX(ndotv, k);\n"
"}\n";

constexpr char const* lightingFragmentSource0 =
"layout (location = 8) uniform samplerCube irradianceTex;\n"
"layout (location = 11) uniform samplerCube prefilterTex;\n"
"layout (location = 12) uniform sampler2D brdfLUT;\n"
"vec3 fresnelSchlick(float cosTheta, vec3 f0) {\n"
"  return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);\n"
"}\n"
"vec3 fresnelSchlickRoughness(float cosTheta, vec3 f0, float roughness) {\n"
"  return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(1.0 - cosTheta, 5.0);\n"
"}\n"
"struct LightingData {\n"
"  vec3 worldPosition;\n"
"  vec3 worldNormal;\n"
"  vec3 albedo;\n"
"  float metallic;\n"
"  float roughness;\n"
"  float ao;\n"
"  float occlusion;\n"
"};\n"
"void lighting(LightingData data) {\n"
"  vec3 v = normalize(camera.position - data.worldPosition);\n"
"  float ndotv = max(dot(data.worldNormal, v), 0.0);\n"
"  vec3 f0 = mix(vec3(0.04), data.albedo, data.metallic);\n"
"  vec3 lo = vec3(0.0);\n"
"  float oneMinusMetallic = 1.0 - data.metallic;\n"
"  for (int i = 0; i < 1; i++) {\n"
"    vec3 light = lights[i].position - data.worldPosition;\n"
"    vec3 l = normalize(light);\n" // radiance
"    vec3 h = normalize(v + l);\n"
"    float distance = length(light);\n"
"    float attenuation = 1.0 / (distance * distance);\n"
"    vec3 radiance = lights[i].color * attenuation;\n"
"    float ndoth = max(dot(data.worldNormal, h), 0.0);\n"
"    float ndf = distributionGGX(ndoth, data.roughness);\n" // cook-torrance
"    float ndotl = max(dot(data.worldNormal, l), 0.0);\n"
"    float g = geometrySmith(ndotl, ndotv, data.roughness);\n"
"    vec3 f = fresnelSchlick(max(dot(h, v), 0.0), f0);\n"
"    float d = 4 * ndotl * ndotv;\n"
"    vec3 specular = (ndf * g * f) / max(d, 0.001);\n"
"    vec3 kd = (vec3(1.0) - f) * oneMinusMetallic;\n"
"    lo += (kd * data.albedo + specular) * radiance * ndotl;\n"
"  }\n"
"  vec3 f = fresnelSchlickRoughness(ndotv, f0, data.roughness);\n"
"  vec3 kd = 1.0 - f;\n" // diffuse irradiance
"  vec3 irradiance = textureCube(irradianceTex, data.worldNormal).rgb;\n"
"  vec3 diffuse = irradiance * data.albedo;\n"
"  vec3 r = reflect(-v, data.worldNormal);\n" // reflectance
"  const float maxReflectionLod = 7;\n"
"  vec3 prefilterColor = textureLod(prefilterTex, r, data.roughness * maxReflectionLod).rgb;\n"
"  vec2 envBRDF = texture2D(brdfLUT, vec2(ndotv, data.roughness)).rg;\n"
"  vec3 specular = prefilterColor * (f * envBRDF.x + envBRDF.y);\n"
"  vec3 ambient = (kd * diffuse + specular) * data.ao;\n" // final color
"  vec3 color = ambient + lo * data.occlusion;\n"
"  fragColor = color;\n"
"  setupBrightness();\n"
"}\n";

constexpr auto worldPosFromDepth =
"in vec2 texCoord;\n"
"layout (location = 0) uniform sampler2D depthTex;\n"
"layout (location = 4) uniform mat4 clipToWorld;\n"
"float getWindowDepth() {\n"
"  return texture2D(depthTex, texCoord).r;\n"
"}\n"
"vec3 getWorldPosition(float depth) {\n"
"  vec4 clipPosition = vec4(texCoord, depth, 1.0) * 2.0 - 1.0;\n"
"  vec4 homogenousPosition = clipToWorld * clipPosition;\n"
"  return homogenousPosition.xyz / homogenousPosition.w;\n"
"}\n";

constexpr auto lightingFragmentSource1 =
"layout (location = 1) uniform sampler2D normalsTex;\n"
"layout (location = 2) uniform sampler2D albedoSpecularTex;\n"
"layout (location = 3) uniform sampler2D materialPropertiesTex;\n"
"layout (location = 13) uniform sampler2D ssaoTex;\n"
"void main() {\n"
"  LightingData data;\n"
"  vec4 albedoSpecular = texture2D(albedoSpecularTex, texCoord);\n"
"  data.albedo = albedoSpecular.rgb;\n"
"  float windowDepth = getWindowDepth();\n"
"  if (windowDepth == 1.0) {\n"
"    fragColor = data.albedo;\n"
"  }\n"
"  else {\n"
"  data.worldPosition = getWorldPosition(windowDepth);\n"
"  data.worldNormal = texture2D(normalsTex, texCoord).rgb * 2 - 1;\n"
"  vec4 materialProperties = texture2D(materialPropertiesTex, texCoord);\n"
"  data.metallic = materialProperties.r;\n"
"  data.roughness = materialProperties.g;\n"
"  data.ao = materialProperties.b * texture2D(ssaoTex, texCoord).r;\n"
"  data.occlusion = materialProperties.a;\n"
"  lighting(data);\n"
"  }\n"
"}\n";

static u32 shaderMap[1 << 10];

static u32 getShader(Material const& material, bool skinned, bool displaced, bool shadow, bool deferred) {
  u32 id = 0;
  for (auto tex{ 0 }; tex < textureTypeCount; tex++) {
    if (material.textures[tex] != 0) {
      id |= 1 << tex;
    }
  }

  if (skinned)   id |= 1 << (textureTypeCount + 0);
  if (displaced) id |= 1 << (textureTypeCount + 1);
  if (shadow)    id |= 1 << (textureTypeCount + 2);
  if (deferred)  id |= 1 << (textureTypeCount + 3);
  ASSERT_EX(id < _countof(shaderMap));

  if (shaderMap[id] == 0) {
  #if BUILD_DEVELOPMENT
    auto type0{ shadow ? "Shadow" : deferred ? "Deferred" : "Forward" };
    auto type1{ skinned ? " Skinned" : "" };
    auto type2{ displaced ? " Displaced" : "" };
  #endif

    beginGroup("Program %s%s%s [%02x]", type0, type1, type2, id);
    auto prog = glCreateProgram();
    ShaderBuilder vert;
    ShaderBuilder frag;

    if (shadow) {
      vert.push(shadowVertexSource0);
    }
    else {
      vert.push(baseVertexSource0);
      frag.push(lightsBufferSource);
      frag.push(cameraBufferSource);
      frag.push(baseFragmentSource0);
    }

    vert.push(vertexMeshSource);
    vert.push(skinned ? skinnedVertexSource : staticVertexSource);

    if (shadow) {
      vert.push(shadowVertexSource1);
    }
    else {
      for (auto tex{ 0 }; tex < textureTypeCount; tex++) {
        if (material.textures[tex] != 0) {
          vert.push(texVertexSources[tex]);
          frag.push(texFragmentSources[tex]);
        }
        else {
          vert.push(f32VertexSources[tex]);
          frag.push(f32FragmentSources[tex]);
        }
      }
      vert.push(baseVertexSource1);
      if (deferred) {
        frag.push(gBufferOutputs);
        frag.push(baseFragmentSourceDeferred);
      }
      else {
        frag.push(lightingOutput);
        frag.push(distributionGGX);
        frag.push(geometrySmithDirect);
        frag.push(geometrySmith);
        frag.push(lightingFragmentSource0);
        frag.push(baseFragmentSourceForward);
      }
    }

    if (displaced) {
      ShaderBuilder hull;
      ShaderBuilder domain;

      hull.push(cameraBufferSource);
      hull.push(testHullSource);
      domain.push(testDomainSource);

      auto hullShader{ hull.compile(GL_TESS_CONTROL_SHADER) };
      auto domainShader{ domain.compile(GL_TESS_EVALUATION_SHADER) };
      glAttachShader(prog, hullShader);
      glAttachShader(prog, domainShader);
      label(GL_SHADER, hullShader,   "%s%s%s [%02x] Hull", type0, type1, type2, id);
      label(GL_SHADER, domainShader, "%s%s%s [%02x] Domain", type0, type1, type2, id);
    }

    auto vertShader{ vert.compile(GL_VERTEX_SHADER) };
    glAttachShader(prog, vertShader);
    label(GL_SHADER, vertShader, "%s%s%s [%02x] Vertex", type0, type1, type2, id);

    if (!shadow) {
      auto fragShader{ frag.compile(GL_FRAGMENT_SHADER) };
      glAttachShader(prog, fragShader);
      label(GL_SHADER, fragShader, "%s%s%s [%02x] Fragment", type0, type1, type2, id);
    }

    glLinkProgram(prog);
    glCheckProgram(prog);
    glUseProgram(prog);
    label(GL_PROGRAM, prog, "%s%s%s [%02x] Program", type0, type1, type2, id);

    for (auto tex{ 0 }; tex < textureTypeCount; tex++) {
      if (material.textures[tex] != 0) {
        glUniform1i(tex + 2, tex);
      }
    }
    if (!shadow) {
      if (!deferred) {
        glUniform1i(8, 6);
        glUniform1i(11, 8);
        glUniform1i(12, 9);
      }
      glUniform1i(10, 7);
    }
    endGroup();
    glCheck();

    shaderMap[id] = prog;
  }
  return shaderMap[id];
}

static Material shadowMat;

void setupModel(Model* model, MaterialBuffer* materialBuffers, u32* materialIndex,
                f32 metallic = 0, f32 roughness = 1)
{
  materialBuffers[*materialIndex].emissiveColor = { 0, 0,0 };
  materialBuffers[*materialIndex].metallic = metallic;
  materialBuffers[*materialIndex].roughness = roughness;
  materialBuffers[*materialIndex].ao = 1;

  model->progsForward = new u32[model->numMaterials];
  model->progsDeferred = new u32[model->numMaterials];
  model->shadowProg = getShader(shadowMat, model->isSkinned, false, true, false);
  for (auto n{ 0 }; n < model->numMaterials; n++) {
    model->progsForward[n] = getShader(model->materials[n], model->isSkinned, false, false, false);
    model->progsDeferred[n] = getShader(model->materials[n], model->isSkinned, false, false, true);
    model->materials[n].uboIndex = *materialIndex;
  }
  (*materialIndex)++;
}

static u32 fboShadow;
static u32 texShadow;

constexpr i32 velocityIterations = 6;
constexpr i32 positionIterations = 2;

static b2World world{{0, -10}};
static b2Body* groundBody;
static b2Body* body;
static u32 mvpPos;

static rtm::vector4f cameraPosition = rtm::vector_set(0.f, 3.5f, -10, 1.f);
static f32 cameraYaw;
static f32 cameraPitch;

static u32 currentShader;

static Mat4 lookAt(rtm::vector4f eye, rtm::vector4f at, rtm::vector4f up) {
  auto c{ rtm::vector_sub(at, eye) };
  c = rtm::vector_normalize3(c, c);
  auto a{ rtm::vector_cross3(up, c) };
  auto b{ rtm::vector_cross3(c, a) };
  a = rtm::vector_normalize3(a, a);
  b = rtm::vector_normalize3(b, b);

  auto x{ rtm::vector_dot3(a, eye) };
  auto y{ rtm::vector_dot3(b, eye) };
  auto z{ rtm::vector_dot3(c, eye) };

  Mat4 ret;
  ret.m4x4 = rtm::matrix_set(
    rtm::vector_set(-rtm::vector_get_x(a), rtm::vector_get_x(b), -rtm::vector_get_x(c)),
    rtm::vector_set(-rtm::vector_get_y(a), rtm::vector_get_y(b), -rtm::vector_get_y(c)),
    rtm::vector_set(-rtm::vector_get_z(a), rtm::vector_get_z(b), -rtm::vector_get_z(c)),
    rtm::vector_set(x, y, z, 1)
  );
  return ret;
}

static Mat4 projOrtho(f32 b, f32 t, f32 l, f32 r, f32 n = -1.f, f32 f = 1.f) {
  auto A{  2.f / (r - l) };
  auto B{  2.f / (t - b) };
  auto C{ -2.f / (f - n) };
  auto X{ (r + l) / (l - r) };
  auto Y{ (t + b) / (b - t) };
  auto Z{ (f + n) / (n - f) };
  Mat4 ret;
  ret.m4x4 = rtm::matrix_set(
    rtm::vector_set( A, 0, 0, 0 ),
    rtm::vector_set( 0, B, 0, 0 ),
    rtm::vector_set( 0, 0, C, 0 ),
    rtm::vector_set( X, Y, Z, 1 )
  );
  return ret;
}

static Mat4 projPerspective(float fov, float aspect, float n, float f) {
  Mat4 ret;
  auto r = n - f;
  auto t = std::tanf(fov * .5f);
  auto x = 1.f / (t * aspect);
  auto y = 1.f / t;
  auto a = (-n - f) / r;
  auto b = 2 * f * n / r;
  ret.m4x4 = rtm::matrix_set(rtm::vector_set(x, 0, 0, 0),
                             rtm::vector_set(0, y, 0, 0),
                             rtm::vector_set(0, 0, a, 1),
                             rtm::vector_set(0, 0, b, 0));
  return ret;
}

static void drawSubMeshDepth(MeshHeader::SubMesh const& sm, u32 shader, Mat4 const& mvp) {
  if (currentShader != shader) {
    currentShader = shader;
    glUseProgram(shader);
    glUniformMatrix4fv(0, 1, false, mvp.v);
  }

  glDrawElementsBaseVertex(GL_TRIANGLES,
                           sm.indexCount, GL_UNSIGNED_INT,
                           reinterpret_cast<void const*>(sm.indexOffset),
                           sm.vertexStart);
}

static void drawSubMesh(MeshHeader::SubMesh const& sm, Material const& material,
                        u32 shader, Mat4 const& mvp, Mat4 const& model, Mat4 const& light,
                        bool patches = false)
{
  if (currentShader != shader) {
    currentShader = shader;
    glUseProgram(shader);
    glUniformMatrix4fv(0, 1, false, mvp.v);
    glUniformMatrix4fv(1, 1, false, model.v);
    glUniformMatrix4fv(9, 1, false, light.v);
  }

  for (auto tex{ 0 }; tex < textureTypeCount; tex++) {
    if (material.textures[tex] != 0) {
      glActiveTexture(GL_TEXTURE0 + tex);
      glBindTexture(GL_TEXTURE_2D, material.textures[tex]);
    }
  }

  glBindBufferRange(GL_UNIFORM_BUFFER, 1, uboMaterial,
                    sizeof(MaterialBuffer) * material.uboIndex,
                    sizeof(MaterialBuffer));
  glDrawElementsBaseVertex(patches ? GL_PATCHES : GL_TRIANGLES,
                           sm.indexCount, GL_UNSIGNED_INT,
                           reinterpret_cast<void const*>(sm.indexOffset),
                           sm.vertexStart);
}

static void drawGizmo(Gizmo const& gizmo) {
  glDrawElementsBaseVertex(GL_TRIANGLES, gizmo.indexCount, GL_UNSIGNED_INT,
                           reinterpret_cast<void const*>(gizmo.indexOffset),
                           gizmo.vertexStart);
}

static void drawBones(Mat4& model, Mat4& view, Mat4* bones) {
  for (auto n = 0; n < skeleton.numBones; n++) {
    Mat4 modelToView;
    modelToView.m3x4 = rtm::matrix_mul(bones[n].m3x4,
                                       rtm::matrix_mul(model.m3x4, view.m3x4));
    marker("Bone Gizmo %d", n);
    glUniformMatrix4fv(1, 1, false, modelToView.v);
    drawGizmo(sphereGizmo);
  }
}

constexpr u32 cubeSize = 1024;
constexpr u32 irradianceSize = 32;
constexpr u32 prefilterSize = 128;
constexpr u32 brdfIntegrationSize = 512;

static GLenum formatFromInternal(GLint format) {
  switch (format) {
  case GL_R8:
  case GL_R8_SNORM:
  case GL_R8I:
  case GL_R8UI:
  case GL_R16:
  case GL_R16_SNORM:
  case GL_R16F:
  case GL_R16I:
  case GL_R16UI:
  case GL_R32F:
  case GL_R32I:
  case GL_R32UI:
    return GL_RED;
  case GL_RG8:
  case GL_RG8_SNORM:
  case GL_RG8I:
  case GL_RG8UI:
  case GL_RG16:
  case GL_RG16_SNORM:
  case GL_RG16F:
  case GL_RG16I:
  case GL_RG16UI:
  case GL_RG32F:
  case GL_RG32I:
  case GL_RG32UI:
    return GL_RG;
  case GL_RGB4:
  case GL_RGB5:
  case GL_RGB8:
  case GL_RGB8_SNORM:
  case GL_RGB8I:
  case GL_RGB8UI:
  case GL_RGB10:
  case GL_RGB12:
  case GL_RGB16:
  case GL_RGB16_SNORM:
  case GL_RGB16F:
  case GL_RGB16I:
  case GL_RGB16UI:
  case GL_RGB32F:
  case GL_RGB32I:
  case GL_RGB32UI:
  case GL_SRGB8:
  case GL_R3_G3_B2:
  case GL_R11F_G11F_B10F:
  case GL_RGB565:
  case GL_RGB9_E5:
    return GL_RGB;
  case GL_RGBA2:
  case GL_RGBA4:
  case GL_RGBA8:
  case GL_RGBA8_SNORM:
  case GL_RGBA8I:
  case GL_RGBA8UI:
  case GL_RGBA12:
  case GL_RGBA16:
  case GL_RGBA16_SNORM:
  case GL_RGBA16F:
  case GL_RGBA16I:
  case GL_RGBA16UI:
  case GL_RGBA32F:
  case GL_RGBA32I:
  case GL_RGBA32UI:
  case GL_SRGB8_ALPHA8:
  case GL_RGB5_A1:
  case GL_RGB10_A2:
  case GL_RGB10_A2UI:
    return GL_RGBA;
  case GL_DEPTH24_STENCIL8:
    return GL_DEPTH_STENCIL;
  case GL_DEPTH_COMPONENT16:
  case GL_DEPTH_COMPONENT24:
  case GL_DEPTH_COMPONENT32F:
    return GL_DEPTH_COMPONENT;
  default:
    ASSERT(0, "Unhandled format 0x%04x in formatFromInternal", format);
    UNREACHABLE;
  }
}

static GLenum typeFromInternal(GLint format) {
  // TODO sync with formatFromInternal
  switch (format) {
  case GL_R8:
  case GL_R16:
  case GL_RG8:
  case GL_RG16:
  case GL_RGB8:
  case GL_RGB16:
  case GL_RGBA8:
  case GL_RGBA16:
    return GL_UNSIGNED_BYTE;
  case GL_R16F:
  case GL_R32F:
  case GL_RG16F:
  case GL_RG32F:
  case GL_RGB16F:
  case GL_RGB32F:
  case GL_RGBA16F:
  case GL_RGBA32F:
    return GL_FLOAT;
  case GL_R3_G3_B2:         return GL_UNSIGNED_BYTE_2_3_3_REV;
  case GL_R11F_G11F_B10F:   return GL_UNSIGNED_INT_10F_11F_11F_REV;
  case GL_RGB565:           return GL_UNSIGNED_SHORT_5_6_5_REV;
  case GL_RGB5_A1:          return GL_UNSIGNED_SHORT_1_5_5_5_REV;
  case GL_RGB10_A2:
  case GL_RGB10_A2UI:       return GL_UNSIGNED_INT_2_10_10_10_REV;
  case GL_DEPTH24_STENCIL8: return GL_UNSIGNED_INT_24_8;
  default:
    ASSERT(0, "Unhandled format 0x%04x in typeFromInternal", format);
    UNREACHABLE;
  }
}

static f32 currentWidth;
static f32 currentHeight;

struct RenderTarget {
  u32 tex;
  i32 width;
  i32 height;
  i32 internalFormat;
  f32 viewportScale;
  bool isBuffer;
};

std::vector<RenderTarget> renderTargets;

static void resize(RenderTarget& target, u32 w, u32 h) {
  target.width = static_cast<i32>(w * target.viewportScale);
  target.height = static_cast<i32>(h * target.viewportScale);
  if (target.isBuffer) {
    glBindRenderbuffer(GL_RENDERBUFFER, target.tex);
    glRenderbufferStorage(GL_RENDERBUFFER, target.internalFormat,
                          target.width, target.height);
  }
  else {
    glBindTexture(GL_TEXTURE_2D, target.tex);
    glTexImage2D(GL_TEXTURE_2D, 0, target.internalFormat,
                 target.width, target.height, 0,
                 formatFromInternal(target.internalFormat),
                 typeFromInternal(target.internalFormat),
                 nullptr);
  }
}

static u32 createRenderTarget(StringView name, i32 internalFormat,
                              u32 width, u32 height,
                              u32 filter = GL_NEAREST,
                              u32 wrap = GL_CLAMP_TO_EDGE,
                              f32 viewportScale = 1)
{
  RenderTarget rt;
  rt.internalFormat = internalFormat;
  rt.viewportScale  = viewportScale;
  rt.isBuffer       = false;
  glGenTextures(1, &rt.tex);
  resize(rt, width, height);
  setupTexture(wrap, filter, false);
  label(GL_TEXTURE, rt.tex, name);
  glCheck();
  renderTargets.push_back(rt);
  return rt.tex;
}

static u32 createRenderBuffer(StringView name, i32 internalFormat,
                              u32 width, u32 height,
                              f32 viewportScale = 1)
{
  RenderTarget rt;
  rt.internalFormat = internalFormat;
  rt.viewportScale  = viewportScale;
  rt.isBuffer       = true;
  glGenRenderbuffers(1, &rt.tex);
  resize(rt, width, height);
  label(GL_RENDERBUFFER, rt.tex, name);
  glCheck();
  renderTargets.push_back(rt);
  return rt.tex;
}

static void resize(u32 w, u32 h) {
  LOG(Test, Info, "Resize to %ux%u", w, h);
  beginGroup("Resize"sv);
  for (auto& target : renderTargets) {
    if (target.viewportScale > 0) {
      resize(target, w, h);
    }
  }
  endGroup();
  glCheck();
}

#include "app/windows/WindowsMain.hpp"
#include "app/windows/WindowsWindow.hpp"
#include "gpu/opengl/OpenGL.hpp"

void renderMain() {
  //auto gl{ reinterpret_cast<OpenGL*>(arg) };
  auto vp = App::Windows::Main::getMainWindow()->getViewport();
  auto gl = static_cast<Gpu::OpenGL::Viewport*>(vp);

#if PLATFORM_IPHONE
  auto fboSurface{ gl->getSurface() };
#else
  constexpr u32 fboSurface = 0;
#endif

  gl->makeCurrent();

  if (!hasInit) {
    hasInit = true;

  #if PLATFORM_APPLE
    pthread_setname_np("Render");
  #elif PLATFORM_POSIX
    pthread_setname_np(pthread_self(), "Render");
  #endif

  #if PLATFORM_IPHONE
    glCheckFramebuffer();
  #endif

    LOG(Test, Info, "OpenGL Version %s", glGetString(GL_VERSION));
    LOG(Test, Info, "OpenGL Vendor: %s", glGetString(GL_VENDOR));
    LOG(Test, Info, "OpenGL Renderer: %s", glGetString(GL_RENDERER));

    // TODO better detection, usage
  #if PLATFORM_ANDROID || PLATFORM_IPHONE || PLATFORM_HTML5
    isGLES = true;
  #endif

    u16 glVersion;
    {
      i32 major, minor;
      glGetIntegerv(GL_MAJOR_VERSION, &major);
      glGetIntegerv(GL_MINOR_VERSION, &minor);

      switch (glGetError()) {
      default:
        glCheck();
        UNREACHABLE;

      case GL_NO_ERROR:
        glVersion = glMakeVersion(major, minor);
        break;

      case GL_INVALID_ENUM:
        auto str{ reinterpret_cast<char const*>(glGetString(GL_VERSION)) };
        auto len{ strlen(str) };

        // NOTE: assuming "x.y" format: no versions are double digits
        if (len < 3 || str[1] != '.' || !isdigit(str[0]) || !isdigit(str[2])) {
          abort(); // TODO fatal error handling
        }
        glVersion = glMakeVersion(str[0] - '0', str[2] - '0');
        break;
      }
    }

  #if PLATFORM_LINUX || PLATFORM_WINDOWS
  # define GL(type, name) gl##name = reinterpret_cast<PFNGL##type##PROC>(gl->getProcAddress("gl" #name))
  #  if PLATFORM_WINDOWS
    GL1_2_PROCS;
    GL1_3_PROCS;
    GL1_4_PROCS;
  #  endif
    GL1_5_PROCS;
    GL2_0_PROCS;
    if (glVersion >= GL3_0) GL3_0_PROCS;
    if (glVersion >= GL3_1) GL3_1_PROCS;
    if (glVersion >= GL3_2) GL3_2_PROCS;
    if (glVersion >= GL3_3) GL3_3_PROCS;
    if (glVersion >= GL4_3) GL4_3_PROCS;
  # undef GL
  #endif

    if (glVersion < GL3_0) {
      auto str{ reinterpret_cast<char const*>(glGetString(GL_EXTENSIONS)) };
      for (;;) {
        auto end{ strchr(str, ' ') };
        if (!end) {
          setupExtension({ str });
          break;
        }

        setupExtension({ str, static_cast<usize>(end - str) });
        str = end + 1;
      }
    }
    else {
      i32 numExts;
      glGetIntegerv(GL_NUM_EXTENSIONS, &numExts);

      for (auto i{ 0 }; i < numExts; i++) {
        auto str{ reinterpret_cast<char const*>(glGetStringi(GL_EXTENSIONS, i)) };
        setupExtension({ str });
      }
    }

  #if BUILD_DEBUG
    glDebugMessageCallback(debugCallback, nullptr);
  #endif

    {
      auto str{ reinterpret_cast<char const*>(glGetString(GL_SHADING_LANGUAGE_VERSION)) };
      LOG(Test, Info, "GLSL Version: %s", str);
    }

    if (isGLES) {
      switch (glVersion) {
      #define V(gl, glsl) case GLES##gl: glslHeader = "#version " #glsl " es\n"; break
      V(2_0, 100); default:
        V(3_0, 300);
      #undef V
      }
    }
    else {
      switch (glVersion) {
      #define V(gl, glsl) case GL##gl: glslHeader = "#version " #glsl "\n"; break
        V(2_0, 110);
        V(2_1, 120);
        V(3_0, 130);
        V(3_1, 140);
        V(3_2, 150);
        V(3_3, 330);
        V(4_0, 400);
        V(4_1, 410);
        V(4_2, 420);
        V(4_3, 430);
        V(4_4, 440);
      V(4_5, 450); default:
        V(4_6, 460);
      #undef V
      }
    }

    if (exts.ARB_explicit_attrib_location) {
      glslHeader += "#extension GL_ARB_explicit_attrib_location : require\n";
    }

    if (exts.ARB_explicit_uniform_location) {
      glslHeader += "#extension GL_ARB_explicit_uniform_location : require\n";
    }

    if (isGLES) {
      glslHeader += "precision mediump float;\n";
    }

    if (glVersion >= GL3_0) {
      glslHeader += "#define texture1D texture\n";
      glslHeader += "#define texture2D texture\n";
      glslHeader += "#define texture3D texture\n";
      glslHeader += "#define textureCube texture\n";
      glslHeader += "#define texture2DLod textureLod;\n";
      glslHeader += "#define texture3DLod textureLod;\n";
      glslHeader += "#define textureCubeLod textureLod;\n"; // FIXME: NV driver crash when used
    }

    glslHeader += "const float PI = 3.1415926535897932384626433832795;\n";
    glslHeader += "const float HALF_PI = 1.57079632679489661923;\n";
    glslHeader += "const float PI_2 = 6.28318530718;\n";

    b2BodyDef groundBodyDef;
    groundBodyDef.position.Set(0, -12);
    groundBody = world.CreateBody(&groundBodyDef);

    b2PolygonShape groundBox;
    groundBox.SetAsBox(50, 10);
    groundBody->CreateFixture(&groundBox, 0);

    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(0, 1);
    body = world.CreateBody(&bodyDef);

    b2PolygonShape dynamicBox;
    dynamicBox.SetAsBox(1, 1);
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &dynamicBox;
    fixtureDef.density = 1;
    fixtureDef.friction = .3f;
    body->CreateFixture(&fixtureDef);

    jank_imgui_init();

    auto& io{ ImGui::GetIO() };
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.SetClipboardTextFn = jank_imgui_setClipboardText;
    io.GetClipboardTextFn = jank_imgui_getClipboardText;

    beginGroup("Setup"sv);
    glDisable(GL_MULTISAMPLE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    glFrontFace(GL_CW);

    int intValue;
    glGetIntegerv(GL_MAX_DEBUG_MESSAGE_LENGTH, &intValue);
    maxDebugMessageLength = intValue;

    glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &maxColorAttachments);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxVertexTextures);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxFragmentTextures);
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_BLOCKS, &maxVertexUBOs);
    glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_BLOCKS, &maxFragmentUBOs);
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxTextureLayers);
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &maxTextureSize3D);
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &maxTextureSizeCube);
    glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &maxRenderbufferSize);
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &uboMaxSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uboOffsetAlignment);

    // 14 UBOs per stage
    // -----------------
    // - Scene  (time, number, ...)
    // - Camera (worldToView, viewToClip, worldToClip, inverses, position, ...)
    // - Object
    //   - Shadow (localToLight)
    //   - Base (localToWorld, localToClip, materialIndex, ...)
    // - Shadow (worldToLight)
    // - Material
    //   - Variant (shadingModel, albedo, emissive, specular, metallic,
    //              roughness, ao, positionOffset, displacement, tessFactor,
    //              subsurfaceColor, refraction, opacity, opacityMask)
    // - Light
    //   - Base (position, color, maskTexture)
    //   - Directional (direction)
    //   - Spot (direction, angle)
    //   - Point ()
    // - PostProcess
    //   - Bloom (factor)

    glGenBuffers(1, &uboModel);
    glGenBuffers(1, &uboMaterial);
    glBindBuffer(GL_UNIFORM_BUFFER, uboModel);
    glBindBuffer(GL_UNIFORM_BUFFER, uboMaterial);
    label(GL_BUFFER, uboModel, "Model UBO");
    label(GL_BUFFER, uboMaterial, "Material UBO");
    glCheck();

    MaterialBuffer materialBuffers[256];
    auto materialIndex{ 0u };

    // System Textures
    // ------------------------------------------------------------------------

    beginGroup("System Textures"sv);
    u8 whitePixel[]{ 0xFF, 0xFF, 0xFF };
    glGenTextures(1, &whiteTex);
    glBindTexture(GL_TEXTURE_2D, whiteTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, whitePixel);
    setupTexture(GL_CLAMP_TO_EDGE, GL_NEAREST, false);

    u8 blackPixel[]{ 0x00, 0x00, 0x00 };
    glGenTextures(1, &blackTex);
    glBindTexture(GL_TEXTURE_2D, blackTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, blackPixel);
    setupTexture(GL_CLAMP_TO_EDGE, GL_NEAREST, false);

    label(GL_TEXTURE, whiteTex, "White"sv);
    label(GL_TEXTURE, blackTex, "Black"sv);
    endGroup();

    // FBO
    // ------------------------------------------------------------------------

    beginGroup("G-Buffer"sv);
    gBufferDepthStencil       = createRenderTarget("G-Buffer Depth-Stencil"sv, GL_DEPTH24_STENCIL8, gl->width, gl->height);
    gBufferNormals            = createRenderTarget("G-Buffer Normals"sv, GL_RGB10_A2, gl->width, gl->height);
    gBufferAlbedoSpecular     = createRenderTarget("G-Buffer Albedo/Specular"sv, GL_RGBA8, gl->width, gl->height);
    gBufferMaterialProperties = createRenderTarget("G-Buffer Material"sv, GL_RGBA8, gl->width, gl->height);
    glGenFramebuffers(1, &gBufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gBufferFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, gBufferDepthStencil, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gBufferNormals, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gBufferAlbedoSpecular, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gBufferMaterialProperties, 0);
    {
      u32 attachments[]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
      glDrawBuffers(_countof(attachments), attachments);
    }
    label(GL_FRAMEBUFFER, gBufferFBO, "G-Buffer FBO"sv);
    glCheckFramebuffer();
    endGroup();
    glCheck();

    beginGroup("HDR"sv);
    rboDepth = createRenderBuffer("Depth RBO"sv, GL_DEPTH_COMPONENT24, gl->width, gl->height);
    texHDR = createRenderTarget("HDR Texture"sv, GL_RGB16F, gl->width, gl->height);
    texBright = createRenderTarget("Brightness Texture"sv, GL_RGB16F, gl->width, gl->height, GL_LINEAR);
    glGenFramebuffers(1, &fboHDR);
    glBindFramebuffer(GL_FRAMEBUFFER, fboHDR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texHDR, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, texBright, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    {
      u32 attachments[]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
      glDrawBuffers(_countof(attachments), attachments);
    }
    label(GL_FRAMEBUFFER, fboHDR, "HDR FBO"sv);
    glCheckFramebuffer();
    endGroup();
    glCheck();

    beginGroup("LDR"sv);
    texLDR = createRenderTarget("LDR Texture"sv, GL_RGB8, gl->width, gl->height);
    glGenFramebuffers(1, &fboLDR);
    glBindFramebuffer(GL_FRAMEBUFFER, fboLDR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texLDR, 0);
    label(GL_FRAMEBUFFER, fboLDR, "LDR FBO"sv);
    glCheckFramebuffer();
    endGroup();
    glCheck();

    beginGroup("Scratch"sv);
    texScratch = createRenderTarget("Scratch FBO"sv, GL_RGB8, gl->width, gl->height);
    glGenFramebuffers(1, &fboScratch);
    glBindFramebuffer(GL_FRAMEBUFFER, fboScratch);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texScratch, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    label(GL_TEXTURE, texScratch, "Scratch Texture"sv);
    glCheckFramebuffer();
    endGroup();
    glCheck();

    beginGroup("Blur"sv);
    texBlur[0] = createRenderTarget("Blur Texture"sv, GL_RGB16F, gl->width, gl->height, GL_LINEAR);
    glGenFramebuffers(2, fboBlur);
    glBindFramebuffer(GL_FRAMEBUFFER, fboBlur[0]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texBlur[0], 0);
    glCheckFramebuffer();

    texBlur[1] = texBright;
    glBindFramebuffer(GL_FRAMEBUFFER, fboBlur[1]);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texBlur[1], 0);
    label(GL_FRAMEBUFFER, fboBlur[0], "BlurH FBO"sv);
    label(GL_FRAMEBUFFER, fboBlur[1], "BlurV FBO"sv);
    glCheckFramebuffer();
    endGroup();
    glCheck();
    LOG(Test, Info, "Framebuffer");

    // PostProcess
    // ------------------------------------------------------------------------

    // TODO:
    // - Better bloom blur (pyramid FBos)
    // - Motion Blur
    // - Depth of Field
    // - Color Grading
    // - Vignette
    // - Screen Space Reflections
    // - Chromatic Abberation
    // - Anti-Aliasing
    // - Eye Adaptation
    // - Lens Flare

    auto screenVertexSource =
      "out vec2 texCoord;\n"
      "void main() {\n"
      "  vec2 pos = vec2(-1.0 + float((gl_VertexID & 1) << 2),\n"
      "                  -1.0 + float((gl_VertexID & 2) << 1));\n"
      "  texCoord = (pos + 1.0) * 0.5;\n"
      "  gl_Position = vec4(pos, 0, 1);\n"
      "}\n";

    auto ppSource =
      "in vec2 texCoord;\n"
      "layout (location = 0) out vec3 fragColor;\n"
      "layout (location = 0) uniform sampler2D screenTex;\n"
      "vec3 getScreenColor(vec2 uv) { return texture2D(screenTex, uv).rgb; }\n";
    auto ppHDRSource =
      "layout (location = 1) uniform sampler2D bloomTex;\n"
      "layout (location = 2) uniform float bloomFactor;\n"
      "void main() {\n"
      "  vec3 bloom = texture2D(bloomTex, texCoord).rgb * 0.25;\n"
      "  fragColor = tonemap(getScreenColor(texCoord) + bloom * bloomFactor, 3.0);\n"
      "}\n";
    auto ppLDRSource =
      "layout (location = 1) uniform float time;\n"
      "void main() {\n"
      "  vec2 uv = texCoord;\n"
      //"  uv.x += sin(texCoord.y * PI * time) / 100;\n"
      "  fragColor = pow(getScreenColor(uv), vec3(1.0/2.2));\n"
      "}\n";

    beginGroup("PostProcess"sv);
    auto ppVert{ glBuildShader(GL_VERTEX_SHADER, screenVertexSource) };
    auto hdrFrag{ glBuildShader(GL_FRAGMENT_SHADER, ppSource, tonemapUncharted2Source, ppHDRSource) };
    auto ldrFrag{ glBuildShader(GL_FRAGMENT_SHADER, ppSource, ppLDRSource) };
    label(GL_SHADER, ppVert, "ScreenTriangle Vertex"sv);
    label(GL_SHADER, hdrFrag, "HDR Fragment"sv);
    label(GL_SHADER, ldrFrag, "LDR Fragment"sv);
    glCheck();

    progPostHDR = glBuildProgram(ppVert, hdrFrag);
    glUniform1i(0, 0);
    glUniform1i(1, 1);
    label(GL_PROGRAM, progPostHDR, "PostProcess(HDR) Program"sv);
    glCheck();

    progPostLDR = glBuildProgram(ppVert, ldrFrag);
    glUniform1i(0, 0);
    label(GL_PROGRAM, progPostLDR, "PostProcess(LDR) Program"sv);
    glCheck();

    glGenVertexArrays(1, &vaoScreen);
    glBindVertexArray(vaoScreen);
    label(GL_VERTEX_ARRAY, vaoScreen, "ScreenTriangle VAO"sv);
    glBindVertexArray(0);
    endGroup();
    glCheck();

    // Debug View
    // ------------------------------------------------------------------------

    {
      auto dbg =
        "in vec2 texCoord;\n"
        "layout (location = 0) out vec3 fragColor;\n"
        "layout (location = 0) uniform sampler2D tex;\n"
        "vec4 getColor() { return texture2D(tex, texCoord); }\n";
      auto rSrc = "void main() { fragColor = getColor().rrr; }\n";
      auto rgSrc = "void main() { fragColor = vec3(getColor().rg, 0); }\n";
      auto rgbSrc = "void main() { fragColor = getColor().rgb; }\n";
      auto gSrc = "void main() { fragColor = getColor().ggg; }\n";
      auto bSrc = "void main() { fragColor = getColor().bbb; }\n";
      auto aSrc = "void main() { fragColor = getColor().aaa; }\n";

      auto r{ glBuildShader(GL_FRAGMENT_SHADER, dbg, rSrc) };
      auto rg{ glBuildShader(GL_FRAGMENT_SHADER, dbg, rgSrc) };
      auto rgb{ glBuildShader(GL_FRAGMENT_SHADER, dbg, rgbSrc) };
      auto g{ glBuildShader(GL_FRAGMENT_SHADER, dbg, gSrc) };
      auto b{ glBuildShader(GL_FRAGMENT_SHADER, dbg, bSrc) };
      auto a{ glBuildShader(GL_FRAGMENT_SHADER, dbg, aSrc) };
      label(GL_SHADER, r, "Debug View Fragment (R)"sv);
      label(GL_SHADER, rg, "Debug View Fragment (RG)"sv);
      label(GL_SHADER, rgb, "Debug View Fragment (RGB)"sv);
      label(GL_SHADER, g, "Debug View Fragment (G)"sv);
      label(GL_SHADER, b, "Debug View Fragment (B)"sv);
      label(GL_SHADER, a, "Debug View Fragment (A)"sv);

      debugViewProgR = glBuildProgram(ppVert, r); glUniform1i(0, 0);
      debugViewProgRG = glBuildProgram(ppVert, rg); glUniform1i(0, 0);
      debugViewProgRGB = glBuildProgram(ppVert, rgb); glUniform1i(0, 0);
      debugViewProgG = glBuildProgram(ppVert, g); glUniform1i(0, 0);
      debugViewProgB = glBuildProgram(ppVert, b); glUniform1i(0, 0);
      debugViewProgA = glBuildProgram(ppVert, a); glUniform1i(0, 0);
      label(GL_PROGRAM, debugViewProgR, "Debug View Program (R)"sv);
      label(GL_PROGRAM, debugViewProgRG, "Debug View Program (RG)"sv);
      label(GL_PROGRAM, debugViewProgRGB, "Debug View Program (RGB)"sv);
      label(GL_PROGRAM, debugViewProgG, "Debug View Program (G)"sv);
      label(GL_PROGRAM, debugViewProgB, "Debug View Program (B)"sv);
      label(GL_PROGRAM, debugViewProgA, "Debug View Program (A)"sv);
    }


    // SSAO
    // ------------------------------------------------------------------------

    {
      beginGroup("SSAO"sv);
      std::uniform_real_distribution<f32> random{ 0, 1 };
      std::default_random_engine generator;

      {
        union V {
          rtm::vector4f _4;
          rtm::float2f _2;
        } v;
        rtm::float2f noise[16];
        for (auto n{ 0 }; n < _countof(noise); n++) {
          v._4 = rtm::vector_set(random(generator) * 2 - 1,
                                 random(generator) * 2 - 1,
                                 0);
          v._4 = rtm::vector_normalize3(v._4, v._4);
          noise[n] = v._2;
        }

        glGenTextures(1, &ssaoNoise);
        glBindTexture(GL_TEXTURE_2D, ssaoNoise);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 4, 4, 0, GL_RG, GL_FLOAT, &noise[0].x);
        setupTexture(GL_REPEAT, GL_NEAREST, false);
        label(GL_TEXTURE, ssaoNoise, "SSAO Noise"sv);
        glCheck();
      }

      {
        union V {
          rtm::vector4f _4;
          rtm::float4f _3;
        } v;
        rtm::float4f kernel[64];
        for (auto n{ 0 }; n < _countof(kernel); n++) {
          auto scale{ static_cast<f32>(n) / _countof(kernel) };
          scale = lerp(0.1f, 1.0f, scale * scale);
          v._4 = rtm::vector_set(random(generator) * 2 - 1,
                                 random(generator) * 2 - 1,
                                 random(generator));
          v._4 = rtm::vector_normalize3(v._4, v._4);
          v._4 = rtm::vector_mul(v._4, random(generator) * scale);
          kernel[n] = v._3;
        }

        glGenBuffers(1, &ssaoSamplesUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, ssaoSamplesUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(kernel), &kernel[0].x, GL_STATIC_DRAW);
        label(GL_BUFFER, ssaoSamplesUBO, "SSAO Samples"sv);
        glCheck();
      }

      ssaoTex = createRenderTarget("SSAO Texture"sv, GL_R8, gl->width, gl->height, GL_LINEAR);
      glGenFramebuffers(1, &ssaoFBO);
      glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTex, 0);
      label(GL_FRAMEBUFFER, ssaoFBO, "SSAO FBO"sv);
      label(GL_TEXTURE, ssaoTex, "SSAO Texture"sv);
      glCheckFramebuffer();
      glCheck();

      auto ssaoFragSource =
        "layout (location = 0) out float ssao;\n"
        "layout (location = 1) uniform sampler2D gBufferNormals;\n"
        "layout (location = 2) uniform sampler2D noiseTex;\n"
        "layout (location = 3) uniform mat4 proj;\n"
        "layout (location = 5) uniform vec2 noiseScale;\n"
        "layout (location = 6) uniform float radius;\n"
        "layout (location = 7) uniform float bias;\n"
        "layout (location = 8) uniform mat4 worldToView;\n"
        "layout (binding = 4, std140) uniform Samples {\n"
        "  vec4 samples[64];\n"
        "};\n"
        "const int kernelSize = 64;\n"
        "const float falloff = 0.000001;\n"
        "void main() {\n"
        "  vec3 origin = getWorldPosition(getWindowDepth());\n"
        "  vec3 normal = mat3(worldToView) * normalize(texture2D(gBufferNormals, texCoord).xyz * 2 - 1);\n"
        "  vec3 noise = vec3(texture2D(noiseTex, texCoord * noiseScale).xy, 0);\n"
        "  vec3 tangent = normalize(noise - normal * dot(noise, normal));\n"
        "  vec3 bitangent = cross(normal, tangent);\n"
        "  mat3 tbn = mat3(tangent, bitangent, normal);\n"
        "  float occlusion = 0;\n"
        "  for (int i = 0; i < kernelSize; i++) {\n"
        "    vec3 position = (tbn * samples[i].xyz) * radius + origin;\n"
        "    vec4 offset = proj * vec4(position, 1.0);\n"
        "    offset.xy = (offset.xy / offset.w) * 0.5 + 0.5;\n"
        "    float depth = getWorldPosition(texture2D(depthTex, offset.xy).x).z;\n"
        //"    float rangeCheck = smoothstep(falloff, 1.0, radius / abs(p.z - depth));\n"
        "float rangeCheck = abs(origin.z - depth) < radius ? 1 : 0;\n"
        "    occlusion += (depth <= position.z + bias ? 1.0 : 0.0) * rangeCheck;\n"
        //"    if (abs(p.z - depth) < radius) { occlusion += step(depth, p.z); }"
        "  }\n"
        "  ssao = 1 - occlusion / kernelSize;\n"
        "}\n";

      auto ssaoFrag{ glBuildShader(GL_FRAGMENT_SHADER, worldPosFromDepth, ssaoFragSource) };
      label(GL_SHADER, ssaoFrag, "SSAO Fragment"sv);

      ssaoProg = glBuildProgram(ppVert, ssaoFrag);
      glUniform1i(0, 0);
      glUniform1i(1, 1);
      glUniform1i(2, 2);
      label(GL_PROGRAM, ssaoProg, "SSAO Program"sv);

      fboBlurSSAO[0] = fboBlur[0];
      texBlurSSAO[0] = texBlur[0];
      texBlurSSAO[1] = ssaoTex;
      glGenFramebuffers(1, &fboBlurSSAO[1]);
      glBindFramebuffer(GL_FRAMEBUFFER, fboBlurSSAO[1]);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssaoTex, 0);
      endGroup();
      glCheck();
    }

    // Lighting
    // ------------------------------------------------------------------------

    beginGroup("Lighting"sv);
    auto lightingFrag{ glBuildShader(GL_FRAGMENT_SHADER,
                                     cameraBufferSource,
                                     lightsBufferSource,
                                     lightingOutput,
                                     distributionGGX,
                                     geometrySmithDirect,
                                     geometrySmith,
                                     worldPosFromDepth,
                                     lightingFragmentSource0,
                                     lightingFragmentSource1) };
    label(GL_SHADER, lightingFrag, "Lighting Fragment"sv);

    lightingProg = glBuildProgram(ppVert, lightingFrag);
    glUniform1i(0, 0);
    glUniform1i(1, 1);
    glUniform1i(2, 2);
    glUniform1i(3, 3);
    glUniform1i(8, 6);
    glUniform1i(11, 8);
    glUniform1i(12, 9);
    glUniform1i(13, 10);
    label(GL_PROGRAM, lightingProg, "Lighting Program"sv);
    endGroup();
    glCheck();


    // Blur
    // ------------------------------------------------------------------------

    auto blurHSource = "    vec2 offset = vec2(texOffset.x * i, 0.0);\n";
    auto blurVSource = "    vec2 offset = vec2(0.0, texOffset.y * i);\n";

    auto blurSource0 =
      "const float weight[5] = {0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216};"
      "void main() {\n"
      "  vec2 texOffset = 1.0 / textureSize(screenTex, 0);\n"
      "  vec3 color = getScreenColor(texCoord);\n"
      "  for (int i = 1; i < 5; i++) {\n";
    auto blurSource1 =
      "    color += texture2D(screenTex, texCoord + offset).rgb * weight[i];\n"
      "    color += texture2D(screenTex, texCoord - offset).rgb * weight[i];\n"
      "  }\n"
      "  fragColor = color;\n"
      "}\n";

    beginGroup("Blur"sv);
    auto blurFragH{ glBuildShader(GL_FRAGMENT_SHADER, ppSource, blurSource0, blurHSource, blurSource1) };
    auto blurFragV{ glBuildShader(GL_FRAGMENT_SHADER, ppSource, blurSource0, blurVSource, blurSource1) };
    label(GL_SHADER, blurFragH, "BlurH Frag"sv);
    label(GL_SHADER, blurFragV, "BlurV Frag"sv);
    glCheck();

    progBlur[0] = glBuildProgram(ppVert, blurFragH);
    glUniform1i(0, 1);

    progBlur[1] = glBuildProgram(ppVert, blurFragV);
    glUniform1i(0, 1);
    label(GL_PROGRAM, progBlur[0], "BlurH Program"sv);
    label(GL_PROGRAM, progBlur[1], "BlurV Program"sv);
    endGroup();
    glCheck();

    // Triangle init
    // ------------------------------------------------------------------------
    float vertices[]{
      -.5f, -.5f, 0,
      .5f, -.5f, 0,
      0, .5f, 0
    };

    beginGroup("Triangle"sv);
    glGenVertexArrays(1, &vaoTriangle);
    glBindVertexArray(vaoTriangle);
    label(GL_VERTEX_ARRAY, vaoTriangle, "Triangle VAO");
    glCheck();

    glGenBuffers(1, &vboTriangle);
    glBindBuffer(GL_ARRAY_BUFFER, vboTriangle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    label(GL_BUFFER, vboTriangle, "Triangle VBO"sv);
    glCheck();

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glCheck();
    LOG(Test, Info, "Triangle VAO");

    auto triangleVertexSource =
      "layout (location = 0) in vec3 pos;\n"
      "uniform mat4 mvp;\n"
      "out vec3 frag_col;\n"
      "void main() {\n"
      "  gl_Position = mvp * vec4(pos, 1);\n"
      "  frag_col = normalize(pos);\n"
      "}\n";

    auto triangleFragmentSource =
      "layout (location = 0) out vec3 col;\n"
      "in vec3 frag_col;\n"
      "void main() {\n"
      "  col = frag_col;\n"
      "}\n";

    auto triangleVert{ glBuildShader(GL_VERTEX_SHADER, triangleVertexSource) };
    auto triangleFrag{ glBuildShader(GL_FRAGMENT_SHADER, triangleFragmentSource) };
    label(GL_SHADER, triangleVert, "Triangle Vertex"sv);
    label(GL_SHADER, triangleFrag, "Triangle Fragment"sv);
    glCheck();

    triangleProg = glBuildProgram(triangleVert, triangleFrag);
    label(GL_PROGRAM, triangleProg, "Triangle Program"sv);
    glCheck();

    mvpPos = glGetUniformLocation(triangleProg, "mvp");
    endGroup();
    LOG(Test, Info, "Triangle Program");

    // Shadow Map
    // ------------------------------------------------------------------------

    beginGroup("Shadowmap"sv);
    glGenTextures(1, &texShadow);
    glBindTexture(GL_TEXTURE_2D, texShadow);
    setupTexture(GL_CLAMP_TO_EDGE, GL_LINEAR, false);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, shadowSize, shadowSize,
                 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glGenFramebuffers(1, &fboShadow);
    glBindFramebuffer(GL_FRAMEBUFFER, fboShadow);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texShadow, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glCheckFramebuffer();
    label(GL_TEXTURE, texShadow, "Shadow Texture"sv);
    label(GL_FRAMEBUFFER, fboShadow, "Shadow FBO"sv);
    endGroup();
    glCheck();


    // Floor
    // ------------------------------------------------------------------------

    {
      beginGroup("Floor"sv);
      auto filename = "../data/editor/gizmo/plane.obj";
      auto planeData = testMeshImport(filename);
      testMeshLoad(filename, planeData.data(), &floorModel);

      auto n{ 0u };
      if constexpr (numFloorMats >= 1) {
        floorMat[n].textures[static_cast<u32>(TextureType::Base)] = importTexture("../data/cc0textures/PavingStones10_col.jpg", TextureType::Base);
        floorMat[n].textures[static_cast<u32>(TextureType::Normal)] = importTexture("../data/cc0textures/PavingStones10_nrm.jpg", TextureType::Normal);
        floorMat[n].textures[static_cast<u32>(TextureType::Roughness)] = importTexture("../data/cc0textures/PavingStones10_rgh.jpg", TextureType::Roughness);
        floorMat[n].textures[static_cast<u32>(TextureType::AO)] = importTexture("../data/cc0textures/PavingStones10_AO.jpg", TextureType::AO);
        if constexpr (floorUseDisp) floorMat[n].textures[static_cast<u32>(TextureType::Displacement)] = importTexture("../data/cc0textures/PavingStones10_disp.jpg", TextureType::Displacement);
        materialBuffers[materialIndex].emissiveColor = { 0, 0, 0 };
        materialBuffers[materialIndex].metallic = 0;
        floorMat[n].uboIndex = materialIndex++;
        n++;
      }

      if constexpr (numFloorMats >= 2) {
        floorMat[n].textures[static_cast<u32>(TextureType::Base)] = importTexture("../data/cc0textures/Metal08_col.jpg", TextureType::Base);
        floorMat[n].textures[static_cast<u32>(TextureType::Normal)] = importTexture("../data/cc0textures/Metal08_nrm.jpg", TextureType::Normal);
        floorMat[n].textures[static_cast<u32>(TextureType::Metallic)] = importTexture("../data/cc0textures/Metal08_met.jpg", TextureType::Metallic);
        floorMat[n].textures[static_cast<u32>(TextureType::Roughness)] = importTexture("../data/cc0textures/Metal08_rgh.jpg", TextureType::Roughness);
        if constexpr (floorUseDisp) floorMat[n].textures[static_cast<u32>(TextureType::Displacement)] = importTexture("../data/cc0textures/Metal08_disp.jpg", TextureType::Displacement);
        materialBuffers[materialIndex].emissiveColor = { 0, 0, 0 };
        materialBuffers[materialIndex].ao = 1;
        floorMat[n].uboIndex = materialIndex++;
        n++;
      }

      if constexpr (numFloorMats >= 3) {
        floorMat[n].textures[static_cast<u32>(TextureType::Base)] = importTexture("../data/cc0textures/Tiles15_col.jpg", TextureType::Base);
        floorMat[n].textures[static_cast<u32>(TextureType::Normal)] = importTexture("../data/cc0textures/Tiles15_nrm.jpg", TextureType::Normal);
        floorMat[n].textures[static_cast<u32>(TextureType::Roughness)] = importTexture("../data/cc0textures/Tiles15_rgh.jpg", TextureType::Roughness);
        if constexpr (floorUseDisp) floorMat[n].textures[static_cast<u32>(TextureType::Displacement)] = importTexture("../data/cc0textures/Tiles15_disp.jpg", TextureType::Displacement);
        materialBuffers[materialIndex].emissiveColor = { 0, 0, 0 };
        materialBuffers[materialIndex].metallic = 0;
        materialBuffers[materialIndex].ao = 1;
        floorMat[n].uboIndex = materialIndex++;
        n++;
      }

      if constexpr (numFloorMats >= 4) {
        floorMat[n].textures[static_cast<u32>(TextureType::Base)] = importTexture("../data/cc0textures/Rock26_col.jpg", TextureType::Base);
        floorMat[n].textures[static_cast<u32>(TextureType::Normal)] = importTexture("../data/cc0textures/Rock26_nrm.jpg", TextureType::Normal);
        floorMat[n].textures[static_cast<u32>(TextureType::Roughness)] = importTexture("../data/cc0textures/Rock26_rgh.jpg", TextureType::Roughness);
        floorMat[n].textures[static_cast<u32>(TextureType::AO)] = importTexture("../data/cc0textures/Rock26_AO.jpg", TextureType::AO);
        if constexpr (floorUseDisp) floorMat[n].textures[static_cast<u32>(TextureType::Displacement)] = importTexture("../data/cc0textures/Rock26_disp.jpg", TextureType::Displacement);
        materialBuffers[materialIndex].emissiveColor = { 0, 0, 0 };
        floorMat[n].uboIndex = materialIndex++;
        n++;
      }

      if constexpr (numFloorMats >= 5) {
        floorMat[n].textures[static_cast<u32>(TextureType::Base)] = importTexture("../data/cc0textures/Metal22_col.jpg", TextureType::Base);
        floorMat[n].textures[static_cast<u32>(TextureType::Normal)] = importTexture("../data/cc0textures/Metal22_nrm.jpg", TextureType::Normal);
        floorMat[n].textures[static_cast<u32>(TextureType::Metallic)] = importTexture("../data/cc0textures/Metal22_met.jpg", TextureType::Metallic);
        floorMat[n].textures[static_cast<u32>(TextureType::Roughness)] = importTexture("../data/cc0textures/Metal22_rgh.jpg", TextureType::Roughness);
        if constexpr (floorUseDisp) floorMat[n].textures[static_cast<u32>(TextureType::Displacement)] = importTexture("../data/cc0textures/Metal08_disp.jpg", TextureType::Displacement);
        materialBuffers[materialIndex].emissiveColor = { 0, 0, 0 };
        materialBuffers[materialIndex].ao = 1;
        floorMat[n].uboIndex = materialIndex++;
        n++;
      }

      if constexpr (numFloorMats >= 6) {
        floorMat[n].textures[static_cast<u32>(TextureType::Base)] = importTexture("../data/cc0textures/PavingStones03_col.jpg", TextureType::Base);
        floorMat[n].textures[static_cast<u32>(TextureType::Normal)] = importTexture("../data/cc0textures/PavingStones03_nrm.jpg", TextureType::Normal);
        floorMat[n].textures[static_cast<u32>(TextureType::Roughness)] = importTexture("../data/cc0textures/PavingStones03_rgh.jpg", TextureType::Roughness);
        if constexpr (floorUseDisp) floorMat[n].textures[static_cast<u32>(TextureType::Displacement)] = importTexture("../data/cc0textures/PavingStones03_disp.jpg", TextureType::Displacement);
        materialBuffers[materialIndex].emissiveColor = { 0, 0, 0 };
        materialBuffers[materialIndex].metallic = 0;
        materialBuffers[materialIndex].ao = 1;
        floorMat[n].uboIndex = materialIndex++;
        n++;
      }

      for (auto n{ 0 }; n < numFloorMats; n++) {
        floorProgForward[n] = getShader(floorMat[n], false, floorUseDisp, false, false);
        floorProgDeferred[n] = getShader(floorMat[n], false, floorUseDisp, false, true);
      }
      endGroup();
    }


    // Gizmo
    // ------------------------------------------------------------------------

  #if BUILD_EDITOR
    auto gizmoVertexSource =
      "layout (location = 0) in vec3 vertexPosition;\n"
      "layout (location = 1) in vec3 vertexNormal;\n"
      "layout (location = 0) uniform mat4 viewToClip;\n"
      "layout (location = 1) uniform mat4 localToView;\n"
      "layout (location = 2) uniform vec3 color;\n"
      "out flat vec3 vertexColor;\n"
      "void main() {\n"
      "  const vec3 lightPosition = vec3(-1, -0.5, 1);\n"
      "  vec4 viewPosition = localToView * vec4(vertexPosition, 1);\n"
      "  vec3 n = normalize(mat3(localToView) * vertexNormal);\n"
      "  vec3 l = normalize(lightPosition - viewPosition.xyz);\n"
      "  vec3 diffuse = color * max(dot(n, l), 0.3) * 0.65;\n"
      "  vertexColor = pow(diffuse, vec3(1/2.2));\n"
      "  gl_Position = viewToClip * viewPosition;\n"
      "}\n";

    auto gizmoFragmentSource =
      "layout (location = 0) out vec3 fragColor;\n"
      "in vec3 vertexColor;\n"
      "void main() {\n"
      "  fragColor = vertexColor;\n"
      "}\n";

    beginGroup("Gizmo"sv);
    auto gizmoVert{ glBuildShader(GL_VERTEX_SHADER, gizmoVertexSource) };
    auto gizmoFrag{ glBuildShader(GL_FRAGMENT_SHADER, gizmoFragmentSource) };

    progGizmo = glBuildProgram(gizmoVert, gizmoFrag);
    label(GL_SHADER, gizmoVert, "Gizmo Vertex"sv);
    label(GL_SHADER, gizmoFrag, "Gizmo Fragment"sv);
    label(GL_PROGRAM, progGizmo, "Gizmo Program"sv);
    glCheck();

    {
      auto boneData = testMeshImportSimple("../data/editor/gizmo/bone.stl");
      auto sphereData = testMeshImportSimple("../data/editor/gizmo/sphere.obj");
      auto cubeData = testMeshImportSimple("../data/editor/gizmo/cube.stl");
      auto boneHeader = reinterpret_cast<MeshHeader const*>(boneData.data());
      auto sphereHeader = reinterpret_cast<MeshHeader const*>(sphereData.data());
      auto cubeHeader = reinterpret_cast<MeshHeader const*>(cubeData.data());
      auto boneMesh = reinterpret_cast<MeshHeader::SubMesh const*>(boneData.data() + sizeof(MeshHeader));
      auto sphereMesh = reinterpret_cast<MeshHeader::SubMesh const*>(sphereData.data() + sizeof(MeshHeader));
      auto cubeMesh = reinterpret_cast<MeshHeader::SubMesh const*>(cubeData.data() + sizeof(MeshHeader));
      auto indexOffset = sizeof(MeshHeader) + sizeof(MeshHeader::SubMesh);
      auto boneIndexSize = boneHeader->numIndices * 4;
      auto boneVertexSize = boneHeader->numVertices * 12;
      auto bonePositionOffset = boneData.data() + indexOffset + boneIndexSize;
      auto sphereIndexSize = sphereHeader->numIndices * 4;
      auto sphereVertexSize = sphereHeader->numVertices * 12;
      auto spherePositionOffset = sphereData.data() + indexOffset + sphereIndexSize;
      auto cubeIndexSize = cubeHeader->numIndices * 4;
      auto cubeVertexSize = cubeHeader->numVertices * 12;
      auto cubePositionOffset = cubeData.data() + indexOffset + cubeIndexSize;
      glGenVertexArrays(1, &vaoGizmo);
      glBindVertexArray(vaoGizmo);
      glGenBuffers(1, &vboGizmo);
      glGenBuffers(1, &iboGizmo);
      glBindBuffer(GL_ARRAY_BUFFER, vboGizmo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboGizmo);
      glBufferData(GL_ARRAY_BUFFER, boneVertexSize * 2 + sphereVertexSize * 2 + cubeVertexSize * 2,
                   nullptr, GL_STATIC_DRAW);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, boneIndexSize + sphereIndexSize + cubeIndexSize,
                   nullptr, GL_STATIC_DRAW);
      u32 offset = 0;
      glBufferSubData(GL_ARRAY_BUFFER, offset, boneVertexSize, bonePositionOffset);
      offset += boneVertexSize;
      glBufferSubData(GL_ARRAY_BUFFER, offset, sphereVertexSize, spherePositionOffset);
      offset += sphereVertexSize;
      glBufferSubData(GL_ARRAY_BUFFER, offset, cubeVertexSize, cubePositionOffset);
      offset += cubeVertexSize;
      glBufferSubData(GL_ARRAY_BUFFER, offset, boneVertexSize, bonePositionOffset + boneVertexSize);
      offset += boneVertexSize;
      glBufferSubData(GL_ARRAY_BUFFER, offset, sphereVertexSize, spherePositionOffset + sphereVertexSize);
      offset += sphereVertexSize;
      glBufferSubData(GL_ARRAY_BUFFER, offset, cubeVertexSize, cubePositionOffset + cubeVertexSize);
      offset = 0;
      glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, boneIndexSize, boneData.data() + indexOffset);
      offset += boneIndexSize;
      glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, sphereIndexSize, sphereData.data() + indexOffset);
      offset += sphereIndexSize;
      glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, cubeIndexSize, cubeData.data() + indexOffset);
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, nullptr);
      glVertexAttribPointer(1, 3, GL_FLOAT, false, 0,
                            reinterpret_cast<void const*>(boneVertexSize + sphereVertexSize + cubeVertexSize));
      glBindVertexArray(0);
      label(GL_VERTEX_ARRAY, vaoGizmo, "Gizmo VAO"sv);
      label(GL_BUFFER, vboGizmo, "Gizmo VBO"sv);
      label(GL_BUFFER, iboGizmo, "Gizmo IBO"sv);
      glCheck();

      boneGizmo.indexCount = boneMesh->indexCount;
      boneGizmo.indexOffset = 0;
      boneGizmo.vertexStart = 0;
      sphereGizmo.indexCount = sphereMesh->indexCount;
      sphereGizmo.indexOffset = boneIndexSize;
      sphereGizmo.vertexStart = boneHeader->numVertices;
      cubeGizmo.indexCount = cubeMesh->indexCount;
      cubeGizmo.indexOffset = sphereGizmo.indexOffset + sphereIndexSize;
      cubeGizmo.vertexStart = sphereGizmo.vertexStart + sphereHeader->numVertices;
    }
    endGroup();
  #endif

    // IBL
    // ------------------------------------------------------------------------

    auto cubeVertSource =
      "layout (location = 1) uniform mat4 viewProj;\n"
      "layout (location = 0) in vec3 vertexPosition;\n"
      "out vec3 localPosition;\n"
      "void main() {\n"
      "  localPosition = vertexPosition;\n"
      "  gl_Position = viewProj * vec4(vertexPosition, 1.0);\n"
      "}\n";
    auto cubeFragSource =
      "layout (location = 0) uniform sampler2D tex;\n"
      "layout (location = 0) out vec3 fragColor;\n"
      "in vec3 localPosition;\n"
      "vec2 sphericalUV(vec3 v) {\n"
      "  vec2 uv = vec2(atan(v.z, v.x), -asin(v.y));\n"
      "  uv *= vec2(0.1591, 0.3183);\n"
      "  uv += 0.5;\n"
      "  return uv;\n"
      "}\n"
      "void main() {\n"
      "  fragColor = texture2D(tex, sphericalUV(normalize(localPosition))).rgb;\n"
      "}\n";
    auto cubeIrradianceFragSource =
      "layout (location = 0) uniform samplerCube tex;\n"
      "layout (location = 0) out vec3 fragColor;\n"
      "in vec3 localPosition;\n"
      "void main() {\n"
      "  vec3 normal = normalize(localPosition);\n"
      "  vec3 right = cross(vec3(0, 1, 0), normal);\n"
      "  vec3 up = cross(normal, right);\n"
      "  float nrSamples = 0;\n"
      "  vec3 irradiance = vec3(0);\n"
      "  for (float phi = 0; phi < PI_2; phi += 0.025) {\n"
      "    for (float theta = 0; theta < HALF_PI; theta += 0.1) {\n"
      "      float sinTheta = sin(theta);\n"
      "      float cosTheta = cos(theta);\n"
      "      vec3 tangent = cos(phi) * right + sin(phi) * up;\n"
      "      vec3 sampleVec = cosTheta * normal + sinTheta * tangent;\n"
      //"      vec3 tangentSample = vec3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);\n"
      //"      vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;\n"
      "      irradiance += textureCube(tex, sampleVec).rgb * sinTheta * cosTheta;\n"
      "      nrSamples++;\n"
      "    }\n"
      "  }\n"
      "  fragColor = PI * irradiance * (1.0 / nrSamples);\n"
      "}\n";
    auto importanceSampleGGX =
      "float vanDerCorpus(uint n) {\n"
      "  n = (n << 16u) | (n >> 16u);\n"
      "  n = ((n & 0x55555555u) << 1u) | ((n & 0xAAAAAAAAu) >> 1u);\n"
      "  n = ((n & 0x33333333u) << 2u) | ((n & 0xCCCCCCCCu) >> 2u);\n"
      "  n = ((n & 0x0F0F0F0Fu) << 3u) | ((n & 0xF0F0F0F0u) >> 3u);\n"
      "  n = ((n & 0x00FF00FFu) << 4u) | ((n & 0xFF00FF00u) >> 4u);\n"
      "  return float(n) * 2.3283064365386963e-10;\n"
      "}\n"
      "vec2 hammersley(uint i, uint n) {\n"
      "  return vec2(float(i) / float(n), vanDerCorpus(i));\n"
      "}\n"
      "vec3 importanceSampleGGX(vec2 xi, vec3 n, float roughness) {\n"
      "  float a = roughness * roughness;\n"
      "  float phi = PI_2 * xi.x;\n"
      "  float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));\n"
      "  float sinTheta = sqrt(1.0 - cosTheta * cosTheta);\n"
      "  vec3 h = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);\n"
      "  vec3 up = abs(n.z) < 0.999 ? vec3(0, 0, 1) : vec3(1, 0, 0);\n"
      "  vec3 t = normalize(cross(up, n));\n"
      "  vec3 b = cross(n, t);\n"
      "  return normalize(t * h.x + b * h.y + n * h.z);\n"
      "}\n";
    auto cubePrefilterFragSource =
      "layout (location = 0) uniform samplerCube tex;\n"
      "layout (location = 2) uniform float roughness;\n"
      "layout (location = 0) out vec3 fragColor;\n"
      "in vec3 localPosition;\n"
      "void main() {\n"
      "  vec3 nrv = normalize(localPosition);\n" // normal, reflect, view vectors are all the same
      "  const uint sampleCount = 1024u;\n"
      "  float totalWeight = 0.0;\n"
      "  vec3 prefilterColor = vec3(0);\n"
      "  for (uint i = 0u; i < sampleCount; i++) {\n"
      "    vec2 xi = hammersley(i, sampleCount);\n"
      "    vec3 h = importanceSampleGGX(xi, nrv, roughness);\n"
      "    float hdot = dot(h, nrv);\n"
      "    vec3 l = normalize(2.0 * hdot * h - nrv);\n"
      "    float ldot = max(dot(l, nrv), 0);\n"
      "    if (ldot > 0) {\n"
      "      float d = distributionGGX(hdot, roughness);\n"
      "      float pdf = (d * hdot / (4 * hdot)) + 0.0001;\n"
      "      float resolution = float(textureSize(tex, 0).x);\n"
      "      float saTexel = 4 * PI / (6 * resolution * resolution);\n"
      "      float saSample = 1 / (float(sampleCount) * pdf + 0.0001);\n"
      "      float mipLevel = roughness == 0 ? 0 : 0.5 * log2(saSample / saTexel);\n"
      "      vec3 color = textureLod(tex, l, mipLevel).rgb * ldot;"
      "      prefilterColor += color;\n"
      "      totalWeight += ldot;\n"
      "    }\n"
      "  }\n"
      "  fragColor = vec3(prefilterColor / totalWeight);\n"
      "}\n";
    auto geometrySmithIBL =
      "const float roughnessBias = 0.0;\n"
      "const float roughnessScale = 2.0;\n";
    auto brdfIntegrationFragSource =
      "in vec2 texCoord;\n"
      "layout (location = 0) out vec2 integrated;\n"
      "void main() {\n"
      "  float ndotv = texCoord.x;\n"
      "  float roughness = texCoord.y;\n"
      "  float a = 0;\n"
      "  float b = 0;\n"
      "  vec3 v = vec3(sqrt(1.0 - ndotv * ndotv), 0.0, ndotv);\n"
      "  const vec3 n = vec3(0, 0, 1);\n"
      "  const uint sampleCount = 1024u;\n"
      "  for (uint i = 0u; i < sampleCount; i++) {\n"
      "    vec2 xi = hammersley(i, sampleCount);\n"
      "    vec3 h = importanceSampleGGX(xi, n, roughness);\n"
      "    float vdoth = dot(v, h);\n"
      "    vec3 l = normalize(2.0 * vdoth * h - v);\n"
      "    float ndotl = l.z;\n"
      "    if (ndotl > 0) {\n"
      "      vdoth = max(vdoth, 0);\n"
      "      float ndoth = max(h.z, 0);\n"
      "      float g = geometrySmith(ndotl, ndotv, roughness);\n"
      "      float gVis = (g * vdoth) / (ndoth * ndotv);\n"
      "      float fc = pow(1.0 - vdoth, 5.0);\n"
      "      a += (1.0 - fc) * gVis;\n"
      "      b += fc * gVis;\n"
      "    }\n"
      "  }\n"
      "  integrated = vec2(a / float(sampleCount), b / float(sampleCount));\n"
      "}\n";

    beginGroup("IBL"sv);
    auto cubeVert{ glBuildShader(GL_VERTEX_SHADER, cubeVertSource) };
    auto cubeFrag{ glBuildShader(GL_FRAGMENT_SHADER, cubeFragSource) };
    auto cubeIrradianceFrag{ glBuildShader(GL_FRAGMENT_SHADER, cubeIrradianceFragSource) };
    auto cubePrefilterFrag{ glBuildShader(GL_FRAGMENT_SHADER,
                                          importanceSampleGGX,
                                          distributionGGX,
                                          cubePrefilterFragSource) };
    auto brdfIntegrationFrag{ glBuildShader(GL_FRAGMENT_SHADER,
                                            importanceSampleGGX,
                                            distributionGGX,
                                            geometrySmithIBL,
                                            geometrySmith,
                                            brdfIntegrationFragSource) };
    label(GL_SHADER, cubeVert, "Cube Vertex"sv);
    label(GL_SHADER, cubeFrag, "Cube Fragment"sv);
    label(GL_SHADER, cubeIrradianceFrag, "Irradiance Fragment"sv);
    label(GL_SHADER, cubePrefilterFrag, "Prefilter Fragment"sv);
    label(GL_SHADER, brdfIntegrationFrag, "BRDF Integration Fragment"sv);

    char const* iblTextures[]{
      "../data/hdri/carpentry_shop_02_2k.hdr",
      "../data/hdri/old_tree_in_city_park_2k.hdr",
      "../data/hdri/entrance_hall_2k.hdr",
      "../data/hdri/the_sky_is_on_fire_2k.hdr",
      "../data/hdri/satara_night_2k.hdr",
      "../data/hdri/rooitou_park_2k.hdr"
    };

    auto cubeProj{ projPerspective(rtm::degrees(90.f).as_radians(), 1, .1f, 10).m4x4 };

    auto origin{ rtm::vector_set(0, 0, 0.f) };
    rtm::matrix4x4f cubeViews[]{
      lookAt(origin, rtm::vector_set(-1.f,  0,  0), rtm::vector_set(0.f, -1,  0)).m4x4,
      lookAt(origin, rtm::vector_set( 1.f,  0,  0), rtm::vector_set(0.f, -1,  0)).m4x4,
      lookAt(origin, rtm::vector_set( 0.f, -1,  0), rtm::vector_set(0.f,  0, -1)).m4x4,
      lookAt(origin, rtm::vector_set( 0.f,  1,  0), rtm::vector_set(0.f,  0,  1)).m4x4,
      lookAt(origin, rtm::vector_set( 0.f,  0,  1), rtm::vector_set(0.f, -1,  0)).m4x4,
      lookAt(origin, rtm::vector_set( 0.f,  0, -1), rtm::vector_set(0.f, -1,  0)).m4x4
    };

    Mat4 cubeViewProjs[]{
      rtm::matrix_mul(cubeViews[0], cubeProj),
      rtm::matrix_mul(cubeViews[1], cubeProj),
      rtm::matrix_mul(cubeViews[2], cubeProj),
      rtm::matrix_mul(cubeViews[3], cubeProj),
      rtm::matrix_mul(cubeViews[4], cubeProj),
      rtm::matrix_mul(cubeViews[5], cubeProj)
    };

    u32 cubeFBO;
    glGenFramebuffers(1, &cubeFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, cubeFBO);
    glBindVertexArray(vaoGizmo);
    label(GL_FRAMEBUFFER, cubeFBO, "Cube FBO");

    beginGroup("Env"sv);
    auto cubeProg{ glBuildProgram(cubeVert, cubeFrag) };
    glUniform1i(0, 0);
    label(GL_PROGRAM, cubeProg, "Cube Program"sv);
    glCheck();

    glViewport(0, 0, cubeSize, cubeSize);
    for (auto n{ 0 }; n < numIblTexs; n++) {
      auto const filename{ iblTextures[n] };
    #if BUILD_DEVELOPMENT
      auto name{ baseName(filename) };
    #endif
      beginGroup(name);
      glGenTextures(1, &envTex[n]);
      glBindTexture(GL_TEXTURE_CUBE_MAP, envTex[n]);
      setupTexture3(GL_TEXTURE_CUBE_MAP, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
      for (auto i{ 0 }; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     cubeSize, cubeSize, 0, GL_RGB, GL_FLOAT, nullptr);
      }
      label(GL_TEXTURE, envTex[n], "%s Env[%d]", name, n);

      auto tex{ importTextureHDR(filename) };
      for (auto i{ 0 }; i < 6; i++) {
        marker("Face %d", i);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                               envTex[n], 0);
        glUniformMatrix4fv(1, 1, false, cubeViewProjs[i].v);
        drawGizmo(cubeGizmo);
      }
      glDeleteTextures(1, &tex);

      glBindTexture(GL_TEXTURE_CUBE_MAP, envTex[n]);
      glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
      endGroup();
    }
    endGroup();
    glCheck();

    beginGroup("Irradiance"sv);
    auto cubeIrradianceProg{ glBuildProgram(cubeVert, cubeIrradianceFrag) };
    glUniform1i(0, 0);
    label(GL_PROGRAM, cubeIrradianceProg, "Irradiance Program"sv);
    glCheck();

    glViewport(0, 0, irradianceSize, irradianceSize);
    for (auto n{ 0 }; n < numIblTexs; n++) {
    #if BUILD_DEVELOPMENT
      auto name{ baseName(iblTextures[n]) };
    #endif
      beginGroup(name);
      glGenTextures(1, &irradianceTex[n]);
      glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceTex[n]);
      setupTexture3(GL_TEXTURE_CUBE_MAP, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
      for (auto i{ 0 }; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     irradianceSize, irradianceSize, 0, GL_RGB, GL_FLOAT, nullptr);
      }
      label(GL_TEXTURE, irradianceTex[n], "%s Irradiance[%d]", name, n);

      glBindTexture(GL_TEXTURE_CUBE_MAP, envTex[n]);
      for (auto i{ 0 }; i < 6; i++) {
        marker("Face %d", i);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                               irradianceTex[n], 0);
        glUniformMatrix4fv(1, 1, false, cubeViewProjs[i].v);
        drawGizmo(cubeGizmo);
      }

      glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceTex[n]);
      glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
      endGroup();
    }
    endGroup();
    glCheck();

    beginGroup("Prefilter"sv);
    auto cubePrefilterProg{ glBuildProgram(cubeVert, cubePrefilterFrag) };
    glUniform1i(0, 0);
    label(GL_PROGRAM, cubePrefilterProg, "Prefilter Program"sv);
    glCheck();

    auto numMips{ 1u };
    auto mipSize{ prefilterSize };
    while (mipSize > 1) {
      numMips++;
      mipSize >>= 1;
    }

    for (auto n{ 0 }; n < numIblTexs; n++) {
    #if BUILD_DEVELOPMENT
      auto name{ baseName(iblTextures[n]) };
    #endif
      beginGroup(name);
      glGenTextures(1, &prefilterTex[n]);
      glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterTex[n]);
      setupTexture3(GL_TEXTURE_CUBE_MAP, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
      auto sz{ prefilterSize };
      for (auto mip{ 0 }; mip < numMips; mip++) {
        for (auto i{ 0 }; i < 6; i++) {
          glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mip, GL_RGB16F, sz, sz, 0,
                       GL_RGB, GL_FLOAT, nullptr);
        }
        sz >>= 1;
      }
      label(GL_TEXTURE, prefilterTex[n], "%s Prefilter[%d]", baseName(iblTextures[n]), n);

      sz = prefilterSize;
      glBindTexture(GL_TEXTURE_CUBE_MAP, envTex[n]);
      for (auto mip{ 0 }; mip < numMips; mip++) {
        beginGroup("Mip %d [%dx%d]", mip, sz, sz);
        glViewport(0, 0, sz, sz);
        glUniform1f(2, static_cast<float>(mip) / (numMips - 1));
        for (auto i{ 0 }; i < 6; i++) {
          marker("Face %d", i);
          glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                 prefilterTex[n], mip);
          glUniformMatrix4fv(1, 1, false, cubeViewProjs[i].v);
          drawGizmo(cubeGizmo);
        }
        endGroup();
        sz >>= 1;
      }
      endGroup();
    }
    endGroup();
    glCheck();

    beginGroup("BRDF Integration"sv);
    auto brdfIntegrationProg{ glBuildProgram(ppVert, brdfIntegrationFrag) };
    label(GL_PROGRAM, brdfIntegrationProg, "BRDF Integration Program"sv);
    glCheck();

    glGenTextures(1, &brdfIntegrationTex);
    glBindTexture(GL_TEXTURE_2D, brdfIntegrationTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, brdfIntegrationSize, brdfIntegrationSize,
                 0, GL_RG, GL_FLOAT, nullptr);
    setupTexture(GL_CLAMP_TO_EDGE, GL_LINEAR, false);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfIntegrationTex, 0);
    label(GL_TEXTURE, brdfIntegrationTex, "BRDF Integration Texture"sv);
    glCheck();

    glViewport(0, 0, brdfIntegrationSize, brdfIntegrationSize);
    glUseProgram(brdfIntegrationProg);
    glBindVertexArray(vaoScreen);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    endGroup();
    glCheck();

    glDeleteShader(cubeVert);
    glDeleteShader(cubeFrag);
    glDeleteShader(cubeIrradianceFrag);
    glDeleteShader(cubePrefilterFrag);
    glDeleteShader(brdfIntegrationFrag);
    glDeleteProgram(cubeProg);
    glDeleteProgram(cubeIrradianceProg);
    glDeleteProgram(cubePrefilterProg);
    glDeleteProgram(brdfIntegrationProg);
    glDeleteFramebuffers(1, &cubeFBO);
    endGroup();
    glCheck();

    auto skyboxVertexSource =
      "layout (location = 0) in vec3 vertexPosition;\n"
      "layout (location = 0) uniform mat4 localToClip;\n"
      "out vec3 localPosition;\n"
      "void main() {\n"
      "  localPosition = vertexPosition;\n"
      "  gl_Position = (localToClip * vec4(vertexPosition, 1.0)).xyww;\n"
      "}\n";

  #if 0
    auto skyboxFragmentSource0 =
      "in vec3 localPosition;\n"
      "layout (location = 0) out vec3 fragColor;\n"
      "layout (location = 2) uniform sampler2D iblTex;\n"
      "layout (location = 3) uniform float exposure;\n";
    auto skyboxFragmentSource1 =
      "void main() {\n"
      "  fragColor = texture2D(iblTex, sphericalUV(localPosition)).rgb;\n"
      "}\n";
  #endif
    auto skyboxFragmentSource0 =
      "in vec3 localPosition;\n"
      "layout (location = 2) uniform samplerCube envTex;\n";
    auto skyboxFragmentSourceForward =
      "void main() {\n"
      "  fragColor = textureCube(envTex, localPosition).rgb;\n"
      "  setupBrightness();\n"
      "}\n";
    auto skyboxFragmentSourceDeferred =
      "void main() {\n"
      "  gBufferNormal = vec3(0);\n"
      "  gBufferAlbedoSpecular = vec4(textureCube(envTex, localPosition).rgb, 0.5);\n"
      "  gBufferMaterialProperties = vec4(0, 0, 0, 0);\n"
      //"  gBufferWorldPos = localPosition;\n"
      "}\n";

    beginGroup("Skybox"sv);
    auto skyboxVert{ glBuildShader(GL_VERTEX_SHADER, skyboxVertexSource) };
    auto skyboxFragForward{ glBuildShader(GL_FRAGMENT_SHADER,
                                          lightingOutput,
                                          skyboxFragmentSource0,
                                          skyboxFragmentSourceForward) };
    auto skyboxFragDeferred{ glBuildShader(GL_FRAGMENT_SHADER,
                                           gBufferOutputs,
                                           skyboxFragmentSource0,
                                           skyboxFragmentSourceDeferred) };

    skyboxProgForward = glBuildProgram(skyboxVert, skyboxFragForward);
    glUniform1i(2, 6);
    label(GL_SHADER, skyboxVert, "Skybox Vertex"sv);
    label(GL_SHADER, skyboxFragForward, "Skybox Fragment (Forward)"sv);
    label(GL_PROGRAM, skyboxProgForward, "Skybox Program (Forward)"sv);

    skyboxProgDeferred = glBuildProgram(skyboxVert, skyboxFragDeferred);
    glUniform1i(2, 6);
    label(GL_SHADER, skyboxVert, "Skybox Vertex"sv);
    label(GL_SHADER, skyboxFragDeferred, "Skybox Fragment (Deferred)"sv);
    label(GL_PROGRAM, skyboxProgDeferred, "Skybox Program (Deferred)"sv);
    endGroup();
    glCheck();


    // ImGui Init
    // ------------------------------------------------------------------------
    auto vertexSource =
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
      "in vec2 frag_uv;\n"
      "in vec4 frag_col;\n"
      "uniform sampler2D tex;\n"
      "layout (location = 0) out vec4 out_col;\n"
      "void main() {\n"
      "  out_col = frag_col * texture2D(tex, frag_uv).x;\n"
      "}\n";

    beginGroup("ImGui"sv);
    auto vert{ glBuildShader(GL_VERTEX_SHADER, vertexSource) };
    auto frag{ glBuildShader(GL_FRAGMENT_SHADER, fragmentSource) };

    prog = glBuildProgram(vert, frag);
    label(GL_SHADER, vert, "ImGui Vertex"sv);
    label(GL_SHADER, frag, "ImGui Fragment"sv);
    label(GL_PROGRAM, prog, "ImGui Program"sv);
    glCheck();

    texLoc = glGetUniformLocation(prog, "tex");
    projLoc = glGetUniformLocation(prog, "proj");
    glUseProgram(prog);
    glUniform1i(texLoc, 0);
    LOG(Test, Info, "ImGUI Program");
    glCheck();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    label(GL_VERTEX_ARRAY, vao, "ImGui VAO"sv);
    glCheck();

    glGenBuffers(2, bufs);
    glBindBuffer(GL_ARRAY_BUFFER, bufs[0]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bufs[1]); // FIXME only for label
    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(ImDrawVert), reinterpret_cast<void*>(IM_OFFSETOF(ImDrawVert, pos)));
    glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(ImDrawVert), reinterpret_cast<void*>(IM_OFFSETOF(ImDrawVert, uv)));
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, true, sizeof(ImDrawVert), reinterpret_cast<void*>(IM_OFFSETOF(ImDrawVert, col)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    label(GL_BUFFER, bufs[0], "ImGui VBO"sv);
    label(GL_BUFFER, bufs[1], "ImGui IBO"sv);
    glBindVertexArray(0);
    glCheck();
    LOG(Test, Info, "ImGUI VAO");

    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    setupTexture(GL_CLAMP_TO_EDGE, GL_NEAREST, false);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    {
      u8* pixels;
      i32 width, height;
      io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
    }
    label(GL_TEXTURE, fontTexture, "ImGui Font Texture"sv);
    endGroup();
    glCheck();
    io.Fonts->TexID = reinterpret_cast<ImTextureID>(static_cast<usize>(fontTexture));
    LOG(Test, Info, "ImGUI Texture");


    // Asset & Components Test
    // ------------------------------------------------------------------------

    beginGroup("Mesh"sv);
    auto filename = "../data/mixamo/test/Victory.fbx";
    auto testMeshData = testMeshImport(filename,
                                       &models[0].materials, &models[0].numMaterials,
                                       &skeleton, &animation);
    testMeshLoad(filename, testMeshData.data(), &models[0]);
    setupModel(&models[0], materialBuffers, &materialIndex);

    progGizmoShadow = getShader(shadowMat, false, false, true, false);
    progGizmoForward = getShader(shadowMat, false, false, false, false);
    progGizmoDeferred = getShader(shadowMat, false, false, false, true);

    glGenBuffers(1, &uboTest);
    glGenBuffers(1, &uboBones);
    glBindBuffer(GL_UNIFORM_BUFFER, uboTest);
    glBindBuffer(GL_UNIFORM_BUFFER, uboBones);
    label(GL_BUFFER, uboTest, "Scene UBO"sv);
    label(GL_BUFFER, uboBones, "Joints UBO"sv);
    endGroup();
    glCheck();

    animState.layers = new AnimState::Layer[animation.numLayers];
    animState.localJoints = new Mat4[skeleton.numBones];
    animState.worldJoints = new Mat4[skeleton.numBones];
    animState.finalJoints = new Mat4[skeleton.numBones];

    memcpy(animState.localJoints, skeleton.boneTransforms, skeleton.numBones * sizeof(Mat4));

    for (auto x{ 0 }; x < spherePaletteSize; x++) {
      for (auto y{ 0 }; y < spherePaletteSize; y++) {
        auto n{ y * spherePaletteSize + x };
        materialBuffers[materialIndex].baseColor = { .8, .6, .4 };
        materialBuffers[materialIndex].emissiveColor = { 0, 0, 0 };
        materialBuffers[materialIndex].metallic = static_cast<f32>(x) / spherePaletteSize;
        materialBuffers[materialIndex].roughness = static_cast<f32>(y) / spherePaletteSize;
        materialBuffers[materialIndex].ao = 1;
        spherePaletteMaterials[n].uboIndex = materialIndex++;
      }
    }

    for (auto n{ 0 }; n < numSpheres; n++) {
      materialBuffers[materialIndex].baseColor = { .2, .3, .4 };
      materialBuffers[materialIndex].emissiveColor = { 0, 0, 0 };
      materialBuffers[materialIndex].metallic = 0;
      materialBuffers[materialIndex].roughness = static_cast<f32>(n) / numSpheres;
      materialBuffers[materialIndex].ao = 1;
      sphereMaterials[n].uboIndex = materialIndex++;
    }

    for (auto n{ 0 }; n < numCubes; n++) {
      materialBuffers[materialIndex].baseColor = { .7, .9, .8 };
      materialBuffers[materialIndex].emissiveColor = { 0, 0, 0 };
      materialBuffers[materialIndex].metallic = static_cast<f32>(n) / numCubes;
      materialBuffers[materialIndex].roughness = 0;
      materialBuffers[materialIndex].ao = 1;
      cubeMaterials[n].uboIndex = materialIndex++;
    }

    auto m{ 1 };
    if constexpr (numModels >= 2) {
      auto filename = "../data/models/Eyebot/Eyebot.obj";
      auto data = testMeshImport(filename, &models[m].materials, &models[m].numMaterials);
      testMeshLoad(filename, data.data(), &models[m]);
      setupModel(&models[m], materialBuffers, &materialIndex, 0.85, 0.25);
      m++;
    }

    if constexpr (numModels >= 3) {
      auto filename = "../data/pbr/Cerberus_by_Andrew_Maximov/Cerberus_LP.fbx";
      auto data = testMeshImport(filename, &models[m].materials, &models[m].numMaterials);
      models[m].materials[0].textures[static_cast<u32>(TextureType::Normal)] =
        importTexture("../data/pbr/Cerberus_by_Andrew_Maximov/Textures/Cerberus_N.tga", TextureType::Normal);
      models[m].materials[0].textures[static_cast<u32>(TextureType::Metallic)] =
        importTexture("../data/pbr/Cerberus_by_Andrew_Maximov/Textures/Cerberus_M.tga", TextureType::Metallic);
      models[m].materials[0].textures[static_cast<u32>(TextureType::Roughness)] =
        importTexture("../data/pbr/Cerberus_by_Andrew_Maximov/Textures/Cerberus_R.tga", TextureType::Roughness);
      testMeshLoad(filename, data.data(), &models[m]);
      setupModel(&models[m], materialBuffers, &materialIndex);
      m++;
    }

    if constexpr (numModels >= 4) {
      auto filename = "../data/models/Clone Trooper Ep3/rep_inf_ep3trooper.obj";
      auto data = testMeshImport(filename, &models[m].materials, &models[m].numMaterials);
      testMeshLoad(filename, data.data(), &models[m]);
      setupModel(&models[m], materialBuffers, &materialIndex, 0.25, 0.85);
      m++;
    }

    if constexpr (numModels >= 5) {
      auto filename = "../data/models/Harley Quinn/Harley Quinn.obj";
      auto data = testMeshImport(filename, &models[m].materials, &models[m].numMaterials);
      testMeshLoad(filename, data.data(), &models[m]);
      setupModel(&models[m], materialBuffers, &materialIndex, 0.25, 0.85);
      m++;
    }

    if constexpr (numModels >= 6) {
      auto filename = "../data/models/Main Outfit/Ellie_MainOutfit.obj";
      auto data = testMeshImport(filename, &models[m].materials, &models[m].numMaterials);
      testMeshLoad(filename, data.data(), &models[m]);
      setupModel(&models[m], materialBuffers, &materialIndex, 0.25, 0.85);
      m++;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, uboMaterial);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(MaterialBuffer) * materialIndex, materialBuffers, GL_STATIC_DRAW);
    endGroup();

    currentWidth = gl->width;
    currentHeight = gl->height;
  }

#if GFX_PRESENT_THREAD
  while (true)
  #endif
  {
    // Frame Setup
    // ------------------------------------------------------------------------

    r += 0.0001f;
    g += 0.00025f;
    b += 0.00005f;
    if (r > 1) r = 0;
    if (g > 1) g = 0;
    if (b > 1) b = 0;

    world.Step(1 / 60.f, velocityIterations, positionIterations);

    auto& io{ ImGui::GetIO() };
    io.DisplaySize = { gl->width, gl->height };
  #if 0
    io.DisplayFramebufferScale = { gl->dpi, gl->dpi };
  #else
    io.DisplayFramebufferScale = { 1, 1 };
  #endif

  #if 0
    io.DeltaTime = gl->getDeltaTime();
  #else
    io.DeltaTime = 0.016;
  #endif

    time += io.DeltaTime;

  #if PLATFORM_HTML5 && 0
    jank_imgui_setCursor(ImGui::GetMouseCursor());
  #endif

    jank_imgui_newFrame();
    ImGui::NewFrame();

    // ImGui Frame
    // ------------------------------------------------------------------------

    //bool show = true;
    //ImGui::ShowDemoWindow(&show);

    ImGui::Begin("Hello");
    ImGui::Text("%.3f ms | %.1f FPS", 1000.f / io.Framerate, io.Framerate);
    ImGui::End();

    ImGui::Begin("Rendering");
    ImGui::Checkbox("Wireframe", &drawWireframe);
    ImGui::Checkbox("Shadowmap", &drawShadowmap);
    ImGui::Checkbox("Deferred", &drawDeferred);
    ImGui::Checkbox("Bloom", &drawBloom);
    ImGui::SameLine();
    ImGui::SliderFloat("Factor", &bloomFactor, 0, 1);
    ImGui::Checkbox("SSAO", &drawSSAO);
    ImGui::SliderFloat("SSAO Radius", &ssaoRadius, 0, 1);
    ImGui::SliderFloat("SSAO Bias", &ssaoBias, 0, 0.1);
    ImGui::Checkbox("Gizmos", &drawGizmos);

    ImGui::Combo("Debug View", reinterpret_cast<i32*>(&debugView),
                 debugViews, _countof(debugViews),
                 std::min(static_cast<i32>(_countof(debugViews)), 10));
    ImGui::End();

    ImGui::Begin("Scene");
    ImGui::SliderInt("Floor", &floorIdx, 0, numFloorMats - 1);
    ImGui::SliderInt("Skybox", &iblIdx, 0, numIblTexs - 1);
    ImGui::End();

    ImGui::Render();

    auto drawData{ ImGui::GetDrawData() };
    // TODO check totalVtxCount

    auto offset{ drawData->DisplayPos };
    auto scale{ drawData->FramebufferScale };
    auto width{ static_cast<GLsizei>(drawData->DisplaySize.x * scale.x) };
    auto height{ static_cast<GLsizei>(drawData->DisplaySize.y * scale.y) };

    // Present sync
    // ------------------------------------------------------------------------

  #if GFX_PRESENT_THREAD && (PLATFORM_ANDROID || PLATFORM_APPLE)
    gl->renderReady.wait();
  # if PLATFORM_ANDROID || PLATFORM_IPHONE
    gl->makeCurrent();
  # endif
  #endif

    if (gl->width != currentWidth || gl->height != currentHeight) {
      currentWidth = gl->width;
      currentHeight = gl->height;
      resize(gl->width, gl->height);
    }

    // Skeletal Animation
    // ------------------------------------------------------------------------

    animState.time += io.DeltaTime * animation.ticksPerSecond;
    auto reset{ animState.time >= animation.duration };

    animState.time = fmod(animState.time, animation.duration);

    for (auto n{ 0 }; n < animation.numLayers; n++) {
      auto const& layer{ animation.layers[n] };
      auto& state{ animState.layers[n] };

      auto nextRotationKey{ (state.rotationKey + 1) % layer.numRotationFrames };
      auto nextPositionKey{ (state.positionKey + 1) % layer.numPositionFrames };
      auto nextScaleKey   { (state.scaleKey    + 1) % layer.numScaleFrames    };

      if (reset || animState.time >= layer.rotationKeys[nextRotationKey]) {
        state.rotationKey = nextRotationKey;
        nextRotationKey = (nextRotationKey + 1) % layer.numRotationFrames;
      }

      if (reset || animState.time >= layer.positionKeys[nextPositionKey]) {
        state.positionKey = nextPositionKey;
        nextPositionKey = (nextPositionKey + 1) % layer.numPositionFrames;
      }

      if (reset || animState.time >= layer.scaleKeys[nextScaleKey]) {
        state.scaleKey = nextScaleKey;
        nextScaleKey = (nextScaleKey + 1) % layer.numScaleFrames;
      }

      auto timeRotation{ layer.rotationKeys[state.rotationKey] };
      auto timePosition{ layer.positionKeys[state.positionKey] };
      auto timeScale   { layer.scaleKeys   [state.scaleKey]    };

      auto deltaRotation{ layer.rotationKeys[nextRotationKey] - timeRotation + 0.000001 };
      auto deltaPosition{ layer.positionKeys[nextPositionKey] - timePosition + 0.000001 };
      auto deltaScale   { layer.scaleKeys   [nextScaleKey]    - timeScale    + 0.000001 };

      auto factorRotation{ (animState.time - timeRotation) / deltaRotation };
      auto factorPosition{ (animState.time - timePosition) / deltaPosition };
      auto factorScale   { (animState.time - timeScale   ) / deltaScale    };

      animState.localJoints[layer.boneIndex].m3x4 =
        rtm::matrix_from_qvv(rtm::quat_lerp(layer.rotations[state.rotationKey],
                                            layer.rotations[nextRotationKey],
                                            factorRotation),
                             rtm::vector_lerp(layer.positions[state.positionKey],
                                              layer.positions[nextPositionKey],
                                              factorPosition),
                             rtm::vector_lerp(layer.scales[state.scaleKey],
                                              layer.scales[nextScaleKey],
                                              factorScale));
    }

    for (auto n = 0; n < skeleton.numBones; n++) {
      auto parentId{ skeleton.boneParentIds[n] };
      if (parentId != Skeleton::invalidBoneId) {
        ASSERT(parentId < n);
        animState.worldJoints[n].m3x4 = rtm::matrix_mul(animState.localJoints[n].m3x4,
                                                        animState.worldJoints[parentId].m3x4);
      }
      else {
        animState.worldJoints[n] = animState.localJoints[n];
      }
    }

    for (auto n = 0; n < skeleton.numBones; n++) {
      animState.finalJoints[n].m4x4 =
        rtm::matrix_mul(skeleton.boneOffsets[n], animState.worldJoints[n].m4x4);
    }

    u32 boneSize = skeleton.numBones * sizeof(Mat4);

    glBindBuffer(GL_UNIFORM_BUFFER, uboBones);
    glBufferData(GL_UNIFORM_BUFFER, boneSize, nullptr, GL_STREAM_DRAW);
    glBufferData(GL_UNIFORM_BUFFER, boneSize, animState.finalJoints, GL_STREAM_DRAW);

    // Camera Setup
    // ------------------------------------------------------------------------
  #if 0
    cameraYaw -= gl->xOffset * io.DeltaTime * 7.5;
    cameraPitch -= gl->yOffset * io.DeltaTime * 7.5;
  #endif

    cameraPitch = std::clamp(cameraPitch, -89.f, 89.f);

    constexpr rtm::anglef roll{ rtm::degrees(0.f) };
    auto cameraQuat{ rtm::quat_from_euler(rtm::degrees(cameraYaw), roll, rtm::degrees(cameraPitch)) };

    constexpr f32 moveSpeed = 15;
  #if 0
    auto cameraMove{ rtm::vector_set(gl->getAxis(InputKeys::A, InputKeys::D) * io.DeltaTime * moveSpeed,
                      gl->getAxis(InputKeys::Shift, InputKeys::Space) * io.DeltaTime * moveSpeed,
                      gl->getAxis(InputKeys::S, InputKeys::W) * io.DeltaTime * moveSpeed) };

    cameraPosition = rtm::vector_add(cameraPosition, rtm::quat_mul_vector3(cameraMove, cameraQuat));
  #endif

    auto proj{ projPerspective(rtm::degrees(60.f).as_radians(),
                               gl->width / gl->height, 1.f, 250.f) };

    Mat4 view;
    if (false) {
      auto eye = rtm::vector_set(std::sinf(r * 100) * 50,
                                 std::sinf(g * 100) * 25,
                                 std::cosf(r * 100) * 50);
      auto at = rtm::vector_set(0, 0.f, 0);
      auto up = rtm::vector_set(0, 1.f, 0);

      auto z = rtm::vector_sub(eye, at);
      z = rtm::vector_normalize3(z, z);
      auto x = rtm::vector_cross3(up, z);
      //x = rtm::vector_normalize3(x, x);
      auto y = rtm::vector_cross3(z, x);
      //y = rtm::vector_cross3(y, y);

      auto ee = eye;// rtm::vector_neg(eye);
      auto xx = -rtm::vector_dot3(x, ee);
      auto yy = -rtm::vector_dot3(y, ee);
      auto zz = -rtm::vector_dot3(z, ee);
      view.m4x4 = rtm::matrix_set(x, y, z,
                                  rtm::vector_set(xx, yy, zz, 1.f)
      );
      view.m4x4 = rtm::matrix_inverse(view.m4x4);
      //view = rtm::matrix_mul(view, rtm::matrix_from_translation(eye));
    }
    view.m3x4 = rtm::matrix_from_qvv(cameraQuat, cameraPosition, rtm::vector_set(1.f, 1.f, 1.f));
    view.m4x4 = rtm::matrix_inverse(view.m4x4);

  #if 0
    gl->xOffset = 0;
    gl->yOffset = 0;
  #endif

    Mat4 viewProj;
    viewProj.m4x4 = rtm::matrix_mul(view.m4x4, proj.m4x4);

    struct Buffers {
      CameraBuffer camera;
      LightBuffer lights[2];
    } buffers;

    static constexpr f32 exposures[]{
      15,
      25,
      25,
      9,
      15,
      5
    };

    targetExposure = exposures[iblIdx];
    if (currentExposure > targetExposure + 0.01f) currentExposure -= 0.01f;
    else if (currentExposure < targetExposure - 0.01f) currentExposure += 0.01f;

    buffers.camera.position = cameraPosition;
    //buffers.camera.exposure = std::sinf(r * 300) * 4 + 8;
    buffers.camera.exposure = currentExposure;

    buffers.lights[0] = {
      rtm::vector_set(std::sinf(r * 100)*15, 5, std::cosf(r * 100)*15, 1.f),
      rtm::vector_set(r * 750, g * 750, b * 750, 1.f)
    };
    auto intensity = std::sinf(r * 250) * 50 + 100;
    buffers.lights[1] = {
      rtm::vector_set(100, 10, -10, 1.f),
      //rtm::vector_set(std::sinf(g * 100) * 10, 0.5f, std::cosf(g * 100) * 10, 1.f),
      rtm::vector_set(intensity, intensity, intensity, 1.f)
    };
  #if 0
    buffers.lights[2] = {
      rtm::vector_set(-1, -1, -1, 1.f),
      rtm::vector_set(.3f, .3f, .1f, 1.f)
    };
    buffers.lights[3] = {
      rtm::vector_set(1, -1, -1, 1.f),
      rtm::vector_set(.3f, .3f, .3f, 1.f)
    };
  #endif

    glBindBuffer(GL_UNIFORM_BUFFER, uboTest);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Buffers), nullptr, GL_STREAM_DRAW);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Buffers), &buffers, GL_STREAM_DRAW);

    glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboTest, cameraOffset, sizeof(buffers.camera));
    glBindBufferRange(GL_UNIFORM_BUFFER, 2, uboTest, lightsOffset, sizeof(buffers.lights));
    glBindBufferRange(GL_UNIFORM_BUFFER, 3, uboBones, 0, boneSize);

    // Model Setup
    // ------------------------------------------------------------------------

    Mat4 objects[numModels];
    rtm::vector4f s1 = rtm::vector_set(0.05f, 0.05f, 0.05f);
    rtm::vector4f s4 = rtm::vector_set(0.03f, 0.03f, 0.03f);
    rtm::vector4f s2 = rtm::vector_set(3.5f, 3.5f, 3.5f);
    rtm::vector4f s3 = rtm::vector_set(200.f, 200.f, 200.f);
    rtm::quatf q = rtm::quat_identity();
    objects[0].m3x4 = rtm::matrix_from_qvv(q, rtm::vector_set(0.f, .05f, 0.f, 1.f), s1);

    if constexpr (numModels >= 2)
      objects[1].m3x4 = rtm::matrix_from_qvv(q, rtm::vector_set(-20, 2, -20, 1.f), s1);

    if constexpr (numModels >= 3)
      objects[2].m3x4 = rtm::matrix_from_qvv(q, rtm::vector_set(-20, 4, 20, 1.f), s1);

    if constexpr (numModels >= 4)
      objects[3].m3x4 = rtm::matrix_from_qvv(q, rtm::vector_set(-10, 0, 20, 1.f), s2);

    if constexpr (numModels >= 5)
      objects[4].m3x4 = rtm::matrix_from_qvv(q, rtm::vector_set(-20, 0, 10, 1.f), s4);

    if constexpr (numModels >= 6)
      objects[5].m3x4 = rtm::matrix_from_qvv(q, rtm::vector_set(-20, 0, -10, 1.f), s3);

    constexpr float PI = 3.1415926535897932384626433832795;

    Mat4 spheres[numSpheres];
    for (auto n{ 0 }; n < numSpheres; n++) {
      auto x{ PI * 2 * n / numSpheres };
      spheres[n].m3x4 = rtm::matrix_from_translation(
        rtm::vector_set(std::sinf(x) * 10, 5, std::cosf(x) * 10));
    }

    Mat4 cubes[numCubes];
    for (auto n{ 0 }; n < numCubes; n++) {
      auto x{ PI * 2 * n / numCubes };
      cubes[n].m3x4 = rtm::matrix_from_translation(
        rtm::vector_set(std::sinf(x) * 30, 1, std::cosf(x) * 30));
    }

    // TODO Passes:
    // - reflection capture
    // - planar reflections
    // - dynamic envmap
    // - z prepass
    // - gbuffer decals
    // - msaa
    // - lens flare
    // - cascaded shadow maps
    // - screen space reflections
    // - motion blur
    // - overlay
    // - fxaa / smaa
    // - HMD distortion

    // Shadow Pass
    // ------------------------------------------------------------------------

    beginGroup("Shadowmap"sv);

    glEnable(GL_DEPTH_TEST);
    glDepthMask(true);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    Mat4 lightViewProj;
    if (drawShadowmap) {
      glBindFramebuffer(GL_FRAMEBUFFER, fboShadow);
      glViewport(0, 0, shadowSize, shadowSize);

      glClearDepth(1);
      glClear(GL_DEPTH_BUFFER_BIT);

    #if 0
      Mat4 shadowBias;
      shadowBias.m4x4 = rtm::matrix_set(
        rtm::vector_set(0.5f, 0, 0, 0),
        rtm::vector_set(0, 0.5f, 0, 0),
        rtm::vector_set(0, 0, 0.5f, 0),
        rtm::vector_set(0.5f, 0.5f, 0.5f, 1)
      );
      lightViewProj.m4x4 = rtm::matrix_mul(shadowBias.m4x4, lightViewProj.m4x4);
    #endif

      Mat4 shadowProj{ projOrtho(-20, 20, -20, 20, 1, 100) };
      Mat4 lightView{ lookAt(buffers.lights[0].position,
                             rtm::vector_set(0, 0, 0, 1.f),
                             rtm::vector_set(0, 1, 0, 1.f)) };

      lightViewProj.m4x4 = rtm::matrix_mul(lightView.m4x4, shadowProj.m4x4);
    
      for (auto n{ 0 }; n < numModels; n++) {
        Mat4 mvp;
        mvp.m4x4 = rtm::matrix_mul(objects[n].m4x4, lightViewProj.m4x4);

        beginGroup(models[n].baseName);
        glBindVertexArray(models[n].vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, models[n].ibo);
        for (auto i{ 0 }; i < models[n].subMeshesCount; i++) {
          auto const& sm{ models[n].subMeshes[i] };
          marker("Mesh %d", i);
          drawSubMeshDepth(sm, models[n].shadowProg, mvp);
        }
        endGroup();
      }
      glCheck();

      {
        glBindVertexArray(vaoGizmo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboGizmo);
        glUseProgram(progGizmoShadow);

        Mat4 mvp;
        beginGroup("Spheres"sv);
        for (auto n{ 0 }; n < numSpheres; n++) {
          mvp.m4x4 = rtm::matrix_mul(spheres[n].m4x4, lightViewProj.m4x4);
          marker("Sphere %d", n);
          glUniformMatrix4fv(0, 1, false, mvp.v);
          drawGizmo(sphereGizmo);
        }
        endGroup();
        beginGroup("Cubes"sv);
        for (auto n{ 0 }; n < numCubes; n++) {
          marker("Cube %d", n);
          mvp.m4x4 = rtm::matrix_mul(cubes[n].m4x4, lightViewProj.m4x4);
          glUniformMatrix4fv(0, 1, false, mvp.v);
          drawGizmo(cubeGizmo);
        }
        endGroup();
        glCheck();
      }
    }
    endGroup();

    // Base Pass
    // ------------------------------------------------------------------------

    beginGroup("Base"sv);
    glBindFramebuffer(GL_FRAMEBUFFER, drawDeferred ? gBufferFBO : fboHDR);
    glViewport(0, 0, width, height);

    glCullFace(GL_BACK);

    if (drawWireframe) {
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
      glClear(GL_DEPTH_BUFFER_BIT);
    }
    glCheck();


    if (!drawDeferred) {
      glActiveTexture(GL_TEXTURE0 + 6);
      glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceTex[iblIdx]);
      glActiveTexture(GL_TEXTURE0 + 8);
      glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterTex[iblIdx]);
      glActiveTexture(GL_TEXTURE0 + 9);
      glBindTexture(GL_TEXTURE_2D, brdfIntegrationTex);
    }

    glActiveTexture(GL_TEXTURE0 + 7);
    glBindTexture(GL_TEXTURE_2D, drawShadowmap ? texShadow : whiteTex);

    // Model
    for (auto n{ 0 }; n < numModels; n++) {
      Mat4 mvp;
      mvp.m4x4 = rtm::matrix_mul(objects[n].m4x4, viewProj.m4x4);

      beginGroup(models[n].baseName);
      glBindVertexArray(models[n].vao);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, models[n].ibo);

      auto progs{ drawDeferred ? models[n].progsDeferred : models[n].progsForward };
      for (auto i{ 0 }; i < models[n].subMeshesCount; i++) {
        auto const& sm{ models[n].subMeshes[i] };
        marker("Mesh %d", i);
        drawSubMesh(sm, models[n].materials[sm.materialIndex], progs[sm.materialIndex],
                    mvp, objects[n], lightViewProj);
      }
      endGroup();
      glCheck();
    }

    {
      glBindVertexArray(vaoGizmo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboGizmo);
      glUseProgram(drawDeferred ? progGizmoDeferred : progGizmoForward);
      glUniformMatrix4fv(9, 1, false, lightViewProj.v);

      Mat4 mvp;
      beginGroup("Spheres"sv);
      for (auto n{ 0 }; n < numSpheres; n++) {
        mvp.m4x4 = rtm::matrix_mul(spheres[n].m4x4, viewProj.m4x4);
        marker("Sphere %d", n);
        glUniformMatrix4fv(0, 1, false, mvp.v);
        glUniformMatrix4fv(1, 1, false, spheres[n].v);
        glBindBufferRange(GL_UNIFORM_BUFFER, 1, uboMaterial, sizeof(MaterialBuffer) * sphereMaterials[n].uboIndex, sizeof(MaterialBuffer));
        drawGizmo(sphereGizmo);
      }
      endGroup();
      beginGroup("Cubes"sv);
      for (auto n{ 0 }; n < numCubes; n++) {
        marker("Cube %d", n);
        mvp.m4x4 = rtm::matrix_mul(cubes[n].m4x4, viewProj.m4x4);
        glUniformMatrix4fv(0, 1, false, mvp.v);
        glUniformMatrix4fv(1, 1, false, cubes[n].v);
        glBindBufferRange(GL_UNIFORM_BUFFER, 1, uboMaterial, sizeof(MaterialBuffer) * cubeMaterials[n].uboIndex, sizeof(MaterialBuffer));
        drawGizmo(cubeGizmo);
      }
      endGroup();
      beginGroup("Sphere Palette"sv);
      for (auto x{ 0 }; x < spherePaletteSize; x++) {
        for (auto y{ 0 }; y < spherePaletteSize; y++) {
          marker("Palette %d.%d", x, y);
          auto n{ y * spherePaletteSize + x };
          Mat4 m;
          m.m3x4 = rtm::matrix_from_translation(rtm::vector_set(x * 3, y * 3 + 5, 20.f));
          mvp.m4x4 = rtm::matrix_mul(m.m4x4, viewProj.m4x4);
          glUniformMatrix4fv(0, 1, false, mvp.v);
          glUniformMatrix4fv(1, 1, false, m.v);
          glBindBufferRange(GL_UNIFORM_BUFFER, 1, uboMaterial, sizeof(MaterialBuffer) * spherePaletteMaterials[n].uboIndex, sizeof(MaterialBuffer));
          drawGizmo(sphereGizmo);
        }
      }
      endGroup();
      glCheck();
    }

    // Floor
    Mat4 floorObject;
    floorObject.m3x4 = rtm::matrix_from_scale(rtm::vector_set(100.f, 100.f, 100.f));

    Mat4 mvp;
    mvp.m4x4 = rtm::matrix_mul(floorObject.m4x4, viewProj.m4x4);

    marker("Floor"sv);
    glBindVertexArray(floorModel.vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floorModel.ibo);
    drawSubMesh(floorModel.subMeshes[0], floorMat[floorIdx],
                (drawDeferred ? floorProgDeferred : floorProgForward)[floorIdx],
                mvp, floorObject, lightViewProj, floorUseDisp);

    // Skybox
    marker("Skybox"sv);
    glActiveTexture(GL_TEXTURE0 + 6);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envTex[iblIdx]);
    glActiveTexture(GL_TEXTURE0);

    auto skyboxViewProj{ view };
    skyboxViewProj.m4x4.w_axis = rtm::vector_set(0, 0, 0, 1.f);
    skyboxViewProj.m4x4 = rtm::matrix_mul(skyboxViewProj.m4x4, proj.m4x4);
    glDisable(GL_CULL_FACE);
    glDepthMask(false);
    glDepthFunc(GL_LEQUAL);
    glUseProgram(drawDeferred ? skyboxProgDeferred : skyboxProgForward);
    glBindVertexArray(vaoGizmo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboGizmo);
    glUniformMatrix4fv(0, 1, false, skyboxViewProj.v);
    drawGizmo(cubeGizmo);
    glCheck();

    if (drawWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    endGroup();

    // Lighting Pass
    // ------------------------------------------------------------------------

    if (drawDeferred) {
      beginGroup("Lighting"sv);
      glDisable(GL_DEPTH_TEST);
      glBindVertexArray(vaoScreen);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, gBufferDepthStencil);
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, gBufferNormals);
      glActiveTexture(GL_TEXTURE2);

      if (drawSSAO) {
        Mat4 projInv;
        projInv.m4x4 = rtm::matrix_inverse(proj.m4x4);

        f32 scale[2]{ gl->width / 4, gl->height / 4 };
        beginGroup("SSAO"sv);
        glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO);
        glUseProgram(ssaoProg);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, ssaoNoise);
        glBindBufferRange(GL_UNIFORM_BUFFER, 4, ssaoSamplesUBO, 0, sizeof(rtm::float4f) * 64);
        glUniformMatrix4fv(3, 1, false, proj.v);
        glUniformMatrix4fv(4, 1, false, projInv.v);
        glUniform2fv(5, 1, scale);
        glUniform1f(6, ssaoRadius);
        glUniform1f(7, ssaoBias);
        glUniformMatrix4fv(8, 1, false, view.v);
        glDrawArrays(GL_TRIANGLES, 0, 3);
      #if 0 // FIXME better blur
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ssaoTex);
        for (auto n{ 0 }; n < 2; n++) {
          auto i{ n & 1 };
          marker("Bloom Blur %d", n);
          glBindFramebuffer(GL_FRAMEBUFFER, fboBlurSSAO[i]);
          glUseProgram(progBlur[i]);
          glDrawArrays(GL_TRIANGLES, 0, 3);
          glBindTexture(GL_TEXTURE_2D, texBlurSSAO[i]);
        }
        glBindTexture(GL_TEXTURE_2D, gBufferNormals);
        glActiveTexture(GL_TEXTURE2);
      #endif
        endGroup();
      }

      if (debugView == DebugView::None || debugView == DebugView::Bloom) {
        Mat4 viewProjInv;
        viewProjInv.m4x4 = rtm::matrix_inverse(viewProj.m4x4);

        glBindFramebuffer(GL_FRAMEBUFFER, fboHDR);
        glUseProgram(lightingProg);
        glUniformMatrix4fv(4, 1, false, viewProjInv.v);
        glBindTexture(GL_TEXTURE_2D, gBufferAlbedoSpecular);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, gBufferMaterialProperties);
        glActiveTexture(GL_TEXTURE0 + 6);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceTex[iblIdx]);
        glActiveTexture(GL_TEXTURE0 + 8);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterTex[iblIdx]);
        glActiveTexture(GL_TEXTURE0 + 9);
        glBindTexture(GL_TEXTURE_2D, brdfIntegrationTex);
        glActiveTexture(GL_TEXTURE0 + 10);
        glBindTexture(GL_TEXTURE_2D, drawSSAO ? ssaoTex : whiteTex);
        glDrawArrays(GL_TRIANGLES, 0, 3);
      }
      endGroup();
    }

    // Bloom (Blur)
    // ------------------------------------------------------------------------

    beginGroup("BloomBlur"sv);
    if (drawBloom) {
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, texBright);

      for (auto n{ 0 }; n < 10; n++) {
        auto i{ n & 1 };
        marker("Bloom Blur %d", n);
        glBindFramebuffer(GL_FRAMEBUFFER, fboBlur[i]);
        glUseProgram(progBlur[i]);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindTexture(GL_TEXTURE_2D, texBlur[i]);
      }
    }
    else {
      glActiveTexture(GL_TEXTURE1);
      glBindTexture(GL_TEXTURE_2D, blackTex);
    }
    endGroup();

    // Post Process
    // ------------------------------------------------------------------------

    if (debugView == DebugView::None) {
      beginGroup("PostProcess"sv);
      glBindFramebuffer(GL_FRAMEBUFFER, fboLDR);
      glUseProgram(progPostHDR);
      glUniform1f(2, bloomFactor);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texHDR);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      glCheck();

      glBindFramebuffer(GL_FRAMEBUFFER, fboScratch);
      glUseProgram(progPostLDR);
      glBindTexture(GL_TEXTURE_2D, texLDR);
      glUniform1f(1, time);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      glCheck();
      endGroup();
    }

    // Gizmos
    // ------------------------------------------------------------------------

    if (drawGizmos && debugView == DebugView::None) {
      beginGroup("Gizmos"sv);
      glEnable(GL_DEPTH_TEST);
      glDepthMask(true);
      glDepthFunc(GL_LESS);
      glClear(GL_DEPTH_BUFFER_BIT);
      glCheck();

    #if 1
      //auto pos = body->GetPosition();
      //auto model = rtm::matrix_from_translation(rtm::vector_set(pos.x, pos.y, 0.f, 1.f));
      glUseProgram(triangleProg);
      glBindVertexArray(vaoTriangle);
      for (auto n{ 0 }; n < 2; n++) {
        Mat4 localToWorld;
        localToWorld.m3x4 = rtm::matrix_from_qvv(rtm::quat_identity(),
                                                 buffers.lights[n].position,
                                                 rtm::vector_set(5.f, 5.f, 5.f));
        Mat4 mvp;
        mvp.m4x4 = rtm::matrix_mul(localToWorld.m4x4, viewProj.m4x4);
        marker("Light Gizmo %d", n);
        glUniformMatrix4fv(mvpPos, 1, false, mvp.v);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glCheck();
      }
    #endif

    #if BUILD_EDITOR
      f32 color[3]{ .9f, 0.f, 0.f };
      glEnable(GL_CULL_FACE);
      glUseProgram(progGizmo);
      glBindVertexArray(vaoGizmo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboGizmo);
      glUniform3fv(2, 1, color);
      glUniformMatrix4fv(0, 1, false, proj.v);
      drawBones(objects[0], view, animState.worldJoints);
      glCheck();
    #endif
      endGroup();
    }

    // Debug View
    // ------------------------------------------------------------------------

    if (debugView != DebugView::None) {
      struct DebugViewParams {
        u32 prog;
        u32 tex;
      } params;

      switch (debugView) {
      case DebugView::None: ASSERT(0); UNREACHABLE;
      case DebugView::Depth: params = { debugViewProgR, gBufferDepthStencil }; break;
      case DebugView::Stencil: params = { debugViewProgA, gBufferDepthStencil }; break;
      case DebugView::Normals: params = { debugViewProgRGB, gBufferNormals }; break;
      case DebugView::Albedo: params = { debugViewProgRGB, gBufferAlbedoSpecular }; break;
      case DebugView::Metallic: params = { debugViewProgR, gBufferMaterialProperties }; break;
      case DebugView::Roughness: params = { debugViewProgG, gBufferMaterialProperties }; break;
      case DebugView::AO: params = { debugViewProgB, gBufferMaterialProperties }; break;
      case DebugView::SSAO: params = { debugViewProgR, ssaoTex }; break;
      case DebugView::Bloom: params = { debugViewProgR, texBright }; break;
      }

      beginGroup("DebugView"sv);
      glDisable(GL_CULL_FACE);
      glDisable(GL_DEPTH_TEST);
      glBindFramebuffer(GL_FRAMEBUFFER, fboScratch);
      glBindVertexArray(vaoScreen);
      glUseProgram(params.prog);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, params.tex);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      endGroup();
    }

    // ImGui
    // ------------------------------------------------------------------------

    beginGroup("ImGui"sv);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(false);
    {
      auto l{ drawData->DisplayPos.x };
      auto r{ drawData->DisplaySize.x + l };
      auto t{ drawData->DisplayPos.y };
      auto b{ drawData->DisplaySize.y + t };
      auto proj{ projOrtho(b, t, l, r) };
      glUseProgram(prog);
      glUniformMatrix4fv(projLoc, 1, false, proj.v);
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

#if 0
    vtxOffset = 0;
    idxOffset = 0;
#else
    int vtxOffset = 0, idxOffset = 0;
#endif
#if 1
    for (auto n{0}; n < drawData->CmdListsCount; n++) {
      auto cmd{ drawData->CmdLists[n] };
      glBufferData(GL_ARRAY_BUFFER, cmd->VtxBuffer.Size * sizeof(ImDrawVert), cmd->VtxBuffer.Data, GL_STREAM_DRAW);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, cmd->IdxBuffer.Size * sizeof(ImDrawIdx), cmd->IdxBuffer.Data, GL_STREAM_DRAW);
      for (auto i{0}; i < cmd->CmdBuffer.Size; i++) {
        auto draw{ &cmd->CmdBuffer[i] };
        ASSERT(!draw->UserCallback, "TODO");

        auto clipX{ (draw->ClipRect.x - offset.x) * scale.x };
        auto clipY{ (draw->ClipRect.y - offset.y) * scale.y };
        auto clipZ{ (draw->ClipRect.z - offset.x) * scale.x };
        auto clipW{ (draw->ClipRect.w - offset.y) * scale.y };

        if (clipX < width && clipY < height && clipZ >= 0 && clipW >= 0) {
          glScissor(clipX, height - clipW, clipZ - clipX, clipW - clipY);
          glBindTexture(GL_TEXTURE_2D, static_cast<u32>(reinterpret_cast<usize>(draw->TextureId)));
          glDrawElements/*BaseVertex*/(GL_TRIANGLES, draw->ElemCount,
                                       sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                                       reinterpret_cast<void*>(draw->IdxOffset * sizeof(ImDrawIdx) + idxOffset)/*,
                                                                                                                draw->VtxOffset + vtxOffset*/);
          glCheck();
          //LOG(GfxTest, Info, "DRAW %d.%d", n, i);
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
    endGroup();

    // Final
    // ------------------------------------------------------------------------

#if GFX_PRESENT_THREAD && !PLATFORM_ANDROID && !PLATFORM_APPLE
    gl->renderReady.wait();
#endif

  #if 1
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fboScratch);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboSurface);
    glBlitFramebuffer(0, 0, gl->width, gl->height,
                      0, 0, gl->width, gl->height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
  #endif

#if GFX_PRESENT_THREAD
# if PLATFORM_ANDROID
    gl->clearCurrent();
# endif
    //glFlush();
    gl->presentReady.set();
#else
    gl->present();
    gl->clearCurrent();
#endif
  }

  //glDeleteTextures(1, &fontTexture);
  //io.Fonts->TexID = 0;
}
