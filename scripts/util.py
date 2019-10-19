import multiprocessing
import os
import platform
import shutil
import subprocess
import sys

def error(*args, **kwargs):
  print(*args, file=sys.stderr, **kwargs)

if __name__ == "__main__":
  error("Don't run utils.py directly!")
  quit()

platform_dirs = {
  "Darwin":  "macos",
  "Linux":   "linux",
  "Windows": "windows"
}

root_dir = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
temp_dir = os.path.join(root_dir, ".setup")

inc_dir = os.path.join(root_dir, '3rdparty', 'include')
lib_dir = os.path.join(root_dir, '3rdparty', 'lib', platform_dirs.get(platform.system()), 'x64')

def __copy(name, build_dir, out_dir):
  src = os.path.join(build_dir, name)
  dst = os.path.join(out_dir,   os.path.basename(name))

  if not os.path.isdir(out_dir):
    os.makedirs(out_dir)
  elif os.path.isdir(dst):
    shutil.rmtree(dst)
  else:
    os.remove(dst)

  if os.path.isdir(src):
    shutil.copytree(src, dst)
  else:
    shutil.copyfile(src, dst)

def __cmake_run_make(input_dir, args, includes, libs, config_name, config):
  build_dir = os.path.join(temp_dir, os.path.basename(input_dir), config)
  if not os.path.isdir(build_dir):
    os.makedirs(build_dir)

  subprocess.run(
    ['cmake'] + args + ["-DCMAKE_BUILD_TYPE=" + config,
                        os.path.join(root_dir, input_dir)],
    cwd=build_dir,
    check=True
  )

  subprocess.run(
    ['make', '-j', str(multiprocessing.cpu_count())],
    cwd=build_dir,
    check=True
  )

  inc_dir_full = os.path.join(inc_dir, config_name)
  lib_dir_full = os.path.join(lib_dir, config_name)

  for inc in includes:
    __copy(inc, build_dir, inc_dir_full)

  for lib in libs:
    __copy(lib, build_dir, lib_dir_full)

def cmake(input_dir, args, includes, libs):
  __cmake_run_make(input_dir, args, includes, libs, "debug",   "Debug")
  __cmake_run_make(input_dir, args, includes, libs, "release", "RelWithDebInfo")
