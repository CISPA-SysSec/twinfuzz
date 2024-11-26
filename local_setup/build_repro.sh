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
export CFLAGS='-g'
export CXXFLAGS='-g'

# Disable UBSan vptr since several targets built with -fno-rtti.
export CFLAGS="$CFLAGS -fno-sanitize=vptr"
export CXXFLAGS="$CXXFLAGS -fno-sanitize=vptr"

# Build ffmpeg.
cd $SRC/ffmpeg

./configure \
        --cc="$CC" --cxx="$CXX" --ld="$CXX $CXXFLAGS -fuse-ld=lld -O2 -fno-omit-frame-pointer -gline-tables-only -DFUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION -fno-sanitize=vptr -std=c++11" \
         --extra-cflags='-I/usr/local/include -fsanitize=fuzzer-no-link,address' --extra-ldflags='-L/usr/local/lib -fsanitize=fuzzer-no-link,address' \
        --pkg-config-flags="--static" \
        --optflags=-O2 \
        --libfuzzer='-fsanitize=fuzzer,address' \
        --enable-gpl --enable-nonfree --enable-version3 --enable-libass --enable-libfdk-aac --enable-libopenjpeg --enable-libmp3lame  --enable-libssh --enable-libtheora --enable-libvorbis --enable-libvpx --enable-libwebp --enable-libx264 --enable-libxvid --enable-libxml2 --enable-opengl --enable-openssl --enable-libsrt --enable-decoders --disable-shared  --enable-hwaccels --enable-vaapi --enable-libdrm --enable-opencl --enable-libssh --enable-libtheora\
        --enable-nonfree
make clean
make -j$(nproc) install






# Build the fuzzers.
cd $SRC/ffmpeg

FUZZ_TARGET_SOURCE=$SRC/ffmpeg/tools/target_dec_fuzzer.c

export TEMP_VAR_CODEC="AV_CODEC_ID_H264"
export TEMP_VAR_CODEC_TYPE="VIDEO"



# Build fuzzers for decoders.
CONDITIONALS=$(grep 'H264_DECODER 1$' config_components.h | sed 's/#define CONFIG_\(.*\)_DECODER 1/\1/')
if [ -n "${OSS_FUZZ_CI-}" ]; then
      # When running in CI, check the first targets only to save time and disk space
      CONDITIONALS=(${CONDITIONALS[@]:0:2})
fi
for c in $CONDITIONALS; do
      fuzzer_name=ffmpeg_AV_CODEC_ID_${c}_fuzzer
      symbol=$(echo $c | sed "s/.*/\L\0/")
      echo -en "[libfuzzer]\nmax_len = 1000000\n" >$OUT/${fuzzer_name}.options
      make tools/target_dec_${symbol}_fuzzer
      mv tools/target_dec_${symbol}_fuzzer $OUT/${fuzzer_name}
      patchelf --set-rpath '$ORIGIN/lib' $OUT/$fuzzer_name
done