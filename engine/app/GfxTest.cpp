#include "app/App.hpp"
#include "app/GfxTest.hpp"

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

using namespace std::literals;

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
void jank_imgui_setCursor(ImGuiMouseCursor);

char const* jank_imgui_getClipboardText(void*);
void jank_imgui_setClipboardText(void*, char const*);

#if !PLATFORM_HTML5
void jank_imgui_init() {}
void jank_imgui_newFrame() {}
void jank_imgui_setCursor(ImGuiMouseCursor) {}

char const* jank_imgui_getClipboardText(void*) { return ""; }
void jank_imgui_setClipboardText(void*, char const*) {}
#endif

#if BUILD_EDITOR
std::string testMeshImport(Material** materials, u32* numMaterials,
                           Skeleton* skeleton, Animation* animation);
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
  GL(ENABLEVERTEXATTRIBARRAY, EnableVertexAttribArray); \
  GL(GETPROGRAMINFOLOG, GetProgramInfoLog); \
  GL(GETPROGRAMIV, GetProgramiv); \
  GL(GETSHADERINFOLOG, GetShaderInfoLog); \
  GL(GETSHADERIV, GetShaderiv); \
  GL(GETUNIFORMLOCATION, GetUniformLocation); \
  GL(LINKPROGRAM, LinkProgram); \
  GL(SHADERSOURCE, ShaderSource); \
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
# undef GL
#endif


// OpenGL Helpers
// -----------------------------------------------------------------------------

#if BUILD_DEVELOPMENT
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
    default: ASSERT(0, "Unknown GL Error: %04x", e); UNREACHABLE;
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
      LOG(App, Error, "Failed to " action ":\n%.*s", size, s.data()); \
      ASSERT(0); \
    } \
  }
GL_CHECK_INFO_LOG(glCheckShader,  glGetShaderiv,  glGetShaderInfoLog,  GL_COMPILE_STATUS, "compile shader")
GL_CHECK_INFO_LOG(glCheckProgram, glGetProgramiv, glGetProgramInfoLog, GL_LINK_STATUS,    "link program")
# undef GL_CHECK_INFO_LOG
#else
# define glCheck()
# define glCheckFramebuffer()
# define glCheckShader(shader)
# define glCheckProgram(program)
#endif

static bool isGLES;
static std::string glslHeader;

class ShaderBuilder {
  char const* sources[2];
  u32 numSources;

public:
  ShaderBuilder() : numSources(0) {
    push(glslHeader.c_str());
  }

  void push(char const* source) {
    ASSERT(numSources < 2);
    sources[numSources++] = source;
  }

