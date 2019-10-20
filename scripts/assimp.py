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
  ], {
    'code/libassimp.a': 'libassimp.a',
    'code/Debug/assimp-vc142-mt.lib': 'assimp.lib',
    'code/Debug/assimp-vc142-mt.pdb': 'assimp.pdb',
    'code/RelWithDebInfo/assimp-vc142-mt.lib': 'assimp.lib',
    'code/RelWithDebInfo/assimp-vc142-mt.pdb': 'assimp.pdb'
  })

if __name__ == "__main__":
  build_assimp()
