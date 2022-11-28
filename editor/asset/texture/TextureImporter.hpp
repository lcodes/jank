#pragma once

#include "asset/Importer.hpp"

namespace Asset {

class TextureImporter : public Importer {
  u16 maxSize;

  // Wrapping Mode
  // Filters
  // Anisotropy

  // Compression

protected:
  bool process() override;

public:
  Type const& getType() const override;
};

} // namespace Asset

