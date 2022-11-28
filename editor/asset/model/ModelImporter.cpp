#include "asset/model/ModelImporter.hpp"
#include "asset/texture/TextureImporter.hpp"
#include "asset/Library.hpp"

#include "core/CoreMath.hpp"

#include "anim/Animation.hpp"
#include "anim/Skeleton.hpp"
#include "render/Camera.hpp"
#include "render/Light.hpp"
#include "render/Material.hpp"
#include "render/Model.hpp"

#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/ProgressHandler.hpp>
#include <assimp/scene.h>

#include <iostream> // TODO use our own Archive

#include "app/GfxTest.hpp" // TODO remove

namespace Asset {

ASSET_FORMAT(Mesh, dae);
ASSET_FORMAT(Mesh, fbx);
ASSET_FORMAT(Mesh, gltf);
ASSET_FORMAT(Mesh, obj);
ASSET_FORMAT(Mesh, stl);

}


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
static constexpr auto snorm(f32 f) {
  auto c = std::clamp(f, -1.f, 1.f) * std::numeric_limits<T>::max();
  return static_cast<T>(c >= 0 ? c + .5f : c - .5f);
}

template<typename T>
static constexpr auto unorm(f32 f) {
  return static_cast<T>(std::clamp(f, 0.f, 1.f) * std::numeric_limits<T>::max() + .5f);
}

static_assert(unorm<u16>(0.f) == 0);
static_assert(unorm<u16>(1.f) == UINT16_MAX);

static_assert(snorm<i16>(0.f) == 0);
static_assert(snorm<i16>(1.f) == INT16_MAX);
static_assert(snorm<i16>(-1.f) == -INT16_MAX);

static std::string_view const viewStr(aiString const& str) {
  return { str.data, str.length };
}

static std::string_view const readStr(aiMaterialProperty const& prop) {
  ASSERT(prop.mType == aiPTI_String);
  return { prop.mData + 4, *reinterpret_cast<u32 const*>(prop.mData) };
}

template<typename T>
static T const* readBuf(aiMaterialProperty const& prop, usize size = sizeof(T)) {
  ASSERT(prop.mType == aiPTI_Buffer);
  ASSERT(prop.mDataLength == size);
  return reinterpret_cast<T const*>(prop.mData);
}

static f32 readF32(aiMaterialProperty const& prop) {
  ASSERT(prop.mType == aiPTI_Float);
  ASSERT(prop.mDataLength == sizeof(f32));
  return *reinterpret_cast<f32 const*>(prop.mData);
}

static i32 readI32(aiMaterialProperty const& prop) {
  ASSERT(prop.mType == aiPTI_Integer);
  ASSERT(prop.mDataLength == sizeof(i32));
  return *reinterpret_cast<i32 const*>(prop.mData);
}

union Color8 {
  u8 v[4]{ 0x00, 0x00, 0x00, 0xFF };
  struct {
    u8 r;
    u8 g;
    u8 b;
    u8 a;
  };
};

union Color {
  f32 v[4]{ 0, 0, 0, 1 };
  struct {
    f32 r;
    f32 g;
    f32 b;
    f32 a;
  };
};

static Color readColor(aiMaterialProperty const& prop) {
  ASSERT(prop.mType == aiPTI_Float);
  auto f = reinterpret_cast<f32 const*>(prop.mData);
  Color c;
  switch (prop.mDataLength) {
  default: ASSERT(0, "Unexpected assimp material color size"); UNREACHABLE;
  case sizeof(f32) * 4:
    c.a = f[3];
    FALLTHROUGH;
  case sizeof(f32) * 3:
    c.r = f[0];
    c.g = f[1];
    c.b = f[2];
    break;
  }
  return c;
}

static u8 countSkeletonBones(aiNode const& node) {
  u8 count{ 1 };
  for (u32 n{ 0 }; n < node.mNumChildren; n++) {
    count += countSkeletonBones(*node.mChildren[n]);
  }
  return count;
}

struct BoneLookup {
  rtm::matrix4x4f offset;
  std::string_view name;
  aiNode const* node;
  i16 parent;
  u8 index;
  bool used;
};

static i16 setupSkeletonLookup(aiNode const& node, BoneLookup* lookup, i16 parent = -1, i16 index = -1) {
  i16 offset{ 1 };
  i16 self{ index + offset };
  lookup[self] = {
    rtm::matrix_identity(),
    viewStr(node.mName),
    &node,
    parent,
    Skeleton::invalidBoneId,
    false
  };
  for (i16 n{ 0 }; n < node.mNumChildren; n++) {
    offset += setupSkeletonLookup(*node.mChildren[n], lookup, self, index + offset);
  }
  return offset;
}

static Animation::Behaviour getAnimationBehaviour(aiAnimBehaviour const behaviour) {
  return static_cast<Animation::Behaviour>(behaviour); // Using the same values for now.
}

static u32 getFlags(aiMesh const& mesh) {
  u32 flags = 0;
  // TODO mesh.HasFaces()
  if (mesh.HasPositions()) flags |= MeshHeader::HasPositions;
  if (mesh.HasTangentsAndBitangents()) flags |= MeshHeader::HasTangents;
  /*else*/ if (mesh.HasNormals()) flags |= MeshHeader::HasNormals;
  if (mesh.HasTextureCoords(0)) flags |= MeshHeader::HasTexCoords;
  if (mesh.HasBones()) flags |= MeshHeader::HasBones;
  return flags;
}

static TextureType getTextureType(aiTextureType tex) {
  switch (tex) {
  case aiTextureType_BASE_COLOR:
  case aiTextureType_DIFFUSE: return TextureType::Base;
  //case aiTextureType_SPECULAR: return TextureType::Specular;
  //case aiTextureType_EMISSIVE: return TextureType::Emissive;
  case aiTextureType_NORMALS: return TextureType::Normal;
  case aiTextureType_METALNESS: return TextureType::Metallic;
  case aiTextureType_DIFFUSE_ROUGHNESS: return TextureType::Roughness;
  case aiTextureType_AMBIENT_OCCLUSION: return TextureType::AO;
  default:
    LOG(ImportMesh, Warn, "Unknown texture type: %d", tex);
    return TextureType::Unknown;
  }
}

u32 importTexture(char const* filename, TextureType type);
u32 importTextureData(char const* filename, void const* data, usize size, TextureType type);