  u32 compile(u32 stage) {
#if BUILD_DEBUG
    for (auto n{0}; n < numSources; n++) {
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

u32 glBuildShader(u32 stage, char const* source) {
  ShaderBuilder b;
  b.push(source);
  return b.compile(stage);
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
  TEST(ARB_texture_filter_anisotropic, {

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
    LOG(App, Warn, "Unknown GL Extension: %.*s", static_cast<u32>(ext.size()), ext.data());
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

#if BUILD_EDITOR
u32 loadImage(CMP_Texture const& tex, bool isNormal) {
  u32 id;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
if (isNormal)
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, tex.dwWidth, tex.dwHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, tex.pData);
else
  glCompressedTexImage2D(GL_TEXTURE_2D, 0,
                         isNormal ? GL_COMPRESSED_RG_RGTC2 : GL_COMPRESSED_RGBA_S3TC_DXT5_EXT,
                         tex.dwWidth, tex.dwHeight, 0, tex.dwDataSize, tex.pData);

  glCheck();
  return id;
}
u32 loadImageHDR(void const* data, u32 w, u32 h) {
  u32 id;
  glGenTextures(1, &id);
  glBindTexture(GL_TEXTURE_2D, id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, data);
  glCheck();
  return id;
}
#endif

static bool hasInit;
static u32 fbo, tex, rboDepth;
static u32 vaoTriangle;
static u32 vboTriangle;
static u32 triangleProg, prog, texLoc, projLoc;
static u32 vao;
static u32 bufs[2];
static GLuint fontTexture;
static float r{0}, g{0}, b{0};

struct alignas(256) CameraBuffer {
  rtm::vector4f position;
  float exposure;
};

struct alignas(256) MaterialBuffer {
  float metallic;
  float roughness;
  float ao;
  float useNormalMap;
  rtm::float3f refractiveIndex;
};

struct LightBuffer {
  rtm::vector4f position;
  rtm::vector4f color;
};

constexpr u32 cameraOffset   = 0;
constexpr u32 materialOffset = cameraOffset + 256;
constexpr u32 lightsOffset   = materialOffset + 256;

static u32 iblTex0, iblTex1, iblTex2;
static u32 skyboxProg;

static u32 uboTest;
static u32 uboBones;
static u32 vaoTest;
static u32 vboTest;
static u32 iboTest;
static u32 testProg;
static u32 subMeshesCountTest;
static MeshHeader::SubMesh* subMeshesTest;
static u32 numMaterials;
static Material* materials;
static Skeleton skeleton;
static Animation animation;

union Mat4 {
  rtm::matrix4x4f m4x4;
  rtm::matrix3x4f m3x4;
  float a[4][4];
  float v[16];
};

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

constexpr i32 velocityIterations = 6;
constexpr i32 positionIterations = 2;

static b2World world{{0, -10}};
static b2Body* groundBody;
static b2Body* body;
static u32 mvpPos;

#if BUILD_EDITOR
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

    glUniformMatrix4fv(1, 1, false, modelToView.v);
    drawGizmo(sphereGizmo);
  }
}

#if 0
static u32 vboStream;
static u32 iboStream;
static void* vboStreamMap;
static void* iboStreamMap;
#endif

void* renderMain(void* arg) {
  auto gl{ reinterpret_cast<OpenGL*>(arg) };

#if PLATFORM_IPHONE
  auto fboSurface{ gl->getSurface() };
#else
  constexpr u32 fboSurface = 0;
#endif

  if (!hasInit) {
    hasInit = true;

  #if PLATFORM_APPLE
    pthread_setname_np("Render");
  #elif PLATFORM_POSIX
    pthread_setname_np(pthread_self(), "Render");
  #endif

    gl->makeCurrent();
  #if PLATFORM_IPHONE
    glCheckFramebuffer();
  #endif

    LOG(App, Info, "OpenGL Version %s", glGetString(GL_VERSION));
    LOG(App, Info, "OpenGL Vendor: %s", glGetString(GL_VENDOR));
    LOG(App, Info, "OpenGL Renderer: %s", glGetString(GL_RENDERER));

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

    {
      auto str{ reinterpret_cast<char const*>(glGetString(GL_SHADING_LANGUAGE_VERSION)) };
      LOG(App, Info, "GLSL Version: %s", str);
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
    }

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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    jank_imgui_init();

    auto& io{ ImGui::GetIO() };
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.SetClipboardTextFn = jank_imgui_setClipboardText;
    io.GetClipboardTextFn = jank_imgui_getClipboardText;

    // FBO
    // ------------------------------------------------------------------------
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, gl->width, gl->height);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gl->width, gl->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    glCheckFramebuffer();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboSurface);
    glCheck();
    LOG(Test, Info, "Framebuffer");


    // Triangle init
    // ------------------------------------------------------------------------
    float vertices[]{
      -.5f, -.5f, 0,
      .5f, -.5f, 0,
      0, .5f, 0
    };

    glGenVertexArrays(1, &vaoTriangle);
    glBindVertexArray(vaoTriangle);

    glGenBuffers(1, &vboTriangle);
    glBindBuffer(GL_ARRAY_BUFFER, vboTriangle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, false, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glCheck();
    LOG(Test, Info, "Triangle VAO");

    auto triangleVertexSource =
      "layout (location = 0) in vec3 pos;\n"
      //"uniform vec2 worldPos;\n"
      "uniform mat4 mvp;\n"
      "out vec3 frag_col;\n"
      "void main() {\n"
      "  gl_Position = mvp * vec4(pos, 1);\n"
      "  frag_col = normalize(pos);\n"
      "}\n";

    auto triangleFragmentSource =
      "layout (location = 0) out vec4 col;\n"
      "in vec3 frag_col;\n"
      "void main() {\n"
      "  col = vec4(frag_col, 1);\n"
      "}\n";

    auto triangleVert{ glBuildShader(GL_VERTEX_SHADER, triangleVertexSource) };
    auto triangleFrag{ glBuildShader(GL_FRAGMENT_SHADER, triangleFragmentSource) };

    triangleProg = glCreateProgram();
    glAttachShader(triangleProg, triangleVert);
    glAttachShader(triangleProg, triangleFrag);
    glLinkProgram(triangleProg);
    glCheckProgram(triangleProg);

    mvpPos = glGetUniformLocation(triangleProg, "mvp");
    LOG(Test, Info, "Triangle Program");

    // IBL
    // ------------------------------------------------------------------------

  #define SPHERICAL_UV_SOURCE \
      "vec2 sphericalUV(vec3 v) {\n" \
      "  vec2 uv = vec2(atan(v.z, v.x), -asin(v.y));\n" \
      "  uv *= vec2(0.1591, 0.3183);\n" \
      "  uv += 0.5;\n" \
      "  return uv;\n" \
      "}"

  #define TONEMAP_UNCHARTED_2 \
      "vec3 tonemapUncharted2(vec3 x) {\n" \
      "  const float A = 0.15;\n" \
      "  const float B = 0.50;\n" \
      "  const float C = 0.10;\n" \
      "  const float D = 0.20;\n" \
      "  const float E = 0.02;\n" \
      "  const float F = 0.30;\n" \
      "  return ((x * (x * A + B * C) + D * E) / (x * (x * A + B) + D * F)) - E / F;" \
      "}\n" \
      "vec4 tonemapAndGammaCorrect(vec3 color, float exposure) {\n" \
      "  color = tonemapUncharted2(color * exposure) * (1.0 / tonemapUncharted2(vec3(11.2)));\n" \
      "  color = pow(color, vec3(1.0/2.2));\n" \
      "  return vec4(color, 1);\n" \
      "}\n"

    iblTex0 = importTextureHDR("../data/hdri/entrance_hall_2k.hdr");
    iblTex1 = importTextureHDR("../data/hdri/satara_night_2k.hdr");
    iblTex2 = importTextureHDR("../data/hdri/rooitou_park_2k.hdr");

    auto skyboxVertexSource =
      "layout (location = 0) in vec3 vertexPosition;\n"
      "layout (location = 0) uniform mat4 viewToClip;\n"
      "layout (location = 1) uniform mat4 localToView;\n"
      "out vec3 localPosition;\n"
      "void main() {\n"
      "  localPosition = vertexPosition;\n"
      "  gl_Position = (localToView * viewToClip) * vec4(vertexPosition, 1.0);\n"
      "}\n";

    auto skyboxFragmentSource =
      "in vec3 localPosition;\n"
      "layout (location = 0) out vec4 fragColor;\n"
      "layout (location = 2) uniform sampler2D iblTex;\n"
      TONEMAP_UNCHARTED_2
      SPHERICAL_UV_SOURCE
      "void main() {\n"
      "  vec3 color = texture2D(iblTex, sphericalUV(localPosition)).rgb;\n"
      "  fragColor = tonemapAndGammaCorrect(color, 1.0);\n"
      "}\n";

    auto skyboxVert{ glBuildShader(GL_VERTEX_SHADER, skyboxVertexSource) };
    auto skyboxFrag{ glBuildShader(GL_FRAGMENT_SHADER, skyboxFragmentSource) };

    skyboxProg = glCreateProgram();
    glAttachShader(skyboxProg, skyboxVert);
    glAttachShader(skyboxProg, skyboxFrag);
    glLinkProgram(skyboxProg);
    glCheckProgram(skyboxProg);
    glUseProgram(skyboxProg);
    glUniform1i(2, 2);
    glCheck();


    // Test Model
    // ------------------------------------------------------------------------

    auto testVertexSource =
      "layout (location = 0) in vec3 vertexPosition;\n"
      //"layout (location = 1) in vec3 vertexNormal;\n"
      "layout (location = 1) in vec3 vertexTangent0;\n"
      "layout (location = 2) in vec3 vertexTangent1;\n"
      "layout (location = 3) in vec2 vertexUV;\n"
      "layout (location = 4) in vec4 vertexBoneWeights;\n"
      "layout (location = 5) in ivec4 vertexBoneIndices;\n"
      "layout (location = 0) uniform mat4 localToClip;\n"
      "layout (location = 1) uniform mat4 localToWorld;\n"
      "layout (binding = 3) uniform Skeleton {\n"
      "  mat4 bones[256];\n"
      "};\n"
      "out vec3 worldPosition;\n"
      //"out vec3 worldNormal;\n"
      "out vec3 worldTangent0;\n"
      "out vec3 worldTangent1;\n"
      "out vec2 uv;\n"
      "void main() {\n"
      "  mat4 boneTransform =\n"
      "    bones[vertexBoneIndices[0]] * vertexBoneWeights[0] +\n"
      "    bones[vertexBoneIndices[1]] * vertexBoneWeights[1] +\n"
      "    bones[vertexBoneIndices[2]] * vertexBoneWeights[2] +\n"
      "    bones[vertexBoneIndices[3]] * vertexBoneWeights[3];\n"
      "  vec4 localPosition = boneTransform * vec4(vertexPosition, 1.0);\n"
      "  gl_Position = localToClip * localPosition;\n"
      "  worldPosition = (localToWorld * localPosition).xyz;\n"
      //"  worldNormal = mat3(localToWorld) * vertexNormal;\n"
      "  mat3 boneToWorld = mat3(boneTransform) * mat3(localToWorld);\n"
      "  worldTangent0 = boneToWorld * vertexTangent0;\n"
      "  worldTangent1 = boneToWorld * vertexTangent1;\n"
      "  uv = vertexUV;\n"
      "}\n";

    auto testFragmentSource =
      "const float PI = 3.1415926535897932384626433832795;\n"
      "layout (location = 0) out vec4 fragColor;\n"
      "in vec3 worldPosition;\n"
      //"in vec3 worldNormal;\n"
      "in vec3 worldTangent0;\n"
      "in vec3 worldTangent1;\n"
      "in vec2 uv;\n"
      "layout (location = 2) uniform sampler2D baseTex;\n"
      "layout (location = 3) uniform sampler2D normalTex;\n"
      "layout (location = 4) uniform sampler2D iblTex;\n"
      "layout (binding = 0, std140) uniform Camera {\n"
      "  vec3 position;\n"
      "  float padding0;\n"
      "  float exposure;\n"
      "} camera;\n"
      "layout (binding = 1, std140) uniform Material {\n"
      "  float metallic;\n"
      "  float roughness;\n"
      "  float ao;\n"
      "  float useNormalMap;\n"
      "  vec3  refractiveIndex;\n"
      "} material;\n"
      "struct LightData {\n"
      "  vec3 position;\n"
      "  float padding0;\n"
      "  vec3 color;\n"
      "  float padding1;\n"
      "};\n"
      "layout (binding = 2, std140) uniform Light {\n"
      " LightData lights[2];\n"
      "};\n"
      "vec3 fresnelSchlick(float cosTheta, vec3 f0) {\n"
      "  return f0 + (1.0 - f0) * pow(1.0 - cosTheta, 5.0);\n"
      "}\n"
      "float distributionGGX(vec3 n, vec3 h, float roughness) {\n"
      "  float ndoth = max(dot(n, h), 0.0);\n"
      "  float a     = roughness * roughness;\n"
      "  float a2    = a * a;\n"
      "  float d     = ndoth * ndoth * (a2 - 1.0) + 1.0;\n"
      "  return a2 / max(PI * d * d, 0.001);\n"
      "}\n"
      "float geometrySchlickGGX(float ndotv, float roughness) {\n"
      "  float r = roughness + 1.0;\n"
      "  float k = (r * r) / 8.0;\n"
      "  return ndotv / (ndotv * (1.0 - k) + k);\n"
      "}\n"
      "float geometrySmith(float ndotl, float ndotv, float roughness) {\n"
      "  return geometrySchlickGGX(ndotl, roughness) * geometrySchlickGGX(ndotv, roughness);\n"
      "}\n"
      TONEMAP_UNCHARTED_2
      SPHERICAL_UV_SOURCE
      "void main() {\n"
      "  vec3 t0 = normalize(worldTangent0);\n"
      "  vec3 t1 = normalize(worldTangent1);\n"
      "  vec3 t2 = cross(t0, t1);\n"
      "  mat3 tbn = mat3(t1, t2, t0);\n"
      //"  vec3 nm = vec3(texture2D(normalTex, uv).rg * 2.0 - 1.0, 0);\n"
      //"  nm.z = sqrt(1.0 - dot(nm.xy, nm.xy));\n"
      "  vec3 nm = texture2D(normalTex, uv).rgb;\n"
      "  nm.xy = nm.xy * 2.0 - 1.0;\n"
      //"  vec3 worldNormal = material.useNormalMap > 0.5 ? tbn * nm : t0;\n"
      "  vec3 worldNormal = tbn * nm;\n"
      "  vec3 albedo = texture2D(baseTex, uv).rgb;\n"
      "  vec3 n = normalize(worldNormal);\n"
      "  vec3 v = normalize(camera.position - worldPosition);\n"
      "  float ndotv = max(dot(n, v), 0.0);\n"
      "  vec3 f0 = mix(material.refractiveIndex, albedo, material.metallic);\n"
      "  vec3 lo = vec3(0.0);\n"
      "  float oneMinusMetallic = 1.0 - material.metallic;\n"
      "  for (int i = 0; i < 2; i++) {\n"
      "    vec3 light = lights[i].position - worldPosition;\n"
      "    vec3 l = normalize(light);\n" // radiance
      "    vec3 h = normalize(v + l);\n"
      "    float distance = length(light);\n"
      "    float attenuation = 1.0 / (distance * distance);\n"
      "    vec3 radiance = lights[i].color * attenuation;\n"
      "    float ndf = distributionGGX(n, h, material.roughness);\n" // cook-torrance
      "    float ndotl = max(dot(n, l), 0.0);\n"
      "    float g = geometrySmith(ndotl, ndotv, material.roughness);\n"
      "    vec3 f = fresnelSchlick(max(dot(h, v), 0.0), f0);\n"
      "    float d = 4 * ndotl * ndotv;\n"
      "    vec3 specular = (ndf * g * f) / max(d, 0.001);\n"
      "    vec3 kd = (vec3(1.0) - f) * oneMinusMetallic;\n"
      "    lo += (kd * albedo / PI + specular) * radiance * ndotl;\n"
      "  }\n"
      "  vec3 kd = (vec3(1.0) - fresnelSchlick(ndotv, f0)) * oneMinusMetallic;\n"
      "  vec3 irradiance = texture(iblTex, sphericalUV(n)).rgb * 0.05;\n"
      "  vec3 diffuse = irradiance * albedo;\n"
      "  vec3 ambient = kd * diffuse * material.ao;\n"
      "  vec3 color = ambient + lo;\n"
      "  fragColor = tonemapAndGammaCorrect(color * 16, camera.exposure);\n"
      "}\n";

    auto testVert{ glBuildShader(GL_VERTEX_SHADER, testVertexSource) };
    auto testFrag{ glBuildShader(GL_FRAGMENT_SHADER, testFragmentSource) };

    testProg = glCreateProgram();
    glAttachShader(testProg, testVert);
    glAttachShader(testProg, testFrag);
    glLinkProgram(testProg);
    glCheckProgram(testProg);
    glUseProgram(testProg);
    glUniform1i(2, 0);
    glUniform1i(3, 1);
    glUniform1i(4, 2);
    glCheck();

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
      "layout (location = 0) out vec4 fragColor;\n"
      "in vec3 vertexColor;\n"
      "void main() {\n"
      "  fragColor = vec4(vertexColor, 1);\n"
      "}\n";

    auto gizmoVert{ glBuildShader(GL_VERTEX_SHADER, gizmoVertexSource) };
    auto gizmoFrag{ glBuildShader(GL_FRAGMENT_SHADER, gizmoFragmentSource) };

    progGizmo = glCreateProgram();
    glAttachShader(progGizmo, gizmoVert);
    glAttachShader(progGizmo, gizmoFrag);
    glLinkProgram(progGizmo);
    glCheckProgram(progGizmo);
    glCheck();

    {
      auto boneData = testMeshImportSimple("../data/editor/gizmo/bone.stl");
      auto sphereData = testMeshImportSimple("../data/editor/gizmo/sphere.stl");
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

      boneGizmo.indexCount = boneMesh->indexCount;
      boneGizmo.indexOffset = 0;
      boneGizmo.vertexStart = 0;
      sphereGizmo.indexCount = sphereMesh->indexCount;
      sphereGizmo.indexOffset = boneIndexSize;
      sphereGizmo.vertexStart = boneHeader->numVertices;
      cubeGizmo.indexCount = cubeMesh->indexCount;
      cubeGizmo.indexOffset = boneIndexSize + sphereIndexSize;
      cubeGizmo.vertexStart = boneHeader->numVertices + sphereHeader->numVertices;
    }
  #endif


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

    auto vert{ glBuildShader(GL_VERTEX_SHADER, vertexSource) };
    auto frag{ glBuildShader(GL_FRAGMENT_SHADER, fragmentSource) };

    prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glCheckProgram(prog);

    texLoc = glGetUniformLocation(prog, "tex");
    projLoc = glGetUniformLocation(prog, "proj");
    glUseProgram(prog);
    glUniform1i(texLoc, 0);
    LOG(Test, Info, "ImGUI Program");
    glCheck();

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(2, bufs);
    glBindBuffer(GL_ARRAY_BUFFER, bufs[0]);
    glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(ImDrawVert), reinterpret_cast<void*>(IM_OFFSETOF(ImDrawVert, pos)));
    glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(ImDrawVert), reinterpret_cast<void*>(IM_OFFSETOF(ImDrawVert, uv)));
    glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, true, sizeof(ImDrawVert), reinterpret_cast<void*>(IM_OFFSETOF(ImDrawVert, col)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    glCheck();
    LOG(Test, Info, "ImGUI VAO");

    // ImGui font texture
    // ------------------------------------------------------------------------
    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    {
      u8* pixels;
      i32 width, height;
      io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, pixels);
    }
    glCheck();
    io.Fonts->TexID = reinterpret_cast<ImTextureID>(static_cast<usize>(fontTexture));
    LOG(Test, Info, "ImGUI Texture");


    // Asset & Components Test
    // ------------------------------------------------------------------------
  #if BUILD_EDITOR
    auto testMeshData = testMeshImport(&materials, &numMaterials, &skeleton, &animation);
    auto meshHeader = std::launder(reinterpret_cast<MeshHeader*>(testMeshData.data()));

    glGenVertexArrays(1, &vaoTest);
    glBindVertexArray(vaoTest);
    glGenBuffers(1, &vboTest);
    glGenBuffers(1, &iboTest);
    glBindBuffer(GL_ARRAY_BUFFER, vboTest);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboTest);

