#!/bin/bash

set -xe

# Prepare command line tools
NASM_VER=2.16.01
PROTO_VER=3.20.3
TOOLS_PATH="$(realpath .)/tools-bin"
mkdir -p "$TOOLS_PATH"
wget "https://www.nasm.us/pub/nasm/releasebuilds/$NASM_VER/win64/nasm-$NASM_VER-win64.zip"
7z e "nasm-$NASM_VER-win64.zip" -o"$TOOLS_PATH" "nasm-$NASM_VER/nasm.exe"
wget "https://github.com/protocolbuffers/protobuf/releases/download/v$PROTO_VER/protoc-$PROTO_VER-win64.zip"
7z e "protoc-$PROTO_VER-win64.zip" -o"$TOOLS_PATH" "bin/protoc.exe"
export PATH="$TOOLS_PATH:$PATH"

# Build third party libraries through vcpkg
VCPKG_TRIPLET="x64-windows-release"  # `-release` suffix is required
VCPKG_ROOT="C:/tools/vcpkg/installed/$VCPKG_TRIPLET"
vcpkg install --triplet $VCPKG_TRIPLET opus sdl2

# Build ffmpeg with hardware decoders on Windows
scripts/build-ffmpeg.sh . \
	--target-os=win64 --arch=x86_64 --toolchain=msvc \
	--enable-hwaccel=h264_dxva2 --enable-hwaccel=h264_d3d11va --enable-hwaccel=h264_d3d11va2 \
	--enable-hwaccel=hevc_dxva2 --enable-hwaccel=hevc_d3d11va --enable-hwaccel=hevc_d3d11va2
FFMPEG_ROOT="$(cygpath -m "$(realpath ffmpeg-prefix)")"  # `cygpath -m` converts path to `C:/...`

OPENSSL_ROOT="C:/OpenSSL-v111-Win64"

QT_ROOT="C:/Qt/5.15/msvc2019_64"

PYTHON="C:/Python311-x64/python.exe"
"$PYTHON" -m pip install protobuf==3.20.*

COPY_DLLS="$VCPKG_ROOT/bin/opus.dll $VCPKG_ROOT/bin/SDL2.dll \
$OPENSSL_ROOT/bin/libcrypto-1_1-x64.dll $OPENSSL_ROOT/bin/libssl-1_1-x64.dll"
COPY_PDBS="$VCPKG_ROOT/bin/opus.pdb $VCPKG_ROOT/bin/SDL2.pdb"

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
	..

echo "-- Build"

ninja

echo "-- Test"

cp $COPY_DLLS test/
test/chiaki-unit.exe

cd ..


# Deploy

echo "-- Deploy"

mkdir Chiaki && cp build/gui/chiaki.exe Chiaki
mkdir Chiaki-PDB && cp build/gui/chiaki.pdb Chiaki-PDB

"$QT_ROOT/bin/windeployqt.exe" Chiaki/chiaki.exe
cp -v $COPY_DLLS Chiaki
cp -v $COPY_PDBS Chiaki-PDB