std::string dirName(char const* filename) {
#if PLATFORM_WINDOWS
  {
    auto p{ strrchr(filename, '\\') };
    if (p) return { filename, static_cast<usize>(p - filename) };
  }
#endif
  auto p{ strrchr(filename, '/') };
  return p ? std::string{ filename, static_cast<usize>((p + 1) - filename) } : std::string{ filename };
}

// Importer
// ----------------------------------------------------------------------------

static Asset::Type meshType{ "Mesh" };

Asset::Type const& Asset::MeshImporter::getType() const { return meshType; }

Asset::MeshImporter::MeshImporter() :
  processFlags(static_cast<aiPostProcessSteps>(aiProcess_CalcTangentSpace |
                                               aiProcess_JoinIdenticalVertices |
                                               aiProcess_Triangulate |
                                               aiProcess_RemoveComponent |
                                               //aiProcess_GenSmoothNormals |
                                               //aiProcess_SplitLargeMeshes |
                                               aiProcess_LimitBoneWeights |
                                               aiProcess_ValidateDataStructure |
                                               aiProcess_ImproveCacheLocality |
                                               aiProcess_RemoveRedundantMaterials |
                                               aiProcess_SortByPType |
                                               aiProcess_FindDegenerates |
                                               aiProcess_FindInvalidData |
                                               //aiProcess_GenUVCoords |
                                               aiProcess_FindInstances |
                                               aiProcess_OptimizeMeshes |
                                               aiProcess_OptimizeGraph |
                                               aiProcess_MakeLeftHanded |
                                               aiProcess_FlipUVs |
                                               aiProcess_FlipWindingOrder |
                                               aiProcess_GenBoundingBoxes)),
  removeComponents(aiComponent_COLORS)
{}

bool Asset::MeshImporter::process() {
  Assimp::Importer imp;

  ProgressHandler progressHandler;
  imp.SetProgressHandler(&progressHandler);
  imp.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, removeComponents);

  auto scene{ imp.ReadFile(toUtf8(getFilePath()), processFlags) };

  // RAII will destroy the progress handler, don't let assimp call it.
  imp.SetProgressHandler(nullptr);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    LOG(ImportMesh, Error, "Failed to import model: %s", imp.GetErrorString());
    return false;
  }

  ASSERT(!(scene->mFlags & AI_SCENE_FLAGS_TERRAIN), "TODO: import terrain models");

  // Cameras
  // --------------------------------------------------------------------------

  if (scene->mNumCameras != 0) {
    for (auto n{ 0u }; n < scene->mNumCameras; n++) {
      auto const& camera{ scene->mCameras[n] };

      Render::Camera object;

      // TODO add to model scene
    }
  }

  // Lights
  // --------------------------------------------------------------------------

  if (scene->mNumLights != 0) {
    for (auto n{ 0u }; n < scene->mNumLights; n++) {
      auto const& light{ scene->mLights[n] };

      Render::Light object;

      // TODO add to model scene
    }
  }

  // Materials
  // --------------------------------------------------------------------------

#define PROP(s) prop.mKey.length == sizeof(s) - 1 && memcmp(prop.mKey.data, s, sizeof(s) - 1) == 0

