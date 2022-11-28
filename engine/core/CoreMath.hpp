#pragma once

#include "core/CoreTypes.hpp"

#include <rtm/matrix3x4f.h>
#include <rtm/matrix4x4f.h>
#include <rtm/quatf.h>
#include <rtm/vector4f.h>

union Mat4 {
  rtm::matrix4x4f m4;
  rtm::matrix3x4f m3;
  f32 v[4][4];
  f32 a[16];
};

union Quat {
  rtm::quatf q;
  f32 a[4];
};

union Vec4 {
  rtm::vector4f v;
  f32 a[4];
};
