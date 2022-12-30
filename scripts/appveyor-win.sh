#!/bin/bash

set -xe

# temp test
scripts/build-ffmpeg.sh . \
	--target-os=win64 --arch=x86_64 --toolchain=msvc \
	--enable-dxva2 --enable-hwaccel=h264_dxva2 --enable-hwaccel=hevc_dxva2 \
	--enable-d3d11va --enable-hwaccel=h264_d3d11va --enable-hwaccel=hevc_d3d11va
FFMPEG_ROOT="$(cygpath -m "$(realpath ffmpeg-prefix)")"  # `cygpath -m` converts path to `C:/...`

vcpkg install --triplet x64-windows yasm opus sdl2 protobuf
VCPKG_ROOT="C:/tools/vcpkg/installed/x64-windows"
export PATH="$(cygpath $VCPKG_ROOT)/tools/yasm:$(cygpath $VCPKG_ROOT)/tools/protobuf:$PATH"

#wget https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-n5.1-latest-win64-gpl-shared-5.1.zip && 7z x ffmpeg-n5.1-latest-win64-gpl-shared-5.1.zip
#mv ffmpeg-n5.1-latest-win64-gpl-shared-5.1 ffmpeg-prefix
#FFMPEG_ROOT="$(cygpath -m "$(realpath ffmpeg-prefix)")"  # `cygpath -m` converts path to `C:/...`

wget https://mirror.firedaemon.com/OpenSSL/openssl-1.1.1s.zip && 7z x openssl-1.1.1s.zip
OPENSSL_ROOT="$(cygpath -m "$(realpath openssl-1.1/x64)")"

PYTHON="C:/Python37-x64/python.exe"
"$PYTHON" -m pip install "protobuf>=3,<4"

QT_ROOT="C:/Qt/5.15/msvc2019_64"

COPY_DLLS="$VCPKG_ROOT/bin/SDL2.dll \
$OPENSSL_ROOT/bin/libcrypto-1_1-x64.dll \
$OPENSSL_ROOT/bin/libssl-1_1-x64.dll \
$FFMPEG_ROOT/bin/avcodec-59.dll \
$FFMPEG_ROOT/bin/avutil-57.dll \
$FFMPEG_ROOT/bin/swresample-4.dll"

echo "-- Configure"

mkdir build && cd build

cmake \
	-G Ninja \
	-DCMAKE_C_COMPILER=cl \
	-DCMAKE_C_FLAGS="-we4013" \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_PREFIX_PATH="$FFMPEG_ROOT;$OPENSSL_ROOT;$QT_ROOT;$VCPKG_ROOT" \
	-DPYTHON_EXECUTABLE="$PYTHON" \
	-DCHIAKI_ENABLE_TESTS=ON \
	-DCHIAKI_ENABLE_CLI=OFF \
	-DCHIAKI_GUI_ENABLE_SDL_GAMECONTROLLER=ON \
	--debug-output \
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

ls -lha Chiaki
"$QT_ROOT/bin/windeployqt.exe" Chiaki/chiaki.exe
ls -lha Chiaki
cp -v $COPY_DLLS Chiaki
