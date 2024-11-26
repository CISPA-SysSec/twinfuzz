#!/bin/bash -eux
# Copyright 2016 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################

export CC="clang"
export CXX="clang++"
export CFLAGS=''
export CXXFLAGS=''

# Disable UBSan vptr since several targets built with -fno-rtti.
export CFLAGS="$CFLAGS -fno-sanitize=vptr"
export CXXFLAGS="$CXXFLAGS -fno-sanitize=vptr"

cd $SRC/x265
cd build/linux
cmake -G "Unix Makefiles" ../../source
make -j$(nproc)
make install

# Extract and build alsa-lib
cd $SRC
tar xf alsa-lib-1.1.0.tar.bz2
cd alsa-lib-1.1.0
./configure --enable-static --disable-shared
make clean
make -j$(nproc) all
make install

# Build fdk-aac
cd $SRC/fdk-aac
autoreconf -fiv
CXXFLAGS="$CXXFLAGS -fno-sanitize=shift-base,signed-integer-overflow" \
./configure --disable-shared
make clean
make -j$(nproc) all
make install

# Build and install libva
cd $SRC/libva
./autogen.sh --enable-x11 --prefix=/usr --libdir=/usr/lib/x86_64-linux-gnu
make -j$(nproc)
make install
ldconfig -vvv

# Build and install cmrt
cd $SRC/cmrt
./autogen.sh
make -j$(nproc)
make install

# Build and install gmmlib
mkdir -p $SRC/build-gmmlib
cd $SRC/build-gmmlib
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_LIBDIR=/usr/lib/x86_64-linux-gnu ../gmmlib
make -j$(nproc)
make install

# Build and install intel-media-driver
mkdir -p $SRC/build-media-driver
cd $SRC/build-media-driver
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_INSTALL_LIBDIR=/usr/lib/x86_64-linux-gnu ../media-driver
make -j$(nproc)
make install

cd $SRC/libvdpau
./autogen.sh --enable-static --disable-shared
make clean
make -j$(nproc) all
make install

# Build libvpx
cd $SRC/libvpx
LDFLAGS="$CXXFLAGS" ./configure \
        --disable-examples --disable-unit-tests \
        --size-limit=12288x12288 \
        --extra-cflags="-DVPX_MAX_ALLOCABLE_MEMORY=1073741824"
make clean
make -j$(nproc) all
make install

# Build ogg
cd $SRC/ogg
./autogen.sh
./configure --enable-static --disable-crc
make -j$(nproc)
make install

# Build theora
cd $SRC/theora
CFLAGS="$CFLAGS -fPIC"
./autogen.sh
./configure --enable-static --disable-examples
make -j$(nproc)
make install

# Build vorbis
cd $SRC/vorbis
./autogen.sh
./configure --enable-static
make -j$(nproc)
make install

# Build libxml2
cd $SRC/libxml2
./autogen.sh --enable-static \
      --without-debug --without-ftp --without-http \
      --without-legacy --without-python
make -j$(nproc)
make install

# Build srt
cd $SRC/srt
./configure
make -j$(nproc)
make install