#define BP() // BREAKPOINT()

  if (scene->mNumMaterials != 0) {
    std::vector<TextureImporter> textures;
    std::unique_ptr<Render::Material[]> materials{ new Render::Material[scene->mNumMaterials] };

    struct TextureLookup {
      // addressModeU
      // addressModeV
      // filterMin
      // filterMag
    };
    struct MaterialLookup {
      StringView name;
      Color albedo{};
      Color emissive{};
      f32   metallic{};
      f32   roughness{};
      f32   opacity{ 1 };
      f32   transparency{ 0 };
      f32   displacementScaling{ 1 };
      bool wireframe : 1;
      bool twoSided  : 1;
    } lookup;

    for (auto n{ 0u }; n < scene->mNumMaterials; n++) {
      auto const& mat{ *scene->mMaterials[n] };

      for (auto i{ 0u }; i < mat.mNumProperties; i++) {
        auto const& prop{ *mat.mProperties[i] };

        if (PROP("?mat.name")) {
          lookup.name = readStr(prop);
        }
        else if (PROP("$tex.file")) {
          auto type{ getTextureType(static_cast<aiTextureType>(prop.mSemantic)) };
          auto name{ readStr(prop) };
          ASSERT(name.size());

          if (type == TextureType::Unknown) {
            LOG(ImportMesh, Error, "Unknown assimp texture semantic: 0x%x", prop.mSemantic);
            continue;
          }

          if (name[0] == '*') {
            ASSERT(0, "TODO embedded texture references");
            continue;
          }

          // scan embedded textures

          // load texture from file
        }
        else if (PROP("$tex.file.texCoord")) {
          // uvIndex
          readBuf<u32>(prop);
        }
        else if (PROP("$tex.mapmodeu")) {
          //getAddressMode(prop)
        }
        else if (PROP("$tex.mapmodev")) {
          //getAddressMode(prop)
        }
        else if (PROP("$tex.mappingfiltermag")) {
          // mag filter
        }
        else if (PROP("$tex.mappingfiltermin")) {
          // min filter
          // mip filter
        }
        else if (PROP("$tex.mappingid")) {
          BP();
          readStr(prop);
        }
        else if (PROP("$tex.mappingname")) {
          BP();
          readStr(prop);
        }
        else if (PROP("$tex.scale")) {
          BP();
          readF32(prop);
        }
        else if (PROP("$tex.strength")) {
          BP();
          readF32(prop);
        }
        else if (PROP("$tex.uvwsrc")) {
          BP();
          readI32(prop);
        }
        else if (PROP("$mat.shadingm")) {
          BP();
          switch (*readBuf<aiShadingMode>(prop)) {
          case aiShadingMode_NoShading:
            // TODO unlit
            break;
          case aiShadingMode_Flat:
          case aiShadingMode_Gouraud:
          case aiShadingMode_Phong:
          case aiShadingMode_Blinn:
            // TODO legacy
            break;
          case aiShadingMode_Toon:
            // TODO toon shading
            break;
          case aiShadingMode_OrenNayar:
          case aiShadingMode_Minnaert:
          case aiShadingMode_CookTorrance:
          case aiShadingMode_Fresnel:
            // TODO
            break;
          case _aiShadingMode_Force32Bit:
          default:
            break;
          }
        }
        else if (PROP("$mat.wireframe")) {
          lookup.wireframe = *readBuf<bool>(prop);
        }
        else if (PROP("$mat.twosided")) {
          lookup.twoSided = *readBuf<bool>(prop);
        }
        else if (PROP("$mat.blend")) {
          BP();
        }
        else if (PROP("$mat.opacity")) {
          lookup.opacity = readF32(prop);
        }
        else if (PROP("$mat.transparencyfactor")) {
          lookup.transparency = readF32(prop);
        }
        else if (PROP("$mat.bumpscaling")) {
          BP();
          readF32(prop);
        }
        else if (PROP("$mat.shinpercent")) {
          BP();
          readF32(prop);
        }
        else if (PROP("$mat.refractivity")) {
          BP();
        }
        else if (PROP("$mat.displacementscaling")) {
          lookup.displacementScaling = readF32(prop);
        }
        else if (PROP("$mat.gltf.pbrMetallicRoughness.baseColorFactor")) {
          BP();
        }
        else if (PROP("$mat.gltf.pbrMetallicRoughness.metallicFactor")) {
          lookup.metallic = readF32(prop);
        }
        else if (PROP("$mat.gltf.pbrMetallicRoughness.roughnessFactor")) {
          lookup.roughness = readF32(prop);
        }
        else if (PROP("$mat.gltf.pbrMetallicRoughness.glossinessFactor")) {
          BP();
        }
        else if (PROP("$mat.gltf.pbrSpecularGlossiness")) {
          BP();
        }
        else if (PROP("$mat.gltf.alphaMode")) {
          BP();
          readStr(prop);
        }
        else if (PROP("$mat.gltf.alphaCutoff")) {
          BP();
          readF32(prop);
        }
        else if (PROP("$mat.gltf.unlit")) {
          BP();
        }
        else if (PROP("$clr.diffuse")) {
          lookup.albedo = readColor(prop);
        }
        else if (PROP("$clr.specular")) {
          BP();
          //lookup.specular = readColor(prop);
        }
        else if (PROP("$clr.ambient")) {
          BP();
          readColor(prop);
        }
        else if (PROP("$clr.emissive")) {
          lookup.emissive = readColor(prop);
        }
        else if (PROP("$clr.transparent")) {
          BP();
          readColor(prop);
        }
        else if (PROP("$clr.reflective")) {
          BP();
          readColor(prop);
        }
        else if (PROP("?bg.global")) {
          BP();
        }
        else if (PROP("?sh.lang")) {
          BP();
        }
        else if (PROP("?sh.vs")) {
          BP();
        }
        else if (PROP("?sh.fs")) {
          BP();
        }
        else if (PROP("?sh.gs")) {
          BP();
        }
        else if (PROP("?sh.ts")) {
          BP();
        }
        else if (PROP("?sh.ps")) {
          BP();
        }
        else if (PROP("?sh.cs")) {
          BP();
        }
        //"$raw.Emissive"
        //"$raw.Reflectivity"
        //"$raw.Shininess"
        //"$raw.Ambient"
        //"$raw.Diffuse"
        //"$raw.Specular"
        //"$raw.EmissiveColor|file"
        //"$raw.DiffuseColor|file"
        //"$raw.DiffuseColor|uvtrafo"
        //"$raw.DiffuseColor|uvwsrc"
        //"$raw.NormalMap|file"
        //"$raw.SpecularColor|file"
        else {
          LOG(ImportMesh, Error,
              "Unknown assimp material property: %.*s (type=0x%x, size=0x%x)",
              USTR_ARGS(viewStr(prop.mKey)), prop.mType, prop.mDataLength);
        }
      }

      // TODO find/create matching material source
    }
  }

  // Meshes
  // --------------------------------------------------------------------------

  u32 vertexFlags;
  auto numSubMeshes{ 0u };
  auto totalIndices{ 0u };
  auto totalVertices{ 0u };
  auto meshBoneLookupSize{ 0u };

  for (auto n{ 0u }; n < scene->mNumMeshes; n++) {
    auto const& mesh{ *scene->mMeshes[n] };
    if (mesh.mPrimitiveTypes != aiPrimitiveType_TRIANGLE) {
      LOG(ImportMesh, Warn, "Ignoring non-triangular mesh: %.*s",
          USTR_ARGS(viewStr(mesh.mName)));
      continue;
    }

    if (n == 0) {
      vertexFlags = getFlags(mesh);
    }
    else if (vertexFlags != getFlags(mesh)) {
      LOG(ImportMesh, Warn, "Ignoring mesh with different vertex layout: %.*s",
          USTR_ARGS(viewStr(mesh.mName)));
      continue;
    }

    numSubMeshes++;
    totalIndices       += mesh.mNumFaces * 3;
    totalVertices      += mesh.mNumVertices;
    meshBoneLookupSize += mesh.mNumBones;
  }

