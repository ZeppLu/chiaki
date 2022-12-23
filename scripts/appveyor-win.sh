#!/bin/bash

set -xe

BUILD_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." && pwd )"
BUILD_ROOT="$(echo $BUILD_ROOT | sed 's|^/\([a-z]\)|\1:|g')" # replace /c/... by c:/... for cmake to understand it
echo "BUILD_ROOT=$BUILD_ROOT"

mkdir yasm && cd yasm
wget http://www.tortall.net/projects/yasm/releases/yasm-1.3.0-win64.exe && mv yasm-1.3.0-win64.exe yasm.exe
cd ..

export PATH="$PWD/yasm:/c/Qt/5.15/msvc2019_64/bin:$PATH"

wget https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-n5.1-latest-win64-gpl-shared-5.1.zip && 7z x ffmpeg-n5.1-latest-win64-gpl-shared-5.1.zip
mv ffmpeg-n5.1-latest-win64-gpl-shared-5.1 ffmpeg-prefix

git clone https://github.com/xiph/opus.git && cd opus && git checkout v1.3.1
mkdir build && cd build
cmake \
	-G Ninja \
	-DCMAKE_C_COMPILER=cl \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX="$BUILD_ROOT/opus-prefix" \
	..
ninja
ninja install
cd ../..

wget https://mirror.firedaemon.com/OpenSSL/openssl-1.1.1s.zip && 7z x openssl-1.1.1s.zip

wget https://www.libsdl.org/release/SDL2-devel-2.26.1-VC.zip && 7z x SDL2-devel-2.26.1-VC.zip
export SDL_ROOT="$BUILD_ROOT/SDL2-2.26.1"
export SDL_ROOT=${SDL_ROOT//[\\]//}
echo "set(SDL2_INCLUDE_DIRS \"$SDL_ROOT/include\")
set(SDL2_LIBRARIES \"$SDL_ROOT/lib/x64/SDL2.lib\")
set(SDL2_LIBDIR \"$SDL_ROOT/lib/x64\")" > "$SDL_ROOT/SDL2Config.cmake"

mkdir protoc && cd protoc
wget https://github.com/protocolbuffers/protobuf/releases/download/v3.9.1/protoc-3.9.1-win64.zip && 7z x protoc-3.9.1-win64.zip
cd ..
export PATH="$PWD/protoc/bin:$PATH"

PYTHON="C:/Python37/python.exe"
"$PYTHON" -m pip install protobuf==3.19.5

QT_PATH="C:/Qt/5.15/msvc2019_64"

COPY_DLLS="$PWD/openssl-1.1/x64/bin/libcrypto-1_1-x64.dll $PWD/openssl-1.1/x64/bin/libssl-1_1-x64.dll $SDL_ROOT/lib/x64/SDL2.dll"

echo "-- Configure"

mkdir build && cd build

cmake \
	-G Ninja \
	-DCMAKE_C_COMPILER=cl \
	-DCMAKE_C_FLAGS="-we4013" \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_PREFIX_PATH="$BUILD_ROOT/ffmpeg-prefix;$BUILD_ROOT/opus-prefix;$BUILD_ROOT/openssl-1.1/x64;$QT_PATH;$SDL_ROOT" \
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

"$QT_PATH/bin/windeployqt.exe" Chiaki/chiaki.exe
cp -v $COPY_DLLS Chiaki
