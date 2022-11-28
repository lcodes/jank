#!/usr/bin/env python3

import util

def build_ffmpeg():
  util.configure('ffmpeg', 'external/FFmpeg', [
    '--toolchain=msvc',
    '--target-os=win64', # TODO
    '--disable-x86asm', # TODO
    '--disable-autodetect',
    '--disable-programs',
    '--disable-doc',
    '--disable-everything',
    '--enable-decoder=aac',
    '--enable-decoder=ac3',
    '--enable-decoder=flac',
    '--enable-decoder=mp3',
    '--enable-decoder=vorbis',
    '--enable-encoder=vorbis'
  ])

if __name__ == '__main__':
  build_ffmpeg()
