#pragma once

#include "asset/Importer.hpp"

namespace Asset {

class AudioImporter : public Importer {
protected:
  bool process() override;

public:
  Type const& getType() const override;
};

} // namespace Asset
