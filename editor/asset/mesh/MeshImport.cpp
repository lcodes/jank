#include "core/Core.hpp"

#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/ProgressHandler.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>


// Logger Hook
// ----------------------------------------------------------------------------

DECL_LOG_SOURCE(ImportMesh, Debug);

static class Logger : public Assimp::Logger {
  static void log(LogLevel level, char const* msg) {
    ::log(LOG_SOURCE_ARGS(ImportMesh), level, msg);
  }

protected:
#if BUILD_DEVELOPMENT
  void OnDebug(char const* msg) override { log(LogLevel::Debug, msg); }
#else
  void OnDebug(char const* msg) override {}
#endif
  void OnInfo (char const* msg) override { log(LogLevel::Info,  msg); }
  void OnWarn (char const* msg) override { log(LogLevel::Warn,  msg); }
  void OnError(char const* msg) override { log(LogLevel::Error, msg); }

public:
  bool attachStream (Assimp::LogStream*, u32) override { return false; }
  bool detatchStream(Assimp::LogStream*, u32) override { return false; }

  Logger() {
    Assimp::DefaultLogger::set(this);
  }
} logger;


// Progress Status
// ----------------------------------------------------------------------------

class ProgressHandler : public Assimp::ProgressHandler {
public:
  bool Update(f32 percentage) override {
    return true;
  }

  void UpdateFileRead(i32 currentStep, i32 numberOfSteps) override {

  }

  void UpdatePostProcess(i32 currentStep, i32 numberOfSteps) override {

  }
};


// Importer Utilities
// ----------------------------------------------------------------------------

template<typename T>
constexpr auto snorm(f32 f) {
  auto c = std::clamp(f, -1.f, 1.f) * std::numeric_limits<T>::max();
  return static_cast<T>(c >= 0 ? c + .5f : c - .5f);
}

template<typename T>
constexpr auto unorm(f32 f) {
  return static_cast<T>(std::clamp(f, 0.f, 1.f) * std::numeric_limits<T>::max() + .5f);
}

static_assert(unorm<u16>(0.f) == 0);
static_assert(unorm<u16>(1.f) == UINT16_MAX);

static_assert(snorm<i16>(0.f) == 0);
static_assert(snorm<i16>(1.f) == INT16_MAX);
static_assert(snorm<i16>(-1.f) == -INT16_MAX);


// Importer
// ----------------------------------------------------------------------------
#include "app/GfxTest.hpp"
std::string importMesh() {
  Assimp::Importer imp;
  ProgressHandler progressHandler;
  imp.SetProgressHandler(&progressHandler);
  //imp.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, removeComponents);

  auto processFlags{
    aiProcess_CalcTangentSpace |
    aiProcess_JoinIdenticalVertices |
    aiProcess_MakeLeftHanded |
    aiProcess_Triangulate |
    aiProcess_RemoveComponent |
    aiProcess_GenSmoothNormals |
    aiProcess_SplitLargeMeshes |
    aiProcess_LimitBoneWeights |
    aiProcess_ValidateDataStructure |
    aiProcess_ImproveCacheLocality |
    aiProcess_RemoveRedundantMaterials |
    aiProcess_SortByPType |
    aiProcess_FindDegenerates |
    aiProcess_FindInvalidData |
    aiProcess_GenUVCoords |
    aiProcess_FindInstances |
    aiProcess_OptimizeMeshes |
    aiProcess_OptimizeGraph |
    aiProcess_FlipUVs |
    aiProcess_FlipWindingOrder |
    aiProcess_GenBoundingBoxes
  };
  auto scene{ imp.ReadFile("../data/brick.stl", processFlags) };

  // Prevent Assimp from running the progress handler destructor.
  imp.SetProgressHandler(nullptr);

  if (!scene) {
    LOG(ImportMesh, Error, "Failed to import model");
    return "";
  }

  ASSERT(!(scene->mFlags & AI_SCENE_FLAGS_TERRAIN), "TODO: import terrain models");
  ASSERT(scene->mRootNode, "Expecting model root node");

  std::ostringstream out;

  // TODO textures
  // TODO materials

  // TODO skeleton
  // TODO animations

  MeshHeader header{ 0, 0 };
  ASSERT(scene->mNumMeshes == 1); // TODO

  for (auto n{ 0 }; n < scene->mNumMeshes; n++) {
    auto mesh{ scene->mMeshes[n] };

    ASSERT(mesh->mPrimitiveTypes == aiPrimitiveType_TRIANGLE);

    // TODO find matching vertex layout

    header.numIndices += mesh->mNumFaces * 3;
    header.numVertices += mesh->mNumVertices;

    out.write(reinterpret_cast<char const*>(&header), sizeof(header));

    for (auto n{ 0 }; n < mesh->mNumFaces; n++) {
      auto& face = mesh->mFaces[n];
      ASSERT(face.mNumIndices == 3);
      for (auto f{ 0 }; f < 3; f++) {
        auto index = static_cast<u16>(face.mIndices[f]);
        out.write(reinterpret_cast<char const*>(&index), 2);
      }
    }

    for (auto n{ 0 }; n < mesh->mNumVertices; n++) {
      auto& vert = mesh->mVertices[n];
    #if 1
      ASSERT(vert.x >= -1 && vert.x <= 1 &&
             vert.y >= -1 && vert.y <= 1 &&
             vert.z >= -1 && vert.z <= 1);
      //LOG(ImportMesh, Info, "%f %f %f", vert.x, vert.y, vert.z);
      i16 pack[3]{
        snorm<i16>(vert.x),
        snorm<i16>(vert.z),
        snorm<i16>(vert.y)
      };
      out.write(reinterpret_cast<char const*>(pack), 6);
    #else
      out.write(reinterpret_cast<char const*>(&vert), 12);
    #endif
    }
  }

  // TODO simplify -> LODs

  return out.str();
}


std::string testMeshImport() {
  return importMesh();
}

// MODEL file format
// -----------------

// Want to load model data directly into vertex/index buffers
//   map buffer -> fread to it -> unmap buffer

// HEADER
// u32 Signature ("JANK")
// u32 fileVersion
// u32 numDependencies
// str dependencies[numDependencies]
// u32 numAssets
// u64 assetOffsets[numAssets]

// MODEL HEADER
// u32 assetTypeId
// u32 numVertices
// u32 numIndices

// THINGS TO STORE
// - Indices
// - Vertices
// - Normals
// - Tangents
// - TexCoords[]
// - Colors[]
// - Material Refs
// - LODs
//   - Meshes
//     - Material Index
//     - Index offset & count
//     - Vertex base (start & end? only used by GL in glDrawRangeElements?)

