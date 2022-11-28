#pragma once

#include "asset/Importer.hpp"

#include <assimp/postprocess.h>

#include <type_traits>

template<typename T>
inline void bitSet(T& bits, T bit, bool v) {
  using N = std::underlying_type_t<T>;
  if (v) {
    *reinterpret_cast<N*>(&bits) |= bit;
  }
  else {
    *reinterpret_cast<N*>(&bits) &= ~bit;
  }
}

namespace Asset {

class MeshImporter : public Importer {
  aiPostProcessSteps processFlags;
  aiComponent        removeComponents;

  // generate physics mesh

protected:
  bool process() override;

public:
  MeshImporter();

  Type const& getType() const override;

  bool getCalcTangentSpace() const { return processFlags & aiProcess_CalcTangentSpace; }
  void setCalcTangentSpace(bool v) { bitSet(processFlags, aiProcess_CalcTangentSpace, v); }

  // TODO optimize

  // TODO normals? tangents?

  // TODO individual sets?
  bool getUseVertexColors() const { return (removeComponents & aiComponent_COLORS) == 0; }
  void setUseVertexColors(bool v) { bitSet(removeComponents, aiComponent_COLORS, !v); }

  // TODO individual sets?
  bool getUseTexCoords() const { return (removeComponents & aiComponent_TEXCOORDS) == 0; }
  void setUseTexCoords(bool v) { bitSet(removeComponents, aiComponent_TEXCOORDS, !v); }

  bool getUseSkeleton() const { return (removeComponents & aiComponent_BONEWEIGHTS) == 0; }
  void setUseSkeleton(bool v) { bitSet(removeComponents, aiComponent_BONEWEIGHTS, !v); }

  bool getUseEmbeddedTextures() const { return (removeComponents & aiComponent_TEXTURES) == 0; }
  void setUseEmbeddedTextures(bool v) { bitSet(removeComponents, aiComponent_TEXTURES, !v); }

  bool getUseAnimations() const { return (removeComponents & aiComponent_ANIMATIONS) == 0; }
  void setUseAnimations(bool v) { bitSet(removeComponents, aiComponent_ANIMATIONS, !v); }

  bool getUseCameras() const { return (removeComponents & aiComponent_CAMERAS) == 0; }
  void setUseCameras(bool v) { bitSet(removeComponents, aiComponent_CAMERAS, !v); }

  bool getUseLights() const { return (removeComponents & aiComponent_LIGHTS) == 0; }
  void setUseLights(bool v) { bitSet(removeComponents, aiComponent_LIGHTS, !v); }

  // TODO remove mesh?

  bool getUseMaterials() const { return (removeComponents & aiComponent_MATERIALS) == 0; }
  void setUseMaterials(bool v) { bitSet(removeComponents, aiComponent_MATERIALS, !v); }
};

} // namespace Asset
