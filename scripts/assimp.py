#!/usr/bin/env python3

import util

def build_assimp():
  util.cmake('external/assimp', [
    '-DBUILD_SHARED_LIBS=OFF',
    '-DASSIMP_NO_EXPORT=ON',
    '-DASSIMP_BUILD_ASSIMP_TOOLS=OFF',
    '-DASSIMP_BUILD_TESTS=OFF',
    '-DINJECT_DEBUG_POSTFIX=OFF'
  ], [
    'include/assimp'
  ], [
    'code/libassimp.a'
  ])

if __name__ == "__main__":
  build_assimp()
