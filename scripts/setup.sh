#!/bin/sh -e

# TODO install sdks in system dir? ie /opt

# TODO make this work on macos
# TODO make this work on windows (cygwin? mingw? WSL? cmder?)

# Platform Setup
# -----------------------------------------------------------------------------

case "$(uname -s)" in
  CYGWIN*) platform=windows;;
  MINGW*)  platform=windows;;
  Darwin*) platform=darwin;;
  Linux*)
    if grep -q Microsoft /proc/version; then
      platform=windows
    else
      platform=linux
    fi
    ;;
  *)
    >&2 echo ""
    exit -1
    ;;
esac

if [ $platform != "linux" ]; then
  >&2 echo "TODO: support for $platform"
  exit -1
fi


# Configuration
# -----------------------------------------------------------------------------

android_sdk_url=https://dl.google.com/android/repository/sdk-tools-$platform-4333796.zip

android_build_tools_version=29.0.2
# android_cmake_version=3.10.2.4988404 # TODO use system cmake? 3.13.4 is latest
android_ndk_version=20.0.5594570
android_platform_version=android-29


# Initialization
# -----------------------------------------------------------------------------

script=$(readlink -f "$0")
script_dir=$(dirname "$script")
root_dir=$(cd "$script_dir/.." && pwd -P)
setup_dir=$root_dir/.setup

mkdir -p "$setup_dir"


# System Utilities
# -----------------------------------------------------------------------------

# TODO curl ?

if ! command -v git >/dev/null; then
  sudo apt -y install git
fi

if ! command -v g++ >/dev/null || ! command -v make >/dev/null; then
  sudo apt -y install build-essential
fi

add_to_bash() {
  updated_env=yes
  printf "\\n# Added by %s\\n%s\\n" "$script" "$1" >> ~/.bashrc
}

add_to_path() {
  add_to_bash "export PATH=$1:\$PATH"
}

# Java (required for Android)
# -----------------------------------------------------------------------------

install_java() {
  sudo apt -y install openjdk-8-jdk
}

find_java() {
  java_alt=$(update-java-alternatives -l | grep java-1.8.0 | awk '{print $1}')
}

if ! command -v java >/dev/null; then
  install_java
fi

if ! update-java-alternatives -l | grep 1.8. >/dev/null; then
  if ! find_java; then
    install_java
    find_java
  fi

  sudo update-java-alternatives -s "$java_alt" 2>/dev/null
fi


# Android SDK
# -----------------------------------------------------------------------------

if ! sdkmanager=$(command -v sdkmanager); then
  android_sdk_dir=$setup_dir/android-sdk
  android_sdk_zip=$android_sdk_dir.zip

  if ! [ -d "$android_sdk_dir" ]; then
    if ! [ -f "$android_sdk_zip" ]; then
      curl $android_sdk_url -o "$android_sdk_zip"
    fi

    unzip -qu "$android_sdk_zip" -d "$android_sdk_dir"
  fi

  sdkmanager=$android_sdk_dir/tools/bin/sdkmanager

  add_to_path "$android_sdk_dir/tools/bin" # TODO macos brew links them into /usr/local/bin
  add_to_bash "export ANDROID_SDK_ROOT=$android_sdk_dir"
fi

yes | $sdkmanager --licenses >/dev/null 2>&1

$sdkmanager --install \
            "build-tools;$android_build_tools_version" \
            "ndk;$android_ndk_version" \
            "platform-tools" \
            "platforms;$android_platform_version" \
            >/dev/null 2>&1

if ! command -v gradle >/dev/null; then
  sudo apt -y install gradle
fi


# Emscripten
# -----------------------------------------------------------------------------

if ! command -v em++ >/dev/null; then
  emsdk_dir=$setup_dir/emsdk

  if ! [ -d "$emsdk_dir" ]; then
    git clone https://github.com/emscripten-core/emsdk.git "$emsdk_dir"
  else
    git "--git-dir=$emsdk_dir" pull
  fi

  emsdk=$emsdk_dir/emsdk

  if ! $emsdk list | grep INSTALLED | grep fastcomp; then
    $emsdk install latest
    $emsdk activate latest >/dev/null
  fi

  add_to_bash "source $emsdk_dir/emsdk_env.sh >/dev/null"
fi


# TODO
# -----------------------------------------------------------------------------

# Blender
# Python3, importmagic, epc


# External projects
# ----------------------------------------------------------------------------

git "--git-dir=$root_dir" submodule update --init --recursive


# Done
# -----------------------------------------------------------------------------

echo "Setup complete!"

if [ "$updated_env" != "" ]; then
  echo "Restart the shell to use the updated environment variables."
fi