#define MESH_FILTER()                                             \
  auto const& mesh{ *scene->mMeshes[n] };                         \
  if (mesh.mPrimitiveTypes != aiPrimitiveType_TRIANGLE) continue; \
  if (vertexFlags != getFlags(mesh)) continue;

  // Skeleton
  // --------------------------------------------------------------------------

  std::unique_ptr<f32[][4]> boneWeights;
  std::unique_ptr<u8 [][4]> boneIndices;

  if (vertexFlags & MeshHeader::HasBones) {
    boneWeights = decltype(boneWeights){ new f32[totalVertices][4] };
    boneIndices = decltype(boneIndices){ new u8 [totalVertices][4] };

    std::unique_ptr<i16[]> meshBoneLookup{ new i16[meshBoneLookupSize] };

    auto const boneLookupSize{ countSkeletonBones(*scene->mRootNode) };

    std::unique_ptr<BoneLookup[]> boneLookup{ new BoneLookup[boneLookupSize] };
    setupSkeletonLookup(*scene->mRootNode, boneLookup.get());

    for (auto n{ 0u }; n < totalVertices; n++) {
      for (auto i{ 0u }; i < 4u; i++) {
        boneWeights[n][i] = .0f;
      }
    }

    memset(boneIndices.get(), 0, totalVertices * 4);

    // Build the bone lookup.
    auto boneIndex{ 0u };
    for (auto n{ 0u }; n < scene->mNumMeshes; n++) {
      MESH_FILTER();

      for (auto i{ 0u }; i < mesh.mNumBones; i++) {
        auto const& bone{ *mesh.mBones[i] };
        auto const name{ viewStr(bone.mName) };

        i16 j{ 0 };
        for (; j < boneLookupSize; j++) {
          if (name == boneLookup[j].name) {
            meshBoneLookup[boneIndex] = j;
            boneLookup[j].used = true;
            boneLookup[j].offset = rtm::matrix_transpose(
              *reinterpret_cast<rtm::matrix4x4f const*>(&bone.mOffsetMatrix));
            break;
          }
        }
        ASSERT(j != boneLookupSize);
        boneIndex++;
      }
    }

    // Index the bone lookup.
    boneIndex = 0u;
    for (auto n{ 0u }; n < boneLookupSize; n++) {
      auto& bone{ boneLookup[n] };
      if (bone.used) {
        bone.index = boneIndex++;
      }
    }
    ASSERT(boneIndex < 0xFF);

    // Allocate skeleton data.
    auto numBones{ boneIndex };
    std::unique_ptr<Symbol[]> boneNames     { new Symbol[numBones] };
    std::unique_ptr<Mat4[]>   boneOffsets   { new Mat4  [numBones] };
    std::unique_ptr<Mat4[]>   boneTransforms{ new Mat4  [numBones] };
    std::unique_ptr<u8[]>     boneParentIds { new u8    [numBones] };

    // Build the skeleton & joints.
    boneIndex = 0u;
    for (auto n{ 0u }; n < boneLookupSize; n++) {
      auto const& bone{ boneLookup[n] };
      if (!bone.used) {
        LOG(ImportMesh, Warn, "Unused bone: %.*s", USTR_ARGS(bone.name));
        continue;
      }

      boneNames     [boneIndex]    = bone.name;
      boneOffsets   [boneIndex].m4 = bone.offset;
      boneTransforms[boneIndex].m4 = rtm::matrix_transpose(
        *reinterpret_cast<rtm::matrix4x4f const*>(&bone.node->mTransformation));

      auto parentId{ bone.parent };
      if (parentId == -1) {
        boneParentIds[boneIndex] = Anim::Skeleton::invalidBoneId;
      }
      else {
        boneParentIds[boneIndex] = boneLookup[parentId].index;

        do {
          auto const& parent{ boneLookup[parentId] };
          if (parent.used) break;

          boneTransforms[boneIndex].m4 =
            rtm::matrix_mul(rtm::matrix_transpose(
              *reinterpret_cast<rtm::matrix4x4f const*>(&parent.node->mTransformation)),
              boneTransforms[boneIndex].m4);

          parentId = parent.parent;
        } while (parentId != -1);
      }

      boneIndex++;
    }
    ASSERT(boneIndex == numBones);

    // Create the skeleton asset
    Anim::Skeleton skeleton;
    skeleton.setJoints(std::move(boneNames),
                       std::move(boneOffsets),
                       std::move(boneTransforms),
                       std::move(boneParentIds),
                       numBones);

    Library::save(USTR("test.skeleton"sv), skeleton);

    // Build the vertex data.
    boneIndex = 0u;
    auto vertexStart{ 0u };
    for (auto n{ 0u }; n < scene->mNumMeshes; n++) {
      MESH_FILTER();

      for (auto i{ 0u }; i < mesh.mNumBones; i++) {
        auto const& bone{ *mesh.mBones[i] };
        auto const& data{ boneLookup[meshBoneLookup[boneIndex++]] };
        auto const index{ data.index };
        ASSERT(index != -1);

        for (auto j{ 0u }; j < bone.mNumWeights; j++) {
          auto const weight{ bone.mWeights[j] };
          auto const vertex{ vertexStart + weight.mVertexId };
          ASSERT(vertex < totalVertices);

          auto k{ 0u };
          for (; k < 4; k++) {
            if (boneWeights[vertex][k] == 0.f) {
              boneIndices[vertex][k] = index;
              boneWeights[vertex][k] = weight.mWeight;
              break;
            }
          }
          ASSERT(k != 4u);
        }
      }

      vertexStart += mesh.mNumVertices;
    }
  }

  // Animations
  // --------------------------------------------------------------------------

  if (scene->mNumAnimations != 0) {
    u8 numLayers{ 0 };
    u32 totalRotationKeys{ 0u };
    u32 totalPositionKeys{ 0u };
    u32 totalScaleKeys   { 0u };

    for (auto n{ 0u }; n < scene->mNumAnimations; n++) {
      auto const& anim{ *scene->mAnimations[n] };
      auto const animName{ viewStr(anim.mName) };

      for (auto i{ 0u }; i < anim.mNumChannels; i++) {
        auto const& channel{ *anim.mChannels[i] };
        auto const channelName{ viewStr(channel.mNodeName) };

        auto j{ 0u };
        //for (; j < numBones; j++) {
          // TODO find corresponding bone
        //}

        //if (j == numBones) {
          LOG(ImportMesh, Warn, "No bone matching animation channel %.*/%.s",
              USTR_ARGS(animName), USTR_ARGS(channelName));
          continue;
        //}

        numLayers++;
        totalRotationKeys += channel.mNumRotationKeys;
        totalPositionKeys += channel.mNumPositionKeys;
        totalScaleKeys    += channel.mNumScalingKeys;
      }
    }

    std::unique_ptr<f32[]> rotationKeys{ new f32[totalRotationKeys] };
    std::unique_ptr<f32[]> positionKeys{ new f32[totalPositionKeys] };
    std::unique_ptr<f32[]> scaleKeys   { new f32[totalScaleKeys]    };

    std::unique_ptr<Quat[]> rotations{ new Quat[totalRotationKeys] };
    std::unique_ptr<Vec4[]> positions{ new Vec4[totalPositionKeys] };
    std::unique_ptr<Vec4[]> scales   { new Vec4[totalScaleKeys]    };

    for (auto n{ 0u }; n < scene->mNumAnimations; n++) {
      auto const& anim{ *scene->mAnimations[n] };

      for (auto i{ 0u }; n < anim.mNumChannels; n++) {
        auto const& channel{ *anim.mChannels[i] };

        for (auto k{ 0u }; k < channel.mNumRotationKeys; k++) {

        }

        for (auto k{ 0u }; k < channel.mNumPositionKeys; k++) {

        }

        for (auto k{ 0u }; k < channel.mNumScalingKeys; k++) {

        }
      }
    }
  }

  // Vertices
  // --------------------------------------------------------------------------

  // - Indices
  // 0 Positions
  // 1 Normals
  // 2 Tangents
  // 3 Bone Weights & Indices
  // 4 Texture Coordinates
  // 8 Colors

  return true;
}

// TODO old
// ----------------------------------------------------------------------------

