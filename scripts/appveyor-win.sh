#!/bin/bash

set -xe

BUILD_ROOT="$(dirname "$(dirname "$(realpath "${BASH_SOURCE[0]}")")")"
BUILD_ROOT="$(echo $BUILD_ROOT | sed 's|^/\([a-z]\)|\1:|g')" # replace /c/... by c:/... for cmake to understand it
echo "BUILD_ROOT=$BUILD_ROOT"

vcpkg install --triplet x64-windows yasm opus sdl2 protobuf
VCPKG_ROOT="tools/vcpkg/installed/x64-windows"
export PATH="/c/$VCPKG_ROOT/tools/yasm:/c/$VCPKG_ROOT/tools/protobuf:$PATH"

wget https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-n5.1-latest-win64-gpl-shared-5.1.zip && 7z x ffmpeg-n5.1-latest-win64-gpl-shared-5.1.zip
mv ffmpeg-n5.1-latest-win64-gpl-shared-5.1 ffmpeg-prefix

wget https://mirror.firedaemon.com/OpenSSL/openssl-1.1.1s.zip && 7z x openssl-1.1.1s.zip

PYTHON="C:/Python37-x64/python.exe"
"$PYTHON" -m pip install "protobuf>=3"

QT_PATH="C:/Qt/5.15/msvc2019_64"

COPY_DLLS="$PWD/openssl-1.1/x64/bin/libcrypto-1_1-x64.dll $PWD/openssl-1.1/x64/bin/libssl-1_1-x64.dll /c/$VCPKG_ROOT/bin/SDL2.dll $PWD/ffmpeg-prefix/bin/avcodec-59.dll $PWD/ffmpeg-prefix/bin/avutil-57.dll $PWD/ffmpeg-prefix/bin/swresample-4.dll "

echo "-- Configure"

mkdir build && cd build

cmake \
	-G Ninja \
	-DCMAKE_C_COMPILER=cl \
	-DCMAKE_C_FLAGS="-we4013" \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_PREFIX_PATH="C:/$VCPKG_ROOT;$BUILD_ROOT/ffmpeg-prefix;$BUILD_ROOT/openssl-1.1/x64;$QT_PATH" \
	-DPYTHON_EXECUTABLE="$PYTHON" \
	-DCHIAKI_ENABLE_TESTS=ON \
	-DCHIAKI_ENABLE_CLI=OFF \
	-DCHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER=ON \
	..

echo "-- Build"

ninja -v

echo "-- Test"

cp $COPY_DLLS test/
test/chiaki-unit.exe

cd ..


# Deploy

echo "-- Deploy"

mkdir Chiaki && cp build/gui/chiaki.exe Chiaki
mkdir Chiaki-PDB && cp build/gui/chiaki.pdb Chiaki-PDB

"$QT_PATH/bin/windeployqt.exe" Chiaki/chiaki.exe
ls -lha Chiaki
cp -v $COPY_DLLS Chiaki
