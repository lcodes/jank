#!/usr/bin/env python3

import util

def build_compressonator():
  if util.is_windows:
    util.devenv('VS2015/CompressonatorLib', 'external/Compressonator/Compressonator', libs={
      'Build/VS2015/Debug_MD/x64/Compressonator_MDd.lib': 'Compressonator.lib',
      'Build/VS2015/Release_MD/x64/Compressonator_MD.lib': 'Compressonator.lib'
    }, vcxproj_files=[
      'VS2015/CompressonatorLib.vcxproj'
    ], debug='Debug_MD', release='Release_MD', upgrade=True)
  else:
    util.cmake('Compressonator', 'external/Compressonator/Compressonator/Make', [

    ], libs={

    })

if __name__ == "__main__":
  build_compressonator()
