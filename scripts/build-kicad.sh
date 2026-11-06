#!/bin/sh
# Configures and builds KiCad for AmigaOS 3 (68040) with the libraries
# build-deps.sh made, the GFOpenWX Toolkit SDK and the platform libraries.
#   TC=<compiler prefix> WORK=<build folder> WXSDK=<toolkit sdk/> SYSLIBS=<prefix
#   with cairo, pixman, freetype, harfbuzz, fontconfig, curl, zlib> ./build-kicad.sh [target...]
# Copyright (c) 2026 AmiCradle Team
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
. "$HERE/deps.lock"
TC=${TC:?}; WORK=${WORK:?}; WXSDK=${WXSDK:?}; SYSLIBS=${SYSLIBS:?}
DL=${DL:-$WORK/dl}; JOBS=${JOBS:-$(nproc)}
DEPS=$WORK/deps; HOST=$WORK/host
export PATH="$TC/bin:$HOST/bin:$PATH"
export STOVE_PREFIX="$TC" DEPS_PREFIX="$DEPS" KC_ROOTS="$SYSLIBS;$WXSDK"

# KiCad's source, unpacked and patched once for each set of patches
f="$DL/kicad-$KICAD_VERSION.tar.gz"
if [ ! -f "$f" ] || [ "$(sha256sum "$f" | cut -d' ' -f1)" != "$KICAD_SHA256" ]; then
    curl -fsSL --retry 3 -o "$f.part" "$KICAD_URL"; mv "$f.part" "$f"
fi
[ "$(sha256sum "$f" | cut -d' ' -f1)" = "$KICAD_SHA256" ] || { echo "kicad: sha256 mismatch" >&2; exit 1; }
PSUM=$(find "$HERE/patches/kicad" "$HERE/kicad" -type f | sort | xargs cat | sha256sum | cut -c1-16)
SRC="$WORK/kicad-src"
if [ "$(cat "$SRC/.gfamiga-patches" 2>/dev/null || true)" != "$PSUM" ]; then
    echo "kicad: unpacking and patching ($PSUM)"
    rm -rf "$SRC"; mkdir -p "$SRC"
    tar xzf "$f" -C "$SRC" --strip-components=1 \
        --exclude='*/qa/data' --exclude='*/demos'
    for p in "$HERE"/patches/kicad/*.patch; do
        patch -d "$SRC" -p1 -s --forward < "$p" || { echo "kicad: $p does not apply" >&2; exit 1; }
    done
    # our own files KiCad builds with (stubs for parts not ported yet)
    cp -r "$HERE/kicad/." "$SRC/amiga/"
    echo "$PSUM" > "$SRC/.gfamiga-patches"
fi

B="$WORK/kicad-build"
if [ ! -f "$B/build.ninja" ] || [ "${RECONFIGURE:-0}" = 1 ]; then
    cmake -S "$SRC" -B "$B" -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$HERE/cmake/m68k-amigaos.cmake" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG" -DCMAKE_C_FLAGS_RELEASE="-O2 -DNDEBUG" \
        -DCMAKE_PREFIX_PATH="/" \
        -DwxWidgets_CONFIG_EXECUTABLE="$WXSDK/bin/wx-config" \
        -DKICAD_AMIGA=ON -DKICAD_WAYLAND=OFF \
        -DCAIRO_INCLUDE_DIR="$SYSLIBS/include/cairo" -DCAIRO_LIBRARY="$SYSLIBS/lib/libcairo.a" \
        -DPIXMAN_INCLUDE_DIR="$SYSLIBS/include/pixman-1" -DPIXMAN_LIBRARY="$SYSLIBS/lib/libpixman-1.a" \
        -DFontconfig_INCLUDE_DIR="$SYSLIBS/include" -DFontconfig_LIBRARY="$SYSLIBS/lib/libfontconfig.a" \
        -DKICAD_SCRIPTING_WXPYTHON=OFF -DKICAD_IPC_API=OFF -DKICAD_BUILD_QA_TESTS=OFF \
        -DKICAD_BUILD_I18N=OFF -DKICAD_SPICE_QA=OFF -DKICAD_USE_SENTRY=OFF -DKICAD_UPDATE_CHECK=OFF \
        -DKICAD_USE_PCH=OFF -DKICAD_INSTALL_DEMOS=OFF -DKICAD_SIGNAL_INTEGRITY=OFF -DKICAD_IDF_TOOLS=OFF \
        -DProtobuf_PROTOC_EXECUTABLE="$HOST/bin/protoc" -DProtobuf_INCLUDE_DIR="$DEPS/include" \
        -DProtobuf_LIBRARY="$DEPS/lib/libprotobuf.a" -DKICAD_USE_CMAKE_FINDPROTOBUF=ON \
        -DNGSPICE_INCLUDE_DIR="$DEPS/include" -DNGSPICE_LIBRARY="$DEPS/include/ngspice/sharedspice.h" \
        -DBoost_INCLUDE_DIR="$DEPS/include" -DBoost_LOCALE_LIBRARY_RELEASE="$DEPS/lib/libboost_locale.a" \
        -DBoost_NO_SYSTEM_PATHS=ON -DBoost_USE_STATIC_LIBS=ON \
        -DGLM_INCLUDE_DIR="$DEPS/include" \
        -DZSTD_INCLUDE_DIR="$DEPS/include" -DZSTD_LIBRARY="$DEPS/lib/libzstd.a" \
        -DLIBGIT2_INCLUDE_DIR="$DEPS/include" -DLIBGIT2_LIBRARY="$DEPS/lib/libgit2.a" \
        -DCMAKE_INSTALL_PREFIX="$WORK/kicad-install" \
        > "$WORK/kicad-configure.log" 2>&1 || { tail -40 "$WORK/kicad-configure.log"; exit 1; }
fi
cmake --build "$B" -j"$JOBS" -- -k 0 "$@"
