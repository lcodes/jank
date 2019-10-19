#include "app/App.hpp"
#include "app/GfxTest.hpp"

#include "rtm/vector4f.h"
#include "Box2D/Box2D.h"
#include "imgui.h"

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

#include <string>

using namespace std::literals;

DECL_LOG_SOURCE(Test, Info);

constexpr u16 glMakeVersion(i32 major, i32 minor) {
  ASSERT(major > 0 && minor >= 0 && major < 10 && minor < 10);
  return static_cast<u16>((major << 8) | minor);
}

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


// Dynamic Link Setup
// -----------------------------------------------------------------------------

#if PLATFORM_LINUX || PLATFORM_WINDOWS

// TODO: looks like windows has GL1.1 and linux has GL1.4 we can directly link against
# if PLATFORM_WINDOWS
#  define GL1_2_PROCS \
  GL(TEXIMAGE3D, TexImage3D); \
  GL(TEXSUBIMAGE3D, TexSubImage3D)

#  define GL1_3_PROCS \
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
  GL(UNIFORMMATRIX4FV, UniformMatrix4fv); \
  GL(USEPROGRAM, UseProgram); \
  GL(VERTEXATTRIBPOINTER, VertexAttribPointer)

# define GL3_0_PROCS \
  GL(BINDRENDERBUFFER, BindRenderbuffer); \
  GL(BINDFRAMEBUFFER, BindFramebuffer); \
  GL(BINDVERTEXARRAY, BindVertexArray); \
  GL(BLITFRAMEBUFFER, BlitFramebuffer); \
  GL(CHECKFRAMEBUFFERSTATUS, CheckFramebufferStatus); \
  GL(DELETEFRAMEBUFFERS, DeleteFramebuffers); \
  GL(DELETERENDERBUFFERS, DeleteRenderbuffers); \
  GL(DELETEVERTEXARRAYS, DeleteVertexArrays); \
  GL(FRAMEBUFFERTEXTURE2D, FramebufferTexture2D); \
  GL(GENFRAMEBUFFERS, GenFramebuffers); \
  GL(GENRENDERBUFFERS, GenRenderbuffers); \
  GL(GENVERTEXARRAYS, GenVertexArrays); \
  GL(GETSTRINGI, GetStringi)

# define GL3_1_PROCS \

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

#define glCheck() glCheckImpl(__FILE__, __LINE__)

static void glCheckImpl(char const* file, u32 line) {
  if (auto e{ glGetError() }; UNLIKELY(e != GL_NO_ERROR)) {
    char const* str;
    switch (e) {
#define E(x) case GL_##x: str = #x; break
      E(INVALID_ENUM);
      E(INVALID_VALUE);
      E(INVALID_OPERATION);
      E(OUT_OF_MEMORY);
      E(INVALID_FRAMEBUFFER_OPERATION);
#undef E
    default: ASSERT(0, "Unknown GL Error: %04x", e); UNREACHABLE;
    }
    assertFailure(file, line, str);
  }
}

static void glCheckFramebuffer() {
  if (auto e{ glCheckFramebufferStatus(GL_FRAMEBUFFER) }; UNLIKELY(e != GL_FRAMEBUFFER_COMPLETE)) {
    char const* str;
    switch (e) {
#define E(x) case GL_FRAMEBUFFER_##x: str = #x; break;
      E(INCOMPLETE_ATTACHMENT);
      //E(INCOMPLETE_DIMENSIONS);
      E(INCOMPLETE_MISSING_ATTACHMENT);
      E(UNSUPPORTED);
    default: ASSERT(0, "Unknown GL Framebuffer Error: %04x", e); UNREACHABLE;
#undef E
    }
    ASSERT(0, str);
  }
}

#define GL_CHECK_INFO_LOG(name, get, getInfoLog, status, action) \
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
#undef GL_CHECK_INFO_LOG


// OpenGL Extension Setup
// -----------------------------------------------------------------------------

