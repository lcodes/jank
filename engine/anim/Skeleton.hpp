#pragma once

#include "core/CoreObject.hpp"
#include "core/CoreMath.hpp"
#include "core/CoreString.hpp"

#include <memory>

namespace Anim {

class Skeleton : public Object {
  std::unique_ptr<Symbol[]> names;
  std::unique_ptr<Mat4[]>   joints;
  std::unique_ptr<Mat4[]>   inverses;
  std::unique_ptr<u8[]>     parents;

  u8 numJoints;

public:
  static constexpr u8 invalidBoneId = UINT8_MAX;

  void setJoints(std::unique_ptr<Symbol[]>&& names,
                 std::unique_ptr<Mat4[]>&& joints,
                 std::unique_ptr<Mat4[]>&& inverses,
                 std::unique_ptr<u8[]>&& parents,
                 u8 numJoints);

  u8 getNumJoints() const { return numJoints; }

  void load(std::istream& i) override;
  void save(std::ostream& o) const override;
};

using SkeletonPtr = ObjectPtr<Skeleton>;

} // namespace Anim
