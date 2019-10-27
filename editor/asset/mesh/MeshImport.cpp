#include "core/Core.hpp"

#include <assimp/DefaultLogger.hpp>
#include <assimp/Importer.hpp>
#include <assimp/ProgressHandler.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>

#include "app/GfxTest.hpp"


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

static void const* readBuf(aiMaterialProperty const& prop, usize size) {
  ASSERT(prop.mType == aiPTI_Buffer);
  ASSERT(prop.mDataLength == size);
  return prop.mData;
}

static f32 readF32(aiMaterialProperty const& prop) {
  ASSERT(prop.mType == aiPTI_Float);
  ASSERT(prop.mDataLength == sizeof(f32));
  return *reinterpret_cast<f32 const*>(prop.mData);
}

union Color {
  f32 v[4];
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
  case 4:
    c.a = f[3];
    FALLTHROUGH;
  case 3:
    c.r = f[0];
    c.g = f[1];
    c.b = f[2];
    break;
  }
  return c;
}

u8 countSkeletonBones(aiNode const& node) {
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

i16 setupSkeletonLookup(aiNode const& node, BoneLookup* lookup, i16 parent = -1, i16 index = -1) {
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

Animation::Behaviour getAnimationBehaviour(aiAnimBehaviour const behaviour) {
  return static_cast<Animation::Behaviour>(behaviour); // Using the same values for now.
}

u32 getFlags(aiMesh const& mesh) {
  u32 flags = 0;
  // TODO mesh.HasFaces()
  if (mesh.HasPositions()) flags |= MeshHeader::HasPositions;
  if (mesh.HasTangentsAndBitangents()) flags |= MeshHeader::HasTangents;
  /*else*/ if (mesh.HasNormals()) flags |= MeshHeader::HasNormals;
  if (mesh.HasTextureCoords(0)) flags |= MeshHeader::HasTexCoords;
  if (mesh.HasBones()) flags |= MeshHeader::HasBones;
  return flags;
}

u32 importTexture(char const* filename, bool normal);
u32 importTextureData(void const* data, usize size, bool normal);


// Importer
// ----------------------------------------------------------------------------
#include <unordered_map>
std::string testMeshImport(Material** materials, u32* numMaterials, Skeleton* skeleton, Animation* animation) {
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

  auto scene{ imp.ReadFile("../data/mixamo/test/Cross Jumps.fbx", processFlags) };

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

  *numMaterials = scene->mNumMaterials;
  *materials = new Material[scene->mNumMaterials];

#define PROP(s) prop.mKey.length == sizeof(s) - 1 && memcmp(prop.mKey.data, s, sizeof(s) - 1) == 0
  for (auto n{ 0 }; n < scene->mNumMaterials; n++) {
    (*materials)[n].color = 0;
    (*materials)[n].normal = 0;
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
        if (semantic == aiTextureType_DIFFUSE || semantic == aiTextureType_NORMALS) {
          bool isNormal = semantic == aiTextureType_NORMALS;
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
                  id = importTextureData(tex.pcData, tex.mWidth, isNormal);
                  texCache.insert(std::make_pair(texName, TexCacheItem{ std::string{ texName }, id }));
                }
                found = true;
                break;
              }
            }

            if (!found) {
              //std::string texFile{ "../data/Alduin/" };
              //std::string texFile{ "../data/Harley Quinn/" };
              //std::string texFile{ "../data/Harley_Quinn/" };
              //std::string texFile{ "../data/Fat Man/" };
              //std::string texFile{ "../data/Flamethrower/" };
              std::string texFile{ "../data/Eyebot/" };
              //std::string texFile{ "../data/Main Outfit/" };
              texFile += name;
              if (auto it = texCache.find(texFile); it != texCache.end()) {
                id = it->second.id;
              }
              else {
                LOG(ImportMesh, Info, "TEX %.*s", static_cast<u32>(name.size()), name.data());
                id = importTexture(texFile.c_str(), isNormal);
                texCache.insert(std::make_pair(texFile, TexCacheItem{ texFile, id }));
              }
            }
          }

          if (isNormal) {
            (*materials)[n].normal = id;
          }
          else {
            (*materials)[n].color = id;
          }
        }
      }
      else if (PROP("$tex.file.texCoord")) {
        *reinterpret_cast<u32 const*>(readBuf(prop, sizeof(u32)));
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

  // Sanity check?
#if 0
  for (auto n{ 0 }; n < header.numVertices; n++) {
    for (auto i{ 0 }; i < 3; i++) {
      ASSERT(boneWeights[n][i] > 0);
    }
  }
#endif

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
  // Write the vertex normals.
  /*else*/ if (header.flags & MeshHeader::HasNormals) {
    for (auto n{ 0 }; n < scene->mNumMeshes; n++) {
      auto const& mesh{ *scene->mMeshes[n] };
      if (mesh.mPrimitiveTypes != aiPrimitiveType_TRIANGLE) continue; // TODO
      if (header.flags != getFlags(mesh)) continue;

      out.write(reinterpret_cast<char const*>(mesh.mNormals), 12 * mesh.mNumVertices);
    }
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