static struct {
  usize ARB_ES2_compatibility:                1;
  usize ARB_ES3_compatibility:                1;
  usize ARB_ES3_1_compatibility:                1;
  usize ARB_ES3_2_compatibility:                1;
  usize ARB_arrays_of_arrays: 1;
  usize ARB_base_instance : 1;
  usize ARB_bindless_texture : 1;
  usize ARB_blend_func_extended : 1;
  usize ARB_buffer_storage : 1;
  usize ARB_clear_buffer_object:              1;
  usize ARB_clear_texture : 1;
  usize ARB_clip_control : 1;
  usize ARB_color_buffer_float:               1;
  usize ARB_compressed_texture_pixel_storage: 1;
  usize ARB_compute_shader : 1;
  usize ARB_compute_variable_group_size : 1;
  usize ARB_conditional_render_inverted : 1;
  usize ARB_conservative_depth : 1;
  usize ARB_copy_buffer:                      1;
  usize ARB_copy_image : 1;
  usize ARB_cull_distance : 1;
  usize ARB_debug_output:                     1;
  usize ARB_depth_buffer_float: 1;
  usize ARB_depth_clamp: 1;
  usize ARB_depth_texture:                    1;
  usize ARB_derivative_control: 1;
  usize ARB_direct_state_access: 1;
  usize ARB_draw_buffers_blend: 1;
  usize ARB_draw_elements_base_vertex:        1;
  usize ARB_draw_indirect:1;
  usize ARB_draw_instanced: 1;
  usize ARB_enhanced_layouts: 1;
  usize ARB_explicit_attrib_location:         1;
  usize ARB_explicit_uniform_location:        1;
  usize ARB_fragment_coord_conventions:       1;
  usize ARB_fragment_layer_viewport: 1;
  usize ARB_fragment_shader_interlock: 1;
  usize ARB_framebuffer_no_attachments : 1;
  usize ARB_framebuffer_object:               1;
  usize ARB_framebuffer_sRGB:                 1;
  usize ARB_geometry_shader4 : 1;
  usize ARB_get_program_binary:               1;
  usize ARB_get_texture_sub_image:            1;
  usize ARB_gl_spirv : 1;
  usize ARB_gpu_shader5 : 1;
  usize ARB_gpu_shader_fp64 : 1;
  usize ARB_gpu_shader_int64 : 1;
  usize ARB_half_float_pixel:                 1;
  usize ARB_half_float_vertex:                1;
  usize ARB_imaging: 1;
  usize ARB_indirect_parameters: 1;
  usize ARB_instanced_arrays : 1;
  usize ARB_internalformat_query:             1;
  usize ARB_internalformat_query2:            1;
  usize ARB_invalidate_subdata:               1;
  usize ARB_map_buffer_alignment:             1;
  usize ARB_map_buffer_range:                 1;
  usize ARB_multi_bind:                       1;
  usize ARB_multi_draw_indirect: 1;
  usize ARB_multisample:                      1;
  usize ARB_occlusion_query2:                 1;
  usize ARB_parallel_shader_compile : 1;
  usize ARB_pipeline_statistics_query: 1;
  usize ARB_polygon_offset_clamp: 1;
  usize ARB_post_depth_coverage:1;
  usize ARB_program_interface_query:          1;
  usize ARB_provoking_vertex:                 1;
  usize ARB_query_buffer_object : 1;
  usize ARB_robust_buffer_access_behavior: 1;
  usize ARB_robustness:                       1;
  usize ARB_sample_locations: 1;
  usize ARB_sample_shading:1;
  usize ARB_sampler_objects:                  1;
  usize ARB_seamless_cube_map: 1;
  usize ARB_seamless_cubemap_per_texture: 1;
  usize ARB_separate_shader_objects:          1;
  usize ARB_shader_atomic_counters : 1;
  usize ARB_shader_atomic_counter_ops : 1;
  usize ARB_shader_ballot:1;
  usize ARB_shader_bit_encoding:1;
  usize ARB_shader_clock: 1;
  usize ARB_shader_draw_parameters : 1;
  usize ARB_shader_group_vote: 1;
  usize ARB_shader_image_load_store: 1;
  usize ARB_shader_image_size: 1;
  usize ARB_shader_precision : 1;
  usize ARB_shader_storage_buffer_object : 1;
  usize ARB_shader_subroutine : 1;
  usize ARB_shader_texture_lod:               1;
  usize ARB_shader_texture_image_samples : 1;
  usize ARB_shader_viewport_layer_array : 1;
  usize ARB_shading_language_420pack : 1;
  usize ARB_shading_language_include : 1;
  usize ARB_shading_language_packing : 1;
  usize ARB_sparse_buffer : 1;
  usize ARB_sparse_texture : 1;
  usize ARB_sparse_texture2 : 1;
  usize ARB_sparse_texture_clamp : 1;
  usize ARB_spirv_extensions : 1;
  usize ARB_stencil_texturing: 1;
  usize ARB_sync:                             1;
  usize ARB_tessellation_shader : 1;
  usize ARB_texture_barrier : 1;
  usize ARB_texture_buffer_object : 1;
  usize ARB_texture_buffer_object_rgb32 : 1;
  usize ARB_texture_buffer_range : 1;
  usize ARB_texture_compression_bptc : 1;
  usize ARB_texture_compression_rgtc : 1;
  usize ARB_texture_cube_map_array : 1;
  usize ARB_texture_filter_anisotropic:       1;
  usize ARB_texture_filter_minmax: 1;
  usize ARB_texture_float:                    1;
  usize ARB_texture_gather:1;
  usize ARB_texture_mirror_clamp_to_edge: 1;
  usize ARB_texture_multisample: 1;
  usize ARB_texture_query_levels: 1;
  usize ARB_texture_query_lod;
  usize ARB_texture_rectangle:                1;
  usize ARB_texture_rg: 1;
  usize ARB_texture_rgb10_a2ui:1;
  usize ARB_texture_stencil8:1;
  usize ARB_texture_storage:                  1;
  usize ARB_texture_storage_multisample: 1;
  usize ARB_texture_swizzle:                  1;
  usize ARB_texture_view : 1;
  usize ARB_timer_query : 1;
  usize ARB_transform_feedback2 : 1;
  usize ARB_transform_feedback3 : 1;
  usize ARB_transform_feedback_instanced : 1;
  usize ARB_transform_feedback_overflow_query : 1;
  usize ARB_uniform_buffer_object : 1;
  usize ARB_vertex_array_bgra:                1;
  usize ARB_vertex_array_object:              1;
  usize ARB_vertex_attrib_64bit : 1;
  usize ARB_vertex_attrib_binding:            1;
  usize ARB_vertex_type_10f_11f_11f_rev : 1;
  usize ARB_vertex_type_2_10_10_10_rev : 1;
  usize ARB_viewport_array : 1;
  usize ARB_window_pos:                       1;
  usize EXT_Cg_shader: 1;
  usize EXT_EGL_image_array: 1;
  usize EXT_EGL_image_external_wrap_modes: 1;
  usize EXT_EGL_image_storage: 1;
  usize EXT_YUV_target: 1;
  usize EXT_abgr:                             1;
  usize EXT_bindable_uniform: 1;
  usize EXT_blend_func_extended: 1;
  usize EXT_blit_framebuffer_params: 1;
  usize EXT_buffer_storage: 1;
  usize EXT_clip_control: 1;
  usize EXT_clip_cull_distance: 1;
  usize EXT_color_buffer_float: 1;
  usize EXT_color_buffer_half_float: 1;
  usize EXT_copy_image: 1;
  usize EXT_debug_label:1;
  usize EXT_debug_marker:1;
  usize EXT_direct_state_access: 1;
  usize EXT_discard_framebuffer: 1;
  usize EXT_disjoint_timer_query: 1;
  usize EXT_draw_buffers2: 1;
  usize EXT_draw_buffers_indexed: 1;
  usize EXT_draw_instanced: 1;
  usize EXT_external_buffer: 1;
  usize EXT_fragment_invocation_density: 1;
  usize EXT_framebuffer_blit:                 1;
  usize EXT_framebuffer_multisample: 1;
  usize EXT_framebuffer_object:               1;
  usize EXT_framebuffer_sRGB:                 1;
  usize EXT_geometry_shader: 1;
  usize EXT_geometry_shader4: 1;
  usize EXT_gpu_shader4: 1;
  usize EXT_gpu_shader5: 1;
  usize EXT_import_sync_object: 1;
  usize EXT_memory_object: 1;
  usize EXT_memory_object_fd: 1;
  usize EXT_multisampled_render_to_texture: 1;
  usize EXT_multisampled_render_to_texture2: 1;
  usize EXT_packed_depth_stencil:             1;
  usize EXT_packed_float: 1;
  usize EXT_polygon_offset_clamp: 1;
  usize EXT_post_depth_coverage: 1;
  usize EXT_primitive_bounding_box: 1;
  usize EXT_protected_textures: 1;
  usize EXT_provoking_vertex:                 1;
  usize EXT_pvrtc_sRGB: 1;
  usize EXT_raster_multisample: 1;
  usize EXT_read_format_bgra: 1;
  usize EXT_robustness: 1;
  usize EXT_sRGB: 1;
  usize EXT_sRGB_write_control: 1;
  usize EXT_semaphore : 1;
  usize EXT_separate_shader_objects: 1;
  usize EXT_shader_framebuffer_fetch: 1;
  usize EXT_shader_image_load_formatted: 1;
  usize EXT_shader_image_load_store: 1;
  usize EXT_shader_integer_mix: 1;
  usize EXT_shader_io_blocks: 1;
  usize EXT_shader_non_constant_global_initializers: 1;
  usize EXT_shader_texture_lod: 1;
  usize EXT_shadow_samplers: 1;
  usize EXT_sparse_texture2: 1;
  usize EXT_tessellation_shader: 1;
  usize EXT_texture_array: 1;
  usize EXT_texture_border_clamp: 1;
  usize EXT_texture_buffer: 1;
  usize EXT_texture_buffer_object: 1;
  usize EXT_texture_compression_dxt1:         1;
  usize EXT_texture_compression_latc: 1;
  usize EXT_texture_compression_rgtc: 1;
  usize EXT_texture_compression_s3tc:         1;
  usize EXT_texture_cube_map_array: 1;
  usize EXT_texture_filter_anisotropic:       1;
  usize EXT_texture_filter_minmax: 1;
  usize EXT_texture_format_BGRA8888: 1;
  usize EXT_texture_format_sRGB_override: 1;
  usize EXT_texture_integer: 1;
  usize EXT_texture_lod: 1;
  usize EXT_texture_mirror_clamp:1;
  usize EXT_texture_norm16: 1;
  usize EXT_texture_rectangle:                1;
  usize EXT_texture_sRGB_decode:              1;
  usize EXT_texture_sRGB_R8: 1;
  usize EXT_texture_shared_exponent: 1;
  usize EXT_texture_storage: 1;
  usize EXT_texture_swizzle:                  1;
  usize EXT_texture_type_2_10_10_10_REV: 1;
  usize EXT_timer_query: 1;
  usize EXT_transform_feedback2: 1;
  usize EXT_vertex_array_bgra:                1;
  usize EXT_vertex_attrib_64bit: 1;
  usize EXT_window_rectangles: 1;
  usize EXTX_framebuffer_mixed_formats: 1;
  usize KHR_blend_equation_advanced: 1;
  usize KHR_blend_equation_advanced_coherent: 1;
  usize KHR_debug:                            1;
  usize KHR_context_flush_control:            1;
  usize KHR_no_error:                         1;
  usize KHR_parallel_shader_compile : 1;
  usize KHR_robust_buffer_access_behavior: 1;
  usize KHR_robustness: 1;
  usize KHR_texture_compression_astc_ldr: 1;
  usize KHR_texture_compression_astc_hdr: 1;
  usize KTX_buffer_region : 1;
  usize OES_EGL_image:                        1;
  usize OES_EGL_image_external: 1;
  usize OES_EGL_image_external_essl3: 1;
  usize OES_EGL_sync: 1;
  usize OES_compressed_ETC1_RGB8_texture: 1;
  usize OES_depth24: 1;
  usize OES_depth_texture: 1;
  usize OES_depth_texture_cube_map: 1;
  usize OES_element_index_uint: 1;
  usize OES_framebuffer_object: 1;
  usize OES_get_program_binary: 1;
  usize OES_packed_depth_stencil: 1;
  usize OES_read_format:                      1;
  usize OES_rgb8_rgba8: 1;
  usize OES_shader_image_atomic: 1;
  usize OES_sample_shading: 1;
  usize OES_sample_variables: 1;
  usize OES_shader_multisample_interpolation: 1;
  usize OES_surfaceless_context: 1;
  usize OES_standard_derivatives: 1;
  usize OES_texture_3D: 1;
  usize OES_texture_compression_astc: 1;
  usize OES_texture_float: 1;
  usize OES_texture_float_linear: 1;
  usize OES_texture_format_sRGB_override: 1;
  usize OES_texture_half_float: 1;
  usize OES_texture_half_float_linear: 1;
  usize OES_texture_npot: 1;
  usize OES_texture_stencil8: 1;
  usize OES_texture_storage_multisample_2d_array: 1;
  usize OES_texture_type_2_10_10_10_REV: 1;
  usize OES_texture_view: 1;
  usize OES_vertex_array_object: 1;
  usize OES_vertex_half_float: 1;
  usize AMD_compressed_ATC_texture: 1;
  usize AMD_multi_draw_indirect: 1;
  usize AMD_seamless_cubemap_per_texture: 1;
  usize AMD_shader_trinary_minmax:            1;
  usize AMD_vertex_shader_viewport_index: 1;
  usize AMD_vertex_shader_layer: 1;
  usize ANDROID_extension_pack_es31a: 1;
  usize ANGLE_texture_compression_dxt3:       1;
  usize ANGLE_texture_compression_dxt5:       1;
  usize APPLE_clip_distance: 1;
  usize APPLE_color_buffer_packed_float: 1;
  usize APPLE_copy_texture_levels: 1;
  usize APPLE_rgb_422:1;
  usize APPLE_texture_format_BGRA8888: 1;
  usize ARM_shader_framebuffer_fetch_depth_stencil: 1;
  usize ATI_texture_float:                    1;
  usize ATI_texture_mirror_once:1;
  usize IMG_read_format: 1;
  usize IMG_texture_compression_pvrtc: 1;
  usize MESA_window_pos:                      1;
  usize NV_ES1_1_compatibility : 1;
  usize NV_ES3_1_compatibility : 1;
  usize NV_bindless_multi_draw_indirect : 1;
  usize NV_bindless_multi_draw_indirect_count : 1;
  usize NV_bindless_texture: 1;
  usize NV_blend_equation_advanced: 1;
  usize NV_blend_equation_advanced_coherent : 1;
  usize NV_compute_program5 : 1;
  usize NV_compute_shader_derivatives : 1;
  usize NV_conditional_render : 1;
  usize NV_conservative_raster : 1;
  usize NV_conservative_raster_dilate : 1;
  usize NV_conservative_raster_pre_snap : 1;
  usize NV_conservative_raster_pre_snap_triangles : 1;
  usize NV_conservative_raster_underestimation : 1;
  usize NV_draw_texture : 1;
  usize NV_draw_vulkan_image : 1;
  usize NV_feature_query : 1;
  usize NV_fence : 1;
  usize NV_geometry_shader4 : 1;
  usize NV_geometry_shader_passthrough : 1;
  usize NV_gpu_program4 : 1;
  usize NV_gpu_program4_1 : 1;
  usize NV_gpu_program5 : 1;
  usize NV_gpu_program5_mem_extended : 1;
  usize NV_gpu_program_fp64 : 1;
  usize NV_gpu_shader5 : 1;
  usize NV_half_float : 1;
  usize NV_memory_attachment : 1;
  usize NV_mesh_shader : 1;
  usize NV_packed_depth_stencil:              1;
  usize NV_parameter_buffer_object : 1;
  usize NV_parameter_buffer_object2 : 1;
  usize NV_path_rendering : 1;
  usize NV_path_rendering_shared_edge : 1;
  usize NV_primitive_restart:                 1;
  usize NV_shader_atomic_counters : 1;
  usize NV_shader_atomic_float : 1;
  usize NV_shader_atomic_float64 : 1;
  usize NV_shader_atomic_fp16_vector : 1;
  usize NV_shader_atomic_int64 : 1;
  usize NV_shader_buffer_load : 1;
  usize NV_shader_noperspective_interpolation: 1;
  usize NV_shader_storage_buffer_object : 1;
  usize NV_shader_texture_footprint : 1;
  usize NV_shader_thread_group : 1;
  usize NV_shader_thread_shuffle : 1;
  usize NV_stereo_view_rendering : 1;
  usize NV_texture_barrier : 1;
  usize NV_texture_compression_vtc : 1;
  usize NV_texture_rectangle:                 1;
  usize NV_texture_shader : 1;
  usize NV_texture_shader2 : 1;
  usize NV_texture_shader3 : 1;
  usize NV_transform_feedback : 1;
  usize NV_transform_feedback2 : 1;
  usize NV_uniform_buffer_unified_memory : 1;
  usize NV_vertex_array_range : 1;
  usize NV_vertex_array_range2 : 1;
  usize NV_vertex_attrib_64bit : 1;
  usize NV_vertex_buffer_unified_memory : 1;
  usize NVX_blend_equation_advanced_multi_draw_buffers : 1;
  usize NVX_conditional_render : 1;
  usize NVX_gpu_memory_info : 1;
  usize NVX_multigpu_info : 1;
  usize NVX_nvenc_interop : 1;
  usize OVR_multiview : 1;
  usize OVR_multiview2 : 1;
  usize OVR_multiview_multisampled_render_to_texture: 1;
  usize QCOM_alpha_test: 1;
  usize QCOM_texture_foveated: 1;
  usize QCOM_tiled_rendering: 1;
  usize QCOM_shader_framebuffer_fetch_noncoherent: 1;
  usize QCOM_shader_framebuffer_fetch_rate: 1;
  usize S3_s3tc:                              1;
  usize SGIS_generate_mipmap:                 1;
#if PLATFORM_WINDOWS
  usize EXT_memory_object_win32: 1;
  usize EXT_semaphore_win32: 1;
  usize EXT_win32_keyed_mutex: 1;
  usize WIN_swap_hint: 1;
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
# define IGNORE(name) else if ("GL_"#name##sv == ext) {}
  if (false) {}
  IGNORE(ARB_multitexture)             // 1.2
  IGNORE(ARB_texture_border_clamp)     // 1.3
  IGNORE(ARB_texture_compression)      // 1.3
  IGNORE(ARB_texture_cube_map)         // 1.3
  IGNORE(ARB_depth_texture)            // 1.4
  IGNORE(ARB_point_parameters)         // 1.4
  IGNORE(ARB_texture_mirrored_repeat)  // 1.4
  IGNORE(ARB_occlusion_query)          // 1.5
  IGNORE(ARB_vertex_buffer_object)     // 1.5
  IGNORE(ARB_draw_buffers)             // 2.0
  IGNORE(ARB_fragment_shader)          // 2.0
  IGNORE(ARB_shader_objects)           // 2.0
  IGNORE(ARB_shading_language_100)     // 2.0
  IGNORE(ARB_texture_non_power_of_two) // 2.0
  IGNORE(ARB_vertex_shader)            // 2.0
  IGNORE(ARB_pixel_buffer_object)      // 2.1
  IGNORE(EXT_copy_texture)             // 1.1
  IGNORE(EXT_subtexture)               // 1.1
  IGNORE(EXT_texture)                  // 1.1
  IGNORE(EXT_texture_object)           // 1.1
  IGNORE(EXT_vertex_array)             // 1.1
  IGNORE(EXT_bgra)                     // 1.2
  IGNORE(EXT_draw_range_elements)      // 1.2
  IGNORE(EXT_packed_pixels)            // 1.2
  IGNORE(EXT_texture3D)                // 1.2
  IGNORE(EXT_texture_edge_clamp)       // 1.2
  IGNORE(EXT_texture_cube_map)         // 1.3
  IGNORE(EXT_blend_color)              // 1.3
  IGNORE(EXT_blend_func_separate)      // 1.4
  IGNORE(EXT_blend_minmax)             // 1.4
  IGNORE(EXT_blend_subtract)           // 1.4
  IGNORE(EXT_multi_draw_arrays)        // 1.4
  IGNORE(EXT_point_parameters)         // 1.4
  IGNORE(EXT_stencil_wrap)             // 1.4
  IGNORE(EXT_texture_lod_bias)         // 1.4
  IGNORE(EXT_blend_equation_separate)  // 2.0
  IGNORE(EXT_pixel_buffer_object)      // 2.1
  IGNORE(EXT_texture_sRGB)             // 2.1
  IGNORE(APPLE_packed_pixels)          // 1.2
  IGNORE(ATI_blend_equation_separate)  // 2.0
  IGNORE(ATI_separate_stencil)         // 2.0
  IGNORE(IBM_texture_mirrored_repeat)  // 1.4
  IGNORE(INGR_blend_func_separate)     // 1.4
  IGNORE(SGIS_texture_edge_clamp)      // 1.2
  IGNORE(SGIS_texture_lod)             // 1.2
  IGNORE(SGIS_texture_border_clamp)    // 1.3
  IGNORE(SUN_multi_draw_arrays)        // 1.4
  // Deprecated / Misc. Ignored
  IGNORE(ARB_fragment_program)
  IGNORE(ARB_fragment_program_shadow)
  IGNORE(ARB_point_sprite)
  IGNORE(ARB_shadow)
  IGNORE(ARB_texture_env_add)
  IGNORE(ARB_texture_env_combine)
  IGNORE(ARB_texture_env_crossbar)
  IGNORE(ARB_texture_env_dot3)
  IGNORE(ARB_transpose_matrix)
  IGNORE(ARB_vertex_program)
  IGNORE(EXT_compiled_vertex_array)
  IGNORE(EXT_depth_bounds_test)
  IGNORE(EXT_framebuffer_multisample_blit_scaled)
  IGNORE(EXT_fog_coord)
  IGNORE(EXT_gpu_program_parameters)
  IGNORE(EXT_rescale_normal)
  IGNORE(EXT_texture_env_add)
  IGNORE(EXT_texture_env_combine)
  IGNORE(EXT_texture_env_dot3)
  IGNORE(EXT_texture_shadow_funcs)
  IGNORE(EXT_secondary_color)
  IGNORE(EXT_separate_specular_color)
  IGNORE(EXT_shadow_funcs)
  IGNORE(EXT_stencil_two_side)
  IGNORE(APPLE_client_storage)
  IGNORE(APPLE_container_object_shareable)
  IGNORE(APPLE_flush_render)
  IGNORE(APPLE_row_bytes)
  IGNORE(APPLE_texture_range)
  IGNORE(ATI_draw_buffers)
  IGNORE(ATI_texture_env_combine3)
  IGNORE(ATI_fragment_shader)
  IGNORE(IBM_multimode_draw_arrays)
  IGNORE(IBM_rasterpos_clip)
  IGNORE(MESA_pack_invert)
  IGNORE(NV_blend_square)
  IGNORE(NV_fog_distance)
  IGNORE(NV_light_max_exponent)
  IGNORE(NV_texgen_reflection)
  IGNORE(NV_texture_env_combine4)
  IGNORE(NV_register_combiners)
  IGNORE(NV_register_combiners2)
  else {
    LOG(App, Warn, "Unknown GL Extension: %.*s", static_cast<u32>(ext.size()), ext.data());
  }
# undef IGNORE
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

static bool hasInit;
static u32 fbo, tex;
static u32 vaoTriangle;
static u32 vboTriangle;
static u32 triangleProg, prog, texLoc, projLoc;
static u32 vao;
static u32 bufs[2];
static GLuint fontTexture;
static float r{0}, g{0}, b{0};

constexpr i32 velocityIterations = 6;
constexpr i32 positionIterations = 2;

static b2World world{{0, -10}};
static b2Body* groundBody;
static b2Body* body;
static u32 worldPos;

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