    subMeshesTest = new MeshHeader::SubMesh[meshHeader->subMeshCount];
    subMeshesCountTest = meshHeader->subMeshCount;
    auto subMeshesSize = sizeof(MeshHeader::SubMesh) * meshHeader->subMeshCount;
    memcpy(subMeshesTest, testMeshData.data() + sizeof(MeshHeader), subMeshesSize);

    auto indicesOffset = sizeof(MeshHeader) + subMeshesSize;
    auto indicesSize = meshHeader->numIndices * 4;
    auto verticesOffset = indicesOffset + indicesSize;
    auto verticesSize = 0;

    if (meshHeader->flags & MeshHeader::HasPositions) {
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, false, 0, nullptr);
      verticesSize += meshHeader->numVertices * 12;
    }
    if (meshHeader->flags & MeshHeader::HasTangents) {
      glEnableVertexAttribArray(1);
      glEnableVertexAttribArray(2);
    #if 0
      glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, reinterpret_cast<void*>(verticesSize));
      verticesSize += meshHeader->numVertices * 12;
    #endif
      glVertexAttribPointer(2, 3, GL_FLOAT, false, 0, reinterpret_cast<void*>(verticesSize));
      verticesSize += meshHeader->numVertices * 12;
    }
    /*else*/ if (meshHeader->flags & MeshHeader::HasNormals) {
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 3, GL_FLOAT, false, 0, reinterpret_cast<void*>(verticesSize));
      verticesSize += meshHeader->numVertices * 12;
    }
    if (meshHeader->flags & MeshHeader::HasTexCoords) {
      glEnableVertexAttribArray(3);
      glVertexAttribPointer(3, 2, GL_FLOAT, false, 0, reinterpret_cast<void*>(verticesSize));
      verticesSize += meshHeader->numVertices * 8;
    }
    if (meshHeader->flags & MeshHeader::HasBones) {
      glEnableVertexAttribArray(4);
      glVertexAttribPointer(4, 4, GL_FLOAT, false, 0, reinterpret_cast<void*>(verticesSize));
      verticesSize += meshHeader->numVertices * 16;
      glEnableVertexAttribArray(5);
      glVertexAttribIPointer(5, 4, GL_UNSIGNED_BYTE, 0, reinterpret_cast<void*>(verticesSize));
      verticesSize += meshHeader->numVertices * 4;
    }
    glBufferData(GL_ARRAY_BUFFER, verticesSize, testMeshData.data() + verticesOffset, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesSize, testMeshData.data() + indicesOffset, GL_STATIC_DRAW);
    glBindVertexArray(0);
    glGenBuffers(1, &uboTest);
    glGenBuffers(1, &uboBones);
    glCheck();

    animState.layers = new AnimState::Layer[animation.numLayers];
    animState.localJoints = new Mat4[skeleton.numBones];
    animState.worldJoints = new Mat4[skeleton.numBones];
    animState.finalJoints = new Mat4[skeleton.numBones];

    memcpy(animState.localJoints, skeleton.boneTransforms, skeleton.numBones * sizeof(Mat4));
  #endif
  }

