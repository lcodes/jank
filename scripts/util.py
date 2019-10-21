import multiprocessing
import os
import platform
import re
import shutil
import subprocess
import sys

is_windows = platform.system() == 'Windows'

if is_windows:
  import winreg # TODO use this to find where devenv is installed
  devenv = 'C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\Common7\\IDE\\devenv.com'

def error(*args, **kwargs):
  print(*args, file=sys.stderr, **kwargs)

if __name__ == '__main__':
  error('Do not run utils.py directly!')
  quit()

platform_dirs = {
  'Darwin':  'macos',
  'Linux':   'linux',
  'Windows': 'windows'
}

root_dir = os.path.abspath(os.path.dirname(os.path.dirname(__file__)))
temp_dir = os.path.join(root_dir, '.setup')

inc_dir = os.path.join(root_dir, '3rdparty', 'include')
lib_dir = os.path.join(root_dir, '3rdparty', 'lib', platform_dirs.get(platform.system()), 'x64')

def __copy(src, dst, out_dir, use_move):
  if os.path.exists(src):
    print('Moving' if use_move else 'Copying', 'file', src, 'to', dst)

    if not os.path.isdir(out_dir):
      os.makedirs(out_dir)
    elif os.path.isdir(dst):
      shutil.rmtree(dst)
    elif os.path.isfile(dst):
      os.remove(dst)

    if use_move:
      shutil.move(src, dst)
    elif os.path.isdir(src):
      shutil.copytree(src, dst)
    else:
      shutil.copyfile(src, dst)

def __copy_all(build_dir, kwargs, config_name):
  inc_dir_full = os.path.join(inc_dir, config_name)
  lib_dir_full = os.path.join(lib_dir, config_name)

  for inc in kwargs['includes']:
    src = os.path.join(build_dir, inc)
    dst = os.path.join(inc_dir_full, os.path.basename(inc))
    __copy(src, dst, inc_dir_full, False)

  for lib in kwargs['libs']:
    src = os.path.join(build_dir, lib)
    dst = os.path.join(lib_dir_full, kwargs['libs'][lib])
    __copy(src, dst, lib_dir_full, True)

def __cmake(input_dir, build_dir, args, kwargs):
  subprocess.run(['cmake'] + args + [os.path.join(root_dir, input_dir)],
                 cwd=build_dir,
                 check=True)

def __cmake_devenv_build(build_dir, name, config):
  subprocess.run([devenv, name + '.sln', '/Build', config + '|x64'],
                 cwd=build_dir,
                 check=True)

def __cmake_devenv(input_dir, args, kwargs):
  name      = os.path.basename(input_dir)
  build_dir = os.path.join(temp_dir, name)

  if not os.path.isdir(build_dir):
    os.makedirs(build_dir)

  __cmake(input_dir, build_dir, args, kwargs)

  for file_name in kwargs['vcxproj_files']:
    vcxproj_file = os.path.join(build_dir, file_name)

    with open(vcxproj_file, 'r') as f:
      vcxproj = f.read()

    vcxproj = re.sub('_DEBUG', '_ITERATOR_DEBUG_LEVEL=1;_DEBUG', vcxproj)

    with open(vcxproj_file, 'w') as f:
      f.write(vcxproj)

  __cmake_devenv_build(build_dir, name, 'Debug')
  __copy_all(build_dir, kwargs, 'debug')

  __cmake_devenv_build(build_dir, name, 'RelWithDebInfo')
  __copy_all(build_dir, kwargs, 'release')

def __cmake_make(input_dir, args, kwargs, config_name, config):
  build_dir = os.path.join(temp_dir, os.path.basename(input_dir), config)

  if not os.path.isdir(build_dir):
    os.makedirs(build_dir)

  __cmake(input_dir, build_dir, args + ['-DCMAKE_BUILD_TYPE=' + config], kwargs)

  subprocess.run(['make', '-j', str(multiprocessing.cpu_count())],
                 cwd=build_dir,
                 check=True)

  __copy_all(build_dir, kwargs, config_name)

def cmake(input_dir, args, **kwargs):
  if is_windows:
    __cmake_devenv(input_dir, args, kwargs)
  else:
    __cmake_make(input_dir, args, kwargs, 'debug',   'Debug')
    __cmake_make(input_dir, args, kwargs, 'release', 'RelWithDebInfo')