      for (auto i{0}; i < numExts; i++) {
        auto str{ reinterpret_cast<char const*>(glGetStringi(GL_EXTENSIONS, i)) };
        setupExtension({ str });
      }
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
    fixtureDef.friction = .3;
    body->CreateFixture(&fixtureDef);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    jank_imgui_init();

    auto& io{ ImGui::GetIO() };
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.SetClipboardTextFn = jank_imgui_setClipboardText;
    io.GetClipboardTextFn = jank_imgui_getClipboardText;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, gl->width, gl->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glCheckFramebuffer();

    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboSurface);
    glCheck();
    LOG(Test, Info, "Framebuffer");

    // Triangle init
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

#if PLATFORM_MACOS
# define GLSL_VERSION "#version 410 core\n"
#else
# define GLSL_VERSION "#version 300 es\n" "precision mediump float;\n"
#endif

    auto triangleVertexSource =
      GLSL_VERSION
      "layout (location = 0) in vec3 pos;\n"
      "uniform vec2 worldPos;\n"
      "void main() {\n"
      "  gl_Position = vec4(pos.xy + worldPos, pos.z, 1);\n"
      "}\n";

    auto triangleFragmentSource =
      GLSL_VERSION
      "layout (location = 0) out vec4 col;\n"
      "void main() {\n"
      "  col = vec4(.4, .5, .6, 1);\n"
      "}\n";

    auto triangleVert{ glCreateShader(GL_VERTEX_SHADER) };
    auto triangleFrag{ glCreateShader(GL_FRAGMENT_SHADER) };
    glShaderSource(triangleVert, 1, &triangleVertexSource,   nullptr);
    glShaderSource(triangleFrag, 1, &triangleFragmentSource, nullptr);
    glCompileShader(triangleVert);
    glCompileShader(triangleFrag);
    glCheckShader(triangleVert);
    glCheckShader(triangleFrag);

    triangleProg = glCreateProgram();
    glAttachShader(triangleProg, triangleVert);
    glAttachShader(triangleProg, triangleFrag);
    glLinkProgram(triangleProg);
    glCheckProgram(triangleProg);

    worldPos = glGetUniformLocation(triangleProg, "worldPos");
    LOG(Test, Info, "Triangle Program");

    // ImGui Init
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

    auto vert{ glCreateShader(GL_VERTEX_SHADER) };
    auto frag{ glCreateShader(GL_FRAGMENT_SHADER) };
    glShaderSource(vert, 1, &vertexSource, nullptr);
    glShaderSource(frag, 1, &fragmentSource, nullptr);
    glCompileShader(vert);
    glCompileShader(frag);
    glCheckShader(vert);
    glCheckShader(frag);

    prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glCheckProgram(prog);

    texLoc = glGetUniformLocation(prog, "tex");
    projLoc = glGetUniformLocation(prog, "proj");
    LOG(Test, Info, "ImGUI Program");

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

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
    LOG(Test, Info, "ImGUI VAO");

    // ImGui init
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
    LOG(Test, Info, "ImGUI Texture");

    // ImGui state
    glDisable(GL_DEPTH_TEST);
    glDepthMask(false);