#include <unordered_map>
std::string testMeshImport(char const* filename, Material** materials, u32* numMaterials,
                           Skeleton* skeleton, Animation* animation)
{
  // TODO thread-local importer, recycle its allocations
  Assimp::Importer imp;
  ProgressHandler progressHandler;
  imp.SetProgressHandler(&progressHandler);
  //imp.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, removeComponents);

  auto processFlags{
    aiProcess_CalcTangentSpace |
    aiProcess_JoinIdenticalVertices |
    aiProcess_Triangulate |
    //aiProcess_RemoveComponent |
    //aiProcess_GenSmoothNormals |
    //aiProcess_SplitLargeMeshes |
    aiProcess_LimitBoneWeights |
    aiProcess_ValidateDataStructure |
    aiProcess_ImproveCacheLocality |
    aiProcess_RemoveRedundantMaterials |
    aiProcess_SortByPType |
    aiProcess_FindDegenerates |
    aiProcess_FindInvalidData |
    //aiProcess_GenUVCoords |
    aiProcess_FindInstances |
    aiProcess_OptimizeMeshes |
    aiProcess_OptimizeGraph |
    aiProcess_MakeLeftHanded |
    aiProcess_FlipUVs |
    aiProcess_FlipWindingOrder |
    aiProcess_GenBoundingBoxes
  };
  //auto scene{ imp.ReadFile("../data/Alduin/Alduin.obj", processFlags) };
  //auto scene{ imp.ReadFile("../data/Harley_Quinn/Harley_Quinn.obj", processFlags) };
  //auto scene{ imp.ReadFile("../data/Fat Man/FatMan.obj", processFlags) };
  //auto scene{ imp.ReadFile("../data/Flamethrower/Flamethrower.obj", processFlags) };
  //auto scene{ imp.ReadFile("../data/Eyebot/Eyebot.obj", processFlags) };
  //auto scene{ imp.ReadFile("../data/Main Outfit/Ellie_MainOutfit.obj", processFlags) };

  auto scene{ imp.ReadFile(filename, processFlags) };

  // Prevent Assimp from running the progress handler destructor.
  imp.SetProgressHandler(nullptr);

  if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
    LOG(ImportMesh, Error, "Failed to import model: %s", imp.GetErrorString());
    return "";
  }

  ASSERT(!(scene->mFlags & AI_SCENE_FLAGS_TERRAIN), "TODO: import terrain models");

  std::ostringstream out;

  struct TexCacheItem {
    std::string owner;
    u32 id;
  };
  std::unordered_map<std::string_view, TexCacheItem> texCache;

  for (auto n{ 0 }; n < scene->mNumTextures; n++) {
    auto const& tex{ *scene->mTextures[n] };
    ASSERT(tex.mHeight == 0, "TODO: assimp embedded uncompressed textures");
    //ASSERT(texCache.find())

    LOG(ImportMesh, Info, "Embedded texture: %s", tex.mFilename.C_Str());
#if 0
    importTextureData(tex.pcData, tex.mWidth, false);
#endif
  }

  if (!numMaterials || !materials) goto skipMaterial;

  *numMaterials = scene->mNumMaterials;
  *materials = new Material[scene->mNumMaterials];

#define PROP(s) prop.mKey.length == sizeof(s) - 1 && memcmp(prop.mKey.data, s, sizeof(s) - 1) == 0
  for (auto n{ 0 }; n < scene->mNumMaterials; n++) {
    for (auto i{ 0 }; i < textureTypeCount; i++) {
      (*materials)[n].textures[i] = 0;
    }

    auto const& mat{ *scene->mMaterials[n] };
    for (auto i{ 0 }; i < mat.mNumProperties; i++) {
      auto const& prop{ *mat.mProperties[i] };
      if (PROP("?mat.name")) {
        //name = readStr(prop);
      }
      else if (PROP("$tex.file")) {
        auto semantic{ static_cast<aiTextureType>(prop.mSemantic) };
        auto name{ readStr(prop) };
        ASSERT(name.size() > 0);
        auto textureType{ getTextureType(semantic) };
        if (textureType != TextureType::Unknown) {
          u32 id;
          if (name[0] == '*') {
            ASSERT(0, "TODO assimp embedded texture references");
          }
          else {
            bool found = false;
            for (auto texN{ 0 }; texN < scene->mNumTextures; texN++) {
              auto const& tex{ *scene->mTextures[texN] };
              auto const texName{ viewStr(tex.mFilename) };
              if (texName == name) {
                if (auto it = texCache.find(texName); it != texCache.end()) {
                  id = it->second.id;
                }
                else {
                  ASSERT(tex.mHeight == 0, "TODO embedded uncompressed textures");
                  id = importTextureData(tex.mFilename.C_Str(), tex.pcData, tex.mWidth, textureType);
                  texCache.insert(std::make_pair(texName, TexCacheItem{ std::string{ texName }, id }));
                }
                found = true;
                break;
              }
            }

            if (!found) {
              std::string texFile{ dirName(filename) };
              texFile += name;
              if (auto it = texCache.find(texFile); it != texCache.end()) {
                id = it->second.id;
              }
              else {
                LOG(ImportMesh, Info, "TEX %.*s", static_cast<u32>(name.size()), name.data());
                id = importTexture(texFile.c_str(), textureType);
                texCache.insert(std::make_pair(texFile, TexCacheItem{ texFile, id }));
              }
            }
          }

          (*materials)[n].textures[static_cast<u32>(textureType)] = id;
        }
      }
      else if (PROP("$tex.file.texCoord")) {
      }
      else if (PROP("$tex.mapmodeu")) {
      }
      else if (PROP("$tex.mapmodev")) {
      }
      else if (PROP("$tex.mappingfiltermag")) {
      }
      else if (PROP("$tex.mappingfiltermin")) {
      }
      else if (PROP("$tex.mappingid")) {
      }
      else if (PROP("$tex.mappingname")) {
      }
      else if (PROP("$tex.scale")) {
      }
      else if (PROP("$tex.strength")) {
      }
      else if (PROP("$tex.uvwsrc")) {
      }
      else if (PROP("$mat.twosided")) {
      }
      else if (PROP("$mat.shadingm")) {
      }
      else if (PROP("$mat.wireframe")) {
      }
      else if (PROP("$mat.blend")) {
      }
      else if (PROP("$mat.opacity")) {
      }
      else if (PROP("$mat.transparencyfactor")) {
      }
      else if (PROP("$mat.bumpscaling")) {
      }
      else if (PROP("$mat.shininess")) {
      }
      else if (PROP("$mat.reflectivity")) {
      }
      else if (PROP("$mat.shinpercent")) {
      }
      else if (PROP("$mat.refracti")) {
      }
      else if (PROP("$mat.displacementscaling")) {
      }
      else if (PROP("$mat.gltf.pbrMetallicRoughness.baseColorFactor")) {
      }
      else if (PROP("$mat.gltf.pbrMetallicRoughness.metallicFactor")) {
      }
      else if (PROP("$mat.gltf.pbrMetallicRoughness.roughnessFactor")) {
      }
      else if (PROP("$mat.gltf.pbrMetallicRoughness.glossinessFactor")) {
      }
      else if (PROP("$mat.gltf.pbrSpecularGlossiness")) {
      }
      else if (PROP("$mat.gltf.alphaMode")) {
      }
      else if (PROP("$mat.gltf.alphaCutoff")) {
      }
      else if (PROP("$mat.gltf.unlit")) {
      }
      else if (PROP("$clr.diffuse")) {

      }
      else if (PROP("$clr.specular")) {

      }
      else if (PROP("$clr.ambient")) {

      }
      else if (PROP("$clr.emissive")) {

      }
      else if (PROP("$clr.transparent")) {

      }
      else if (PROP("$clr.reflective")) {

      }
      else if (PROP("$?bg.global")) {
      }
      else if (PROP("$?sh.lang")) {
      }
      else if (PROP("$?sh.vs")) {
      }
      else if (PROP("$?sh.fs")) {
      }
      else if (PROP("$?sh.gs")) {
      }
      else if (PROP("$?sh.ts")) {
      }
      else if (PROP("$?sh.ps")) {
      }
      else if (PROP("$?sh.cs")) {
      }
      else {
        LOG(ImportMesh, Warn, "Unknown assimp material property: %s (type=$%x, size=$%x)",
            prop.mKey.C_Str(), prop.mType, prop.mDataLength);
      }
    }
  }