#if GFX_PRESENT_THREAD
  while (true)
  #endif
  {
    r += 0.0001f;
    g += 0.00025f;
    b += 0.00005f;
    if (r > 1) r = 0;
    if (g > 1) g = 0;
    if (b > 1) b = 0;

    world.Step(1 / 60.f, velocityIterations, positionIterations);

    auto& io{ ImGui::GetIO() };
    io.DisplaySize = ImVec2{ gl->width, gl->height };
    io.DisplayFramebufferScale = ImVec2{ gl->dpi, gl->dpi };

    io.DeltaTime = gl->getDeltaTime();
  #if 0
    io.DeltaTime = 0.016;
  #endif

  #if PLATFORM_HTML5
    jank_imgui_setCursor(ImGui::GetMouseCursor());
  #endif

    jank_imgui_newFrame();
    ImGui::NewFrame();

    //bool show = true;
    //ImGui::ShowDemoWindow(&show);

    ImGui::Begin("Hello");
    ImGui::Text("%.3f ms | %.1f FPS", 1000.f / io.Framerate, io.Framerate);
    ImGui::End();

    ImGui::Render();

    auto drawData{ ImGui::GetDrawData() };
    // TODO check totalVtxCount

    auto offset{ drawData->DisplayPos };
    auto scale{ drawData->FramebufferScale };
    auto width{ static_cast<GLsizei>(drawData->DisplaySize.x * scale.x) };
    auto height{ static_cast<GLsizei>(drawData->DisplaySize.y * scale.y) };

  #if GFX_PRESENT_THREAD && (PLATFORM_ANDROID || PLATFORM_APPLE)
    gl->renderReady.wait();
  # if PLATFORM_ANDROID || PLATFORM_IPHONE
    gl->makeCurrent();
  # endif
  #endif

    glEnable(GL_DEPTH_TEST);
    glDepthMask(true);
    glDepthFunc(GL_LESS);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    //glClearColor(r, g, b, 1);
    glClearDepth(1);
    glClear(GL_DEPTH_BUFFER_BIT);
    glCheck();

    glViewport(0, 0, width, height);
    //if (std::fmod(r * 50, 1) > .5)
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    Mat4 proj;
    {
      auto fov = rtm::degrees(60.f).as_radians();
      auto ar = gl->width / gl->height;
      auto f = 1000.f;
      auto n = .01f;
      auto r = n - f;
      auto t = std::tanf(fov * .5f);
      auto x = 1.f / (t * ar);
      auto y = 1.f / t;
      auto a = (-n - f) / r;
      auto b = 2 * f * n / r;
      proj.m4x4 = rtm::matrix_set(rtm::vector_set(x, 0, 0, 0),
                                  rtm::vector_set(0, y, 0, 0),
                                  rtm::vector_set(0, 0, a, 1),
                                  rtm::vector_set(0, 0, b, 0));
    }

    struct Buffers {
      CameraBuffer camera;
      MaterialBuffer material;
      LightBuffer lights[2];
    } buffers;

    static_assert(offsetof(Buffers, lights) == 512);
    static_assert(sizeof(Buffers) == 256 * 3);

    buffers.camera.position = rtm::vector_set(0.f, 0.1f, -0.1f, 1.f);
    //buffers.camera.exposure = std::sinf(r * 300) * 4 + 8;
    buffers.camera.exposure = 1.0f;

    //buffers.material.metallic = 0.05f;
    //buffers.material.roughness = 0.05f;
    buffers.material.refractiveIndex = rtm::float3f{ .05, .05, .05 };
    buffers.material.metallic = 0.05f;
    buffers.material.roughness = 0.95f;
    //buffers.material.refractiveIndex = rtm::float3f{ .56f, .57f, .58f };
    //buffers.material.refractiveIndex = rtm::float3f{ .95f, .64f, .54f };
    //buffers.material.metallic = std::sinf(r * 50) * .5f + .5f;// 0.05f;
    //buffers.material.roughness = std::sinf(g * 50) * .5f + .5f;//0.95f;
    buffers.material.ao = 1.0f;
    buffers.material.useNormalMap = std::fmod(r * 25, 1);

    buffers.lights[0] = {
      rtm::vector_set(std::sinf(r * 100) * 10, 0.1f, std::cosf(r * 100) * 10, 1.f),
      rtm::vector_set(r * 25, g * 25, b * 25, 1.f)
    };
    auto intensity = std::sinf(r * 250) * 50 + 100;
    buffers.lights[1] = {
      rtm::vector_set(10, 1, -1, 1.f),
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
    view.m4x4 = rtm::matrix_identity();
    view.m4x4.w_axis = buffers.camera.position;
    view.m4x4 = rtm::matrix_inverse(view.m4x4);

    auto viewProj = rtm::matrix_mul(view.m4x4, proj.m4x4);

  #if BUILD_EDITOR
    Mat4 model;
    {
      auto q = rtm::quat_from_axis_angle(rtm::vector_set(0, 1.f, 0),
                                         rtm::radians(r * 20));
      model.m3x4 = rtm::matrix_from_qvv(q,
                                        rtm::vector_set(0.f, .05f, 0.f, 1.f),
                                        //rtm::vector_set(0.f, .0f, 0.f, 1.f),
                                        //rtm::vector_set(.001f, .001f, .001f)
                                        rtm::vector_set(.0005f, .0005f, .0005f)
                                        //rtm::vector_set(10, 10, 10.f)
      );
      Mat4 mvp;
      mvp.m4x4 = rtm::matrix_mul(model.m4x4, viewProj);
      glUseProgram(testProg);
      glBindVertexArray(vaoTest);
      glUniformMatrix4fv(0, 1, false, mvp.v);
      glUniformMatrix4fv(1, 1, false, model.v);
      glBindBufferRange(GL_UNIFORM_BUFFER, 0, uboTest, cameraOffset, sizeof(buffers.camera));
      glBindBufferRange(GL_UNIFORM_BUFFER, 1, uboTest, materialOffset, sizeof(buffers.material));
      glBindBufferRange(GL_UNIFORM_BUFFER, 2, uboTest, lightsOffset, sizeof(buffers.lights));
      glBindBufferRange(GL_UNIFORM_BUFFER, 3, uboBones, 0, boneSize);
      glActiveTexture(GL_TEXTURE0 + 2);
      auto f = std::fmod(r * 10, 1);
      glBindTexture(GL_TEXTURE_2D, f > 0.67 ? iblTex0 : f > 0.33 ? iblTex1 : iblTex2);
      for (auto n{ 0 }; n < subMeshesCountTest; n++) {
        auto& sm = subMeshesTest[n];
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, materials[sm.materialIndex].normal);
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, materials[sm.materialIndex].color);
        glDrawElementsBaseVertex(GL_TRIANGLES, sm.indexCount, GL_UNSIGNED_INT,
                                 reinterpret_cast<void const*>(sm.indexOffset),
                                 sm.vertexStart);
        glCheck();
      }
    }
  #endif

  #if 1
    //auto pos = body->GetPosition();
    //auto model = rtm::matrix_from_translation(rtm::vector_set(pos.x, pos.y, 0.f, 1.f));
    glUseProgram(triangleProg);
    glBindVertexArray(vaoTriangle);
    for (auto n{ 0 }; n < 2; n++) {
      Mat4 localToWorld;
      localToWorld.m3x4 = rtm::matrix_from_qvv(rtm::quat_identity(),
                                        buffers.lights[n].position,
                                        rtm::vector_set(0.5f, 0.5f, 0.5f));
      Mat4 mvp;
      mvp.m4x4 = rtm::matrix_mul(localToWorld.m4x4, viewProj);
      glUniformMatrix4fv(mvpPos, 1, false, mvp.v);
      glDrawArrays(GL_TRIANGLES, 0, 3);
      glCheck();
    }
  #endif

    auto skyboxView{ view };
    skyboxView.m4x4.w_axis = rtm::vector_set(0, 0, 0, 1.f);
    glDepthMask(false);
    glUseProgram(skyboxProg);
    glBindVertexArray(vaoGizmo);
    glUniformMatrix4fv(0, 1, false, proj.v);
    glUniformMatrix4fv(1, 1, false, skyboxView.v);
    drawGizmo(cubeGizmo);
    glDepthMask(true);

  #if BUILD_EDITOR && 0
    glClear(GL_DEPTH_BUFFER_BIT);
    f32 color[3]{ .9f, 0.f, 0.f };
    glUseProgram(progGizmo);
    glBindVertexArray(vaoGizmo);
    glUniform3fv(2, 1, color);
    glUniformMatrix4fv(0, 1, false, proj.v);
    drawBones(model, view, animState.worldJoints);
    glCheck();
  #endif

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(false);
    {
      auto l{ drawData->DisplayPos.x };
      auto r{ drawData->DisplaySize.x + l };
      auto t{ drawData->DisplayPos.y };
      auto b{ drawData->DisplaySize.y + t };
      f32 proj[4][4]{
        { 2.f/(r-l),   0,            0, 0 },
        { 0,           2.f/(t-b),    0, 0 },
        { 0,           0,           -1, 0 },
        { (r+l)/(l-r), (t+b)/(b-t),  0, 1 }
      };
      glUseProgram(prog);
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

    //glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glBindVertexArray(0);
    glUseProgram(0);

#if GFX_PRESENT_THREAD && !PLATFORM_ANDROID && !PLATFORM_APPLE
    gl->renderReady.wait();
#endif

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboSurface);
    glBlitFramebuffer(0, 0, gl->width, gl->height,
                      0, 0, gl->width, gl->height,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);

#if GFX_PRESENT_THREAD
# if PLATFORM_ANDROID
    gl->clearCurrent();
# endif
    //glFlush();
    gl->presentReady.set();
#else
    gl->present();
#endif
  }

  //glDeleteTextures(1, &fontTexture);
  //io.Fonts->TexID = 0;

  return nullptr;
}