#if PLATFORM_MACOS
    // FIXME: getting random crashes using APPLE's software renderer
    //        this seems to fix it? maybe spawning thread too quickly?
    //usleep(100000);
#endif
  }

#if GFX_PRESENT_THREAD
  while (true)
#endif
  {
    r += 0.001;
    g += 0.0025;
    b += 0.0005;
    if (r > 1) r = 0;
    if (g > 1) g = 0;
    if (b > 1) b = 0;

    world.Step(1/60.f, velocityIterations, positionIterations);

    auto& io{ ImGui::GetIO() };
    io.DisplaySize = ImVec2{gl->width, gl->height};
    io.DisplayFramebufferScale = ImVec2{gl->dpi, gl->dpi};

    io.DeltaTime = gl->getDeltaTime();
#if 0
    io.DeltaTime = 0.016;
#endif

    #if PLATFORM_HTML5
    jank_imgui_setCursor(ImGui::GetMouseCursor());
    #endif

    jank_imgui_newFrame();
    ImGui::NewFrame();

    bool show = true;
    ImGui::ShowDemoWindow(&show);

    ImGui::Begin("Hello");
    ImGui::Text("%.3f ms | %.1f FPS", 1000.f / io.Framerate, io.Framerate);
    ImGui::End();

    ImGui::Render();

    auto drawData{ ImGui::GetDrawData() };
    // TODO check totalVtxCount

    auto offset{ drawData->DisplayPos };
    auto scale { drawData->FramebufferScale };
    auto width { static_cast<GLsizei>(drawData->DisplaySize.x * scale.x) };
    auto height{ static_cast<GLsizei>(drawData->DisplaySize.y * scale.y) };

#if GFX_PRESENT_THREAD && (PLATFORM_ANDROID || PLATFORM_APPLE)
    gl->renderReady.wait();
# if PLATFORM_ANDROID || PLATFORM_IPHONE
    gl->makeCurrent();
# endif
#endif

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glClearColor(r, g, b, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glCheck();

    glViewport(0, 0, width, height);

    glUseProgram(triangleProg);
    glUniform2fv(worldPos, 1, reinterpret_cast<float const*>(&body->GetPosition()));
    glBindVertexArray(vaoTriangle);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glCheck();

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
