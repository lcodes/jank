#!/usr/bin/env python3

import util

def build_lz4():
  if util.is_windows:
    util.devenv('visual/VS2017/lz4', 'external/lz4', libs={
      'visual/VS2017/bin/x64_Debug/liblz4_static.lib': 'lz4.lib',
      'visual/VS2017/bin/x64_Release/liblz4_static.lib': 'lz4.lib',
    }, project='liblz4', upgrade=True)

if __name__ == '__main__':
  build_lz4()