#undef PROP

  skipMaterial:

  auto meshBoneLookupSize{ 0 };
  auto const boneLookupSize{ countSkeletonBones(*scene->mRootNode) };
  std::unique_ptr<BoneLookup[]> boneLookup{ new BoneLookup[boneLookupSize] };
  setupSkeletonLookup(*scene->mRootNode, boneLookup.get());

  // Sum up mesh properties.
  MeshHeader header{};
  for (auto n{ 0 }; n < scene->mNumMeshes; n++) {
    auto const& mesh{ *scene->mMeshes[n] };
    if (mesh.mPrimitiveTypes != aiPrimitiveType_TRIANGLE) continue; // TODO

    if (n == 0) header.flags = getFlags(mesh);
    else if (header.flags != getFlags(mesh)) continue;// ASSERT(header.flags == getFlags(mesh));

    meshBoneLookupSize += mesh.mNumBones;
    header.numIndices  += mesh.mNumFaces * 3;
    header.numVertices += mesh.mNumVertices;
    header.subMeshCount++;
  }

  // Allocate bones data.
  std::unique_ptr<i16[]> meshBoneLookup{ new i16[meshBoneLookupSize] };
  std::unique_ptr<f32[][4]> boneWeights{ new f32[header.numVertices][4] };
  std::unique_ptr<u8[][4]> boneIndices{ new u8[header.numVertices][4] };

  for (auto n{ 0 }; n < header.numVertices; n++) {
    for (auto i{ 0 }; i < 4; i++) {
      boneWeights[n][i] = 0;
    }
  }

  memset(boneIndices.get(), 0, header.numVertices * 4);

  // Write the file header.
  out.write(reinterpret_cast<char const*>(&header), sizeof(header));

  // Write the submeshes.
  u32 indexOffset{ 0 };
  u32 vertexStart{ 0 };
  for (auto n{ 0 }; n < scene->mNumMeshes; n++) {
    auto const& mesh{ *scene->mMeshes[n] };
    if (mesh.mPrimitiveTypes != aiPrimitiveType_TRIANGLE) continue; // TODO
    if (header.flags != getFlags(mesh)) continue;

    MeshHeader::SubMesh sm;
    sm.materialIndex = mesh.mMaterialIndex;
    sm.unused0 = 0;
    sm.unused1 = 0;
    sm.indexCount = mesh.mNumFaces * 3;
    sm.indexOffset = indexOffset;
    sm.vertexStart = vertexStart;
    indexOffset += sm.indexCount * 4;
    vertexStart += mesh.mNumVertices;
    out.write(reinterpret_cast<char const*>(&sm), sizeof(sm));
  }

  // Build the bone lookup.
  u32 boneIndex{ 0 };
  if (!skeleton) goto skipSkeleton;
  for (auto n{ 0 }; n < scene->mNumMeshes; n++) {
    auto const& mesh{ *scene->mMeshes[n] };
    if (mesh.mPrimitiveTypes != aiPrimitiveType_TRIANGLE) continue; // TODO
    if (header.flags != getFlags(mesh)) continue;

    for (auto i{ 0 }; i < mesh.mNumBones; i++) {
      auto const& bone{ *mesh.mBones[i] };
      auto const name{ viewStr(bone.mName) };

      i16 j{ 0 };
      for (; j < boneLookupSize; j++) {
        if (name == boneLookup[j].name) {
          meshBoneLookup[boneIndex] = j;
          boneLookup[j].used = true;
          boneLookup[j].offset = rtm::matrix_transpose(
            *reinterpret_cast<rtm::matrix4x4f const*>(&bone.mOffsetMatrix));
          break;
        }
      }
      ASSERT(j != boneLookupSize);
      boneIndex++;
    }
  }

  // Index the bone lookup.
  boneIndex = 0;
  for (auto n{ 0 }; n < boneLookupSize; n++) {
    auto& bone{ boneLookup[n] };
    if (bone.used) {
      bone.index = boneIndex++;
    }
  }
  ASSERT(boneIndex <= 0xFF);

  // Allocate the skeleton.
  skeleton->boneNames = new std::string[boneIndex];
  skeleton->boneOffsets = new rtm::matrix4x4f[boneIndex];
  skeleton->boneTransforms = new rtm::matrix4x4f[boneIndex];
  skeleton->boneParentIds = new u8[boneIndex];
  skeleton->numBones = boneIndex;

  // Build the skeleton & joints.
  boneIndex = 0;
  for (auto n{ 0 }; n < boneLookupSize; n++) {
    auto const& bone{ boneLookup[n] };
    if (!bone.used) {
      LOG(ImportMesh, Warn, "Unused node: %s", bone.node->mName.C_Str());
      continue;
    }

    skeleton->boneNames[boneIndex] = std::string{ bone.name };
    LOG(ImportMesh, Info, "Bone %s[%d]",
        skeleton->boneNames[boneIndex].c_str(),
        boneIndex);

    skeleton->boneOffsets[boneIndex] = bone.offset;
    skeleton->boneTransforms[boneIndex] = rtm::matrix_transpose(
      *reinterpret_cast<rtm::matrix4x4f const*>(&bone.node->mTransformation));

    auto parentId = bone.parent;
    if (parentId == -1) {
      skeleton->boneParentIds[boneIndex] = Skeleton::invalidBoneId;
    }
    else {
      skeleton->boneParentIds[boneIndex] = boneLookup[parentId].index;

      do {
        auto const& parent{ boneLookup[parentId] };
        if (parent.used) break;
        skeleton->boneTransforms[boneIndex] = rtm::matrix_mul(
          rtm::matrix_transpose(
            *reinterpret_cast<rtm::matrix4x4f const*>(&parent.node->mTransformation)),
          skeleton->boneTransforms[boneIndex]);
        parentId = parent.parent;
      } while (parentId != -1);
    }

    boneIndex++;
  }
  ASSERT(boneIndex == skeleton->numBones);

  // Build the vertex data for joints.
  boneIndex = 0;
  vertexStart = 0;
  for (auto n{ 0 }; n < scene->mNumMeshes; n++) {
    auto const& mesh{ *scene->mMeshes[n] };
    if (mesh.mPrimitiveTypes != aiPrimitiveType_TRIANGLE) continue; // TODO
    if (header.flags != getFlags(mesh)) continue;

    for (auto i{ 0 }; i < mesh.mNumBones; i++) {
      auto const& bone{ *mesh.mBones[i] };
      auto const& lookup{ boneLookup[meshBoneLookup[boneIndex++]] };
      auto const index{ lookup.index };
      ASSERT(index != -1);

      for (auto j{ 0 }; j < bone.mNumWeights; j++) {
        auto const weight{ bone.mWeights[j] };
        auto const vertexId{ vertexStart + weight.mVertexId };
        ASSERT(vertexId < header.numVertices);

        auto k{ 0 };
        for (; k < 4; k++) {
          if (boneWeights[vertexId][k] == 0.f) {
            boneIndices[vertexId][k] = index;
            boneWeights[vertexId][k] = weight.mWeight;
            break;
          }
        }
        ASSERT(k != 4);
      }
    }

    vertexStart += mesh.mNumVertices;
  }

  skipSkeleton:

  // Write the indices.
  for (auto n{ 0 }; n < scene->mNumMeshes; n++) {
    auto const& mesh{ *scene->mMeshes[n] };
    if (mesh.mPrimitiveTypes != aiPrimitiveType_TRIANGLE) continue; // TODO
    if (header.flags != getFlags(mesh)) continue;

    for (auto i{ 0 }; i < mesh.mNumFaces; i++) {
      auto const& face = mesh.mFaces[i];
      ASSERT(face.mNumIndices == 3);
      out.write(reinterpret_cast<char const*>(face.mIndices), 12);
    }
  }

  // Write the vertex positions.
  if (header.flags & MeshHeader::HasPositions) {
    for (auto n{ 0 }; n < scene->mNumMeshes; n++) {
      auto const& mesh{ *scene->mMeshes[n] };
      if (mesh.mPrimitiveTypes != aiPrimitiveType_TRIANGLE) continue; // TODO
      if (header.flags != getFlags(mesh)) continue;

      out.write(reinterpret_cast<char const*>(mesh.mVertices), 12 * mesh.mNumVertices);
    }
  }

  // Write the vertex normals.
  if (header.flags & MeshHeader::HasNormals) {
    for (auto n{ 0 }; n < scene->mNumMeshes; n++) {
      auto const& mesh{ *scene->mMeshes[n] };
      if (mesh.mPrimitiveTypes != aiPrimitiveType_TRIANGLE) continue; // TODO
      if (header.flags != getFlags(mesh)) continue;

      out.write(reinterpret_cast<char const*>(mesh.mNormals), 12 * mesh.mNumVertices);
    }
  }

  // Write the vertex tangents.
  if (header.flags & MeshHeader::HasTangents) {
    for (auto n{ 0 }; n < scene->mNumMeshes; n++) {
      auto const& mesh{ *scene->mMeshes[n] };
      if (mesh.mPrimitiveTypes != aiPrimitiveType_TRIANGLE) continue; // TODO
      if (header.flags != getFlags(mesh)) continue;

      out.write(reinterpret_cast<char const*>(mesh.mTangents), 12 * mesh.mNumVertices);
    }
  #if 0
    for (auto n{ 0 }; n < scene->mNumMeshes; n++) {
      auto const& mesh{ *scene->mMeshes[n] };
      if (mesh.mPrimitiveTypes != aiPrimitiveType_TRIANGLE) continue; // TODO
      if (header.flags != getFlags(mesh)) continue;

      out.write(reinterpret_cast<char const*>(mesh.mBitangents), 12 * mesh.mNumVertices);
    }
  #endif
  }

  // Write the texture coordinates.
  if (header.flags & MeshHeader::HasTexCoords) {
    for (auto n{ 0 }; n < scene->mNumMeshes; n++) {
      auto const& mesh{ *scene->mMeshes[n] };
      if (mesh.mPrimitiveTypes != aiPrimitiveType_TRIANGLE) continue; // TODO
      if (header.flags != getFlags(mesh)) continue;

      auto const uvs = mesh.mTextureCoords[0];
      for (auto i{ 0 }; i < mesh.mNumVertices; i++) {
        auto const uv = &uvs[i];
        out.write(reinterpret_cast<char const*>(uv), 8);
      }
    }
  }

  // Write the bones.
  if (header.flags & MeshHeader::HasBones) {
    out.write(reinterpret_cast<char const*>(boneWeights.get()), 16 * header.numVertices);
    out.write(reinterpret_cast<char const*>(boneIndices.get()), 4 * header.numVertices);
  }

  // TODO simplify -> LODs

  // Setup animations.
  if (!animation) return out.str();

  u8 numLayers{ 0 }; // TODO retargetting
  u32 totalRotationFrames{ 0 };
  u32 totalPositionFrames{ 0 };
  u32 totalScaleFrames   { 0 };
  for (auto n{ 0 }; n < scene->mNumAnimations; n++) {
    auto const& anim{ *scene->mAnimations[n] };

    for (auto i{ 0 }; i < anim.mNumChannels; i++) {
      auto const& channel{ *anim.mChannels[i] };
      u8 j{ 0 };
      for (; j < skeleton->numBones; j++) {
        if (viewStr(channel.mNodeName) == skeleton->boneNames[j]) {
          numLayers++;
          totalRotationFrames += channel.mNumRotationKeys;
          totalPositionFrames += channel.mNumPositionKeys;
          totalScaleFrames    += channel.mNumScalingKeys;
          break;
        }
      }
      if (j == skeleton->numBones) {
        LOG(ImportMesh, Warn, "Animation layer has no corresponding bone: %s", channel.mNodeName.C_Str());
      }
    }
    break;
  }

  auto rotationKeys{ new f32[totalRotationFrames] };
  auto positionKeys{ new f32[totalPositionFrames] };
  auto scaleKeys   { new f32[totalScaleFrames] };

  auto rotations{ new rtm::quatf   [totalRotationFrames] };
  auto positions{ new rtm::vector4f[totalPositionFrames] };
  auto scales   { new rtm::vector4f[totalScaleFrames] };

  animation->numLayers = numLayers;
  animation->layers = new Animation::Layer[numLayers];

  numLayers = 0;
  totalRotationFrames = 0;
  totalPositionFrames = 0;
  totalScaleFrames    = 0;

  for (auto n{ 0 }; n < scene->mNumAnimations; n++) {
    auto const& anim{ *scene->mAnimations[n] };

    animation->ticksPerSecond = anim.mTicksPerSecond > 0 ? anim.mTicksPerSecond : 25;
    animation->duration = anim.mDuration;

    for (auto i{ 0 }; i < anim.mNumChannels; i++) {
      auto const& channel{ *anim.mChannels[i] };

      u8 j{ 0 };
      for (; j < skeleton->numBones; j++) {
        if (viewStr(channel.mNodeName) == skeleton->boneNames[j]) {
          break;
        }
      }
      if (j == skeleton->numBones) continue;

      LOG(ImportMesh, Info, "Anim %s.%s[%d] on bone [%s]%d",
          anim.mName.C_Str(), channel.mNodeName.C_Str(), numLayers,
          skeleton->boneNames[j].c_str(), j);

      auto& layer{ animation->layers[numLayers++] };
      layer.boneIndex = j;
      layer.statePre  = getAnimationBehaviour(channel.mPreState);
      layer.statePost = getAnimationBehaviour(channel.mPostState);

      layer.numRotationFrames = channel.mNumRotationKeys;
      layer.numPositionFrames = channel.mNumPositionKeys;
      layer.numScaleFrames    = channel.mNumScalingKeys;

      layer.rotationKeys = &rotationKeys[totalRotationFrames];
      layer.positionKeys = &positionKeys[totalPositionFrames];
      layer.scaleKeys    = &scaleKeys   [totalScaleFrames];

      layer.rotations = &rotations[totalRotationFrames];
      layer.positions = &positions[totalPositionFrames];
      layer.scales    = &scales   [totalScaleFrames];

      totalRotationFrames += channel.mNumRotationKeys;
      totalPositionFrames += channel.mNumPositionKeys;
      totalScaleFrames    += channel.mNumScalingKeys;

      for (auto k{ 0 }; k < layer.numRotationFrames; k++) {
        auto const key{ channel.mRotationKeys[k] };
        layer.rotations[k] = rtm::vector_to_quat(rtm::vector_set(key.mValue.x, key.mValue.y, key.mValue.z, key.mValue.w));
        layer.rotationKeys[k] = static_cast<f32>(channel.mRotationKeys[k].mTime);
      }

      for (auto k{ 0 }; k < layer.numPositionFrames; k++) {
        auto const key{ channel.mPositionKeys[k] };
        layer.positions[k] = rtm::vector_set(key.mValue.x, key.mValue.y, key.mValue.z, 1.f);
        layer.positionKeys[k] = static_cast<f32>(key.mTime);
      }

      for (auto k{ 0 }; k < layer.numScaleFrames; k++) {
        auto const key{ channel.mScalingKeys[k] };
        layer.scales[k] = rtm::vector_set(key.mValue.x, key.mValue.y, key.mValue.z, 0.f);
        layer.scaleKeys[k] = static_cast<f32>(key.mTime);
      }
    }

    break; // TODO extract all anims
  }

  return out.str();
}

