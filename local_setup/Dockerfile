FROM gcr.io/oss-fuzz-base/base-builder@sha256:19782f7fe8092843368894dbc471ce9b30dd6a2813946071a36e8b05f5b1e27e

WORKDIR $SRC 
RUN git clone --depth 1 -b n6.0 https://git.ffmpeg.org/ffmpeg.git ffmpeg

RUN wget https://www.alsa-project.org/files/pub/lib/alsa-lib-1.1.0.tar.bz2
RUN git clone --depth 1 -b v2.0.3 https://github.com/mstorsjo/fdk-aac.git
RUN git clone --depth 1 -b 2.21.0 https://github.com/intel/libva
RUN git clone --depth 1 -b libvdpau-1.2 git://people.freedesktop.org/~aplattner/libvdpau
RUN git clone --depth 1 -b v1.14.0 https://chromium.googlesource.com/webm/libvpx
RUN git clone --depth 1 -b v1.3.5 https://gitlab.xiph.org/xiph/ogg.git
RUN git clone --depth 1 -b v1.5.2 https://gitlab.xiph.org/xiph/opus.git
RUN git clone --depth 1 https://gitlab.xiph.org/xiph/theora.git
RUN git clone --depth 1 -b v1.3.7 https://gitlab.xiph.org/xiph/vorbis.git
RUN git clone --depth 1 -b v2.11.8 https://gitlab.gnome.org/GNOME/libxml2.git
RUN git clone --depth 1 -b 1.0.6 https://github.com/intel/cmrt 
RUN git clone --depth 1 -b intel-gmmlib-22.3.19 https://github.com/intel/gmmlib.git
RUN git clone --depth 1 -b intel-media-24.1.5 https://github.com/intel/media-driver.git
RUN git clone --depth 1 https://github.com/intel/libva-utils.git 
RUN git clone --depth 1 -b v1.5.3 https://github.com/Haivision/srt 
RUN git clone --depth 1 -b 3.6 https://bitbucket.org/multicoreware/x265_git.git x265

RUN sed -i '/^#\sdeb-src /s/^#//' "/etc/apt/sources.list" && \
    apt-get update && \
    apt-get install -y build-essential fakeroot dpkg-dev devscripts && \
    apt-get install -y mesa-va-drivers && \
    apt-get install -y lsb-release wget software-properties-common gnupg && \
    apt-get install -y autoconf libtool libdrm-dev xorg xorg-dev openbox libx11-dev libgl1-mesa-glx
ENV APT_PACKAGES="ffmpeg intel-media-driver alsa-lib libcmrt libogg opus libtheora libva libva-utils libvdpau libvorbis libvpx libxml2 x265"
RUN apt-get build-dep -y $APT_PACKAGES
RUN curl -LO http://mirrors.kernel.org/ubuntu/pool/main/a/automake-1.16/automake_1.16.5-1.3_all.deb && \
    apt-get install -y ./automake_1.16.5-1.3_all.deb
RUN apt-get install -y tclsh
RUN apt-get purge -y --autoremove libva2 'libva-*'
RUN apt-get install -y libxcb-dri3-dev libx11-xcb-dev
RUN rm -rf /var/lib/apt/lists/*

COPY build_deps_repro.sh $SRC/build_deps.sh
RUN chmod +x build_deps.sh  
RUN ./build_deps.sh
RUN ldconfig

COPY build_repro.sh $SRC/build.sh
RUN chmod +x build.sh 
COPY target_dec_fuzzer.c $SRC/ffmpeg/tools/target_dec_fuzzer.c
COPY tools/Makefile $SRC/ffmpeg/tools/Makefile
COPY Makefile $SRC/ffmpeg/Makefile
RUN ./build.sh
RUN ldconfig

WORKDIR $OUT
RUN ldconfig

# Set environment variables
ENV LIBVA_DRIVER_NAME=iHD

COPY corpus $OUT/corpus
COPY fuzzer_script.sh $OUT/evaluation_script.sh

 
