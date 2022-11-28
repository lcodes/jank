#pragma once

#include "core/CoreObject.hpp"

//#include "gpu/Texture.hpp"

namespace Gpu {
class GraphicsPipeline;
class Texture;
class TextureView;
}

namespace Render {

// Shaders
// - System  -> No material
// - Generic -> No material code
// - Material -> Material code
// - Surface  -> Mesh & Material code

class Technique : Object {
  // Index by pass (depth, shadow, base, forward, ...)
  // Index by mesh (static, skinned, decals, particles, ...)
  // Also features (tangents, uv channels, vertex colors, displacement)
  // More features (forward lights, )
  // Even more (shader model, define permutations, ...)
  Gpu::GraphicsPipeline* pipelines;

  // TODO shader sources
};

class Material : Object {
  Technique* tech;

  Gpu::Texture* textures;

  // TODO subsurface profile
  // TODO uniforms
  // TODO parameters
};

using TechniquePtr = ObjectPtr<Technique>;
using MaterialPtr = ObjectPtr<Material>;


class MaterialParams : NonCopyable {
  Gpu::TextureView* textureViews;
};

} // namespace Render
