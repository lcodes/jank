#pragma once

#include "core/Core.hpp"

#define GFX_PRESENT_THREAD 0 && !PLATFORM_HTML5

#if PLATFORM_POSIX
# include <pthread.h>

class SyncEvent {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
  bool flag;

public:
  SyncEvent(bool initValue = false) : flag(initValue) {
    pthread_mutex_init(&mutex, nullptr);
    pthread_cond_init(&cond, nullptr);
  }

  ~SyncEvent() {
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
  }

  void set() {
    pthread_mutex_lock(&mutex);
    flag = true;
    pthread_mutex_unlock(&mutex);
    pthread_cond_signal(&cond);
  }

  void wait() {
    pthread_mutex_lock(&mutex);
    while (!flag) pthread_cond_wait(&cond, &mutex);
    flag = false;
    pthread_mutex_unlock(&mutex);
  }
};
#elif PLATFORM_WINDOWS
# pragma warning(push, 0)
# include <Windows.h>
# pragma warning(pop)

class SyncEvent {
  HANDLE handle;

public:
  SyncEvent(bool initValue = false) :
    handle(CreateEventW(nullptr, false, initValue, nullptr)) {}

  ~SyncEvent() {
    CloseHandle(handle);
  }

  void set() {
    SetEvent(handle);
  }

  void wait() {
    WaitForSingleObject(handle, INFINITE);
  }
};
#elif PLATFORM_HTML5
// No implementation ?
#else
# error TODO Implement SyncEvent for the target platform
#endif

enum class InputKeys {
  W, S, A, D,
  Shift, Space,
  Count
};

#if 0
class OpenGL {
public:
#if GFX_PRESENT_THREAD
  SyncEvent renderReady;
  SyncEvent presentReady;
#endif

  f32 width;
  f32 height;
  f32 dpi;

  bool input[static_cast<u32>(InputKeys::Count)]{ false, false, false, false, false, false };
  f32 mouseX = -1;
  f32 mouseY = -1;
  f32 xOffset = 0;
  f32 yOffset = 0;

  bool getInput(InputKeys k) const {
    return input[static_cast<u32>(k)];
  }
  f32 getAxis(InputKeys a, InputKeys b) const {
    return getInput(a) ? -1 : getInput(b) ? 1 : 0;
  }

  OpenGL() :
#if GFX_PRESENT_THREAD
    renderReady(true),
#endif
    width(0), height(0), dpi(1) {}

  virtual void* getProcAddress(char const* name UNUSED) { ASSERT(0); UNREACHABLE; }

  virtual void present() {}

  virtual void clearCurrent() = 0;
  virtual void makeCurrent() = 0;

  virtual f64 getDeltaTime() = 0;

#if PLATFORM_IPHONE
  virtual u32 getSurface() = 0;
#endif
};
#endif

#pragma pack(1)
struct MeshHeader {
  enum {
    HasPositions = 1 << 0,
    HasNormals = 1 << 1,
    HasTangents = 1 << 2,
    HasTexCoords = 1 << 3,
    HasBones = 1 << 4
  };
  struct SubMesh {
    u8 materialIndex;
    u8 unused0;
    u16 unused1;
    u32 indexCount;
    u32 indexOffset;
    u32 vertexStart;
  };
  u32 numIndices;
  u32 numVertices;
  u16 flags;
  u16 subMeshCount;
  // submeshes...
  // indices data...
  // vertice data...
};
#pragma pack()

#include "rtm/matrix4x4f.h"
#include "rtm/quatf.h"
#include "rtm/vector4f.h"
#include <string>

enum class TextureType : u8 {
  Base,
  Normal,
  Metallic,
  Roughness,
  AO,
  Displacement,
  Count,
  Unknown = 0xFF,
};
constexpr auto textureTypeCount{ static_cast<u32>(TextureType::Count) };

struct Skeleton {
  static constexpr u8 invalidBoneId = UINT8_MAX;

  std::string* boneNames;
  rtm::matrix4x4f* boneOffsets;
  rtm::matrix4x4f* boneTransforms;
  u8* boneParentIds;
  u8 numBones;
};

struct Animation {
  enum class Behaviour : u8 {
    Default,
    Constant,
    Linear,
    Repeat
  };
  struct Layer {
    f32* rotationKeys;
    f32* positionKeys;
    f32* scaleKeys;
    rtm::quatf* rotations;
    rtm::vector4f* positions; // TODO use vec3's instead?
    rtm::vector4f* scales;
    u32 numRotationFrames;
    u32 numPositionFrames;
    u32 numScaleFrames;
    Behaviour statePre;
    Behaviour statePost;
    u8 boneIndex;
  };

  // Name
  // Target Skeleton
  f32 duration;
  f32 ticksPerSecond;
  Layer* layers;
  u8 numLayers;
};

/**
 * Describes how objects are rendered.
 */
struct MaterialDef {
  // Shader implementation sources (& fallbacks / quality levels)
  // Shader Pipeline Map (vertex type -> pass type)
  // Texture Parameter Defs
  // Uniform Parameter Defs
};

/**
 * Parameter set for a MaterialDef.
 */
struct Material {
  MaterialDef* def;
  u32 textures[static_cast<u32>(TextureType::Count)];
  u32 uboIndex;
  // Uniforms
};

/**
 * Component used for CPU culling of RenderObjects by a Camera.
 */
struct Visibility {
  // bounding box
  // bounding sphere
};

/**
 * Component holding shared data for all specialized render components.
 */
struct RenderObject {
  Material* materials;
  bool shadowed  : 1;
  bool shadowing : 1;
  bool useRProbe : 1;
};

// TODO
// - static batching
// - hierarchical LODs
struct LODs {
};

class IndexBuffer {
  u32 ibo;

public:
};

class VertexBuffer {
  u32 vbo;

public:
};

struct VertexStream {
  u32 vao;
};

template<typename T>
class TrailingArray {
  u32 count;
  T data[0];
};

struct StaticMeshData {
  struct SubMesh {
    VertexStream stream;
    u8 materialIndex;
    u8 unused;
    u16 unused2;
    u32 indexCount;
    u32 indexOffset;
    u32 vertexStart;
  };

  IndexBuffer indices;
  VertexBuffer positions;
  VertexBuffer attributes;
  TrailingArray<SubMesh> subMeshes;
};

struct StaticMeshAsset {
  StaticMeshData* data;
};

struct SkinnedMeshData {
  struct SubMesh {

  };

  u32 subMeshCount;
  SubMesh subMeshes[0];
};

/**
 * Renderer data for meshes whose geometry never changes.
 */
struct StaticMesh {
  StaticMeshData* data;
};

/**
 * Renderer data for meshes whose geometry is animated using a skeleton.
 */
struct SkinnedMesh {
  SkinnedMeshData* data;
  bool offscreenUpdate : 1;
};

// Particles
// Lines
// Trail

// ENTITY ID
// - LocalToWorld
// - Visibility
// - RenderObject
// - StaticMesh
//   - StaticMeshData
//     - Index & Vertex buffers
//     - SubMeshes
//       - Vertex Stream  -> IBO/VBO offsets, size, stride -> VAO/Views
//       - Material Index -> RenderObject.materials[index] -> Shader
//       - Draw Call Params