std::string testMeshImportSimple(char const* filename) {
  auto processFlags{
    aiProcess_CalcTangentSpace |
    aiProcess_JoinIdenticalVertices |
    aiProcess_Triangulate |
    aiProcess_ValidateDataStructure |
    aiProcess_FindDegenerates |
    aiProcess_FindInvalidData |
    aiProcess_MakeLeftHanded |
    aiProcess_FlipWindingOrder
  };
  Assimp::Importer imp;
  auto scene{ imp.ReadFile(filename, processFlags) };
  if (!scene) {
    LOG(ImportMesh, Error, "Failed to import model");
    return "";
  }

  std::ostringstream out;

  MeshHeader header{};
  for (auto n{ 0 }; n < scene->mNumMeshes; n++) {
    auto const& mesh{ *scene->mMeshes[n] };
    if (mesh.mPrimitiveTypes != aiPrimitiveType_TRIANGLE) continue; // TODO

    if (n == 0) header.flags = getFlags(mesh);
    else if (header.flags != getFlags(mesh)) continue;// ASSERT(header.flags == getFlags(mesh));

    header.numIndices  += mesh.mNumFaces * 3;
    header.numVertices += mesh.mNumVertices;
    header.subMeshCount++;
  }
  ASSERT(header.subMeshCount == 1);

  out.write(reinterpret_cast<char const*>(&header), sizeof(header));

  auto const& mesh{ *scene->mMeshes[0] };
  MeshHeader::SubMesh sm;
  sm.materialIndex = 0;
  sm.unused0 = 0;
  sm.unused1 = 0;
  sm.indexCount = mesh.mNumFaces * 3;
  sm.indexOffset = 0;
  sm.vertexStart = 0;
  out.write(reinterpret_cast<char const*>(&sm), sizeof(sm));

  for (auto i{ 0 }; i < mesh.mNumFaces; i++) {
    auto const& face = mesh.mFaces[i];
    ASSERT(face.mNumIndices == 3);
    out.write(reinterpret_cast<char const*>(face.mIndices), 12);
  }

  out.write(reinterpret_cast<char const*>(mesh.mVertices), 12 * mesh.mNumVertices);
  out.write(reinterpret_cast<char const*>(mesh.mNormals), 12 * mesh.mNumVertices);
  return out.str();
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
