#!/usr/bin/env python3

import util

def build_assimp():
  util.cmake('assimp', 'external/assimp', [
    '-DBUILD_SHARED_LIBS=OFF',
    '-DASSIMP_NO_EXPORT=ON',
    '-DASSIMP_BUILD_ASSIMP_TOOLS=OFF',
    '-DASSIMP_BUILD_TESTS=OFF',
    '-DINJECT_DEBUG_POSTFIX=OFF'
  ], includes=[
    'include/assimp'
  ], libs={
    'code/libassimp.a':                           'libassimp.a',
    'code/Debug/assimp-vc142-mt.lib':             'assimp.lib',
    'code/Debug/assimp-vc142-mt.pdb':             'assimp-vc142-mt.pdb',
    'code/RelWithDebInfo/assimp-vc142-mt.lib':    'assimp.lib',
    'code/RelWithDebInfo/assimp-vc142-mt.pdb':    'assimp-vc142-mt.pdb',
    'contrib/irrXML/Debug/IrrXML.lib':            'IrrXML.lib',
    'contrib/irrXML/RelWithDebInfo/IrrXML.lib':   'IrrXML.lib',
    'contrib/zlib/Debug/zlibstaticd.lib':         'zlib.lib',
    'contrib/zlib/RelWithDebInfo/zlibstatic.lib': 'zlib.lib'
  }, vcxproj_files=[
    'code/assimp.vcxproj',
    'contrib/irrXML/IrrXML.vcxproj'
  ])

if __name__ == '__main__':
  build_assimp()
