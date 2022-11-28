#include "anim/Skeleton.hpp"

#include "core/CoreDebug.hpp"

namespace Anim {

void Skeleton::setJoints(std::unique_ptr<Symbol[]>&& names,
                 std::unique_ptr<Mat4[]>&& joints,
                 std::unique_ptr<Mat4[]>&& inverses,
                 std::unique_ptr<u8[]>&& parents,
                 u8 numJoints)
{
  this->names     = std::move(names);
  this->joints    = std::move(joints);
  this->inverses  = std::move(inverses);
  this->parents   = std::move(parents);
  this->numJoints = numJoints;
}

void Skeleton::load(std::istream& i) {
}

void Skeleton::save(std::ostream& o) const {
  o << numJoints;
  o.write(reinterpret_cast<char const*>(joints  .get()), numJoints * sizeof(Mat4));
  o.write(reinterpret_cast<char const*>(inverses.get()), numJoints * sizeof(Mat4));
  o.write(reinterpret_cast<char const*>(parents .get()), numJoints * sizeof(u8));

  for (auto n{ 0u }; n < numJoints; n++) {
    auto const& name{ *names[n] };
    ASSERT(name.size() <= 0xFF);

    o << static_cast<u8>(name.size());
    o.write(name.data(), name.length());
  }
}

} // namespace Anim
