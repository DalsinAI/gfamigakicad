#!/bin/sh
# Configures and builds KiCad for AmigaOS 3 (68040) with the libraries
# build-deps.sh made (zlib on z.library among them), the GFOpenWX Toolkit SDK
# and build-curl.sh's libcurl. Fonts are gfopenfont.library's and the canvas
# draws through gfopengpu.library, both opened at run time.
#   TC=<compiler prefix> WORK=<build folder> WXSDK=<toolkit sdk/> CURL=<build-curl.sh's
#   prefix> ./build-kicad.sh [target...]
# Copyright (c) 2026 AmiCradle Team
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
. "$HERE/deps.lock"
TC=${TC:?}; WORK=${WORK:?}; WXSDK=${WXSDK:?}; CURL=${CURL:?}
DL=${DL:-$WORK/dl}; JOBS=${JOBS:-$(nproc)}
DEPS=$WORK/deps; HOST=$WORK/host
export PATH="$TC/bin:$HOST/bin:$PATH"
export STOVE_PREFIX="$TC" DEPS_PREFIX="$DEPS" KC_ROOTS="$CURL;$DEPS;$WXSDK"

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
    NEW="$WORK/kicad-src.new"
    rm -rf "$NEW"; mkdir -p "$NEW"
    tar xzf "$f" -C "$NEW" --strip-components=1 \
        --exclude='*/qa/data' --exclude='*/demos'
    for p in "$HERE"/patches/kicad/*.patch; do
        patch -d "$NEW" -p1 -s --forward < "$p" || { echo "kicad: $p does not apply" >&2; exit 1; }
    done
    # our own files KiCad builds with (stubs for parts not ported yet)
    cp -r "$HERE/kicad/." "$NEW/amiga/"
    if [ -d "$SRC" ] && command -v rsync >/dev/null 2>&1; then
        # only files whose contents changed get a new time, so ninja rebuilds
        # just what the new patches touch
        rsync -rc --delete --exclude=.gfamiga-patches "$NEW/" "$SRC/"
        rm -rf "$NEW"
    else
        rm -rf "$SRC"; mv "$NEW" "$SRC"
    fi
    echo "$PSUM" > "$SRC/.gfamiga-patches"
fi

# lemon (KiCad's parser generator) runs at build time: build it for the host
if [ ! -x "$HOST/bin/lemon" ] || [ "$SRC/thirdparty/lemon/lemon.c" -nt "$HOST/bin/lemon" ]; then
    mkdir -p "$HOST/bin"; ${HOSTCC:-cc} -O2 -o "$HOST/bin/lemon" "$SRC/thirdparty/lemon/lemon.c"
fi

B="$WORK/kicad-build"
# configured again when the tree it was configured from moves, or this
# script (its configure line) or the toolkit changes
HERE_STAMP="$HERE $(sha256sum "$0" | cut -d' ' -f1) $TOOLKIT_COMMIT"
if [ ! -f "$B/build.ninja" ] || [ "${RECONFIGURE:-0}" = 1 ] \
   || [ "$(cat "$B/.gfamiga-here" 2>/dev/null || true)" != "$HERE_STAMP" ]; then
    cmake -S "$SRC" -B "$B" -G Ninja \
        -DCMAKE_TOOLCHAIN_FILE="$HERE/cmake/m68k-amigaos.cmake" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS_RELEASE="-O2 -DNDEBUG" -DCMAKE_C_FLAGS_RELEASE="-O2 -DNDEBUG" \
        -DCMAKE_PREFIX_PATH="/" \
        -DwxWidgets_CONFIG_EXECUTABLE="$WXSDK/bin/wx-config" \
        -DKC_EXTRA_LIBS="-Wl,--wrap=open -Wl,--wrap=read -Wl,--wrap=close $DEPS/lib/gfamiga_random.o -Wl,--start-group -L$CURL/lib -lcurl -lnghttp2 -L$DEPS/lib -lz -lgfamiga_compat -Wl,--end-group" -DZLIB_INCLUDE_DIR="$DEPS/include" -DZLIB_LIBRARY="$DEPS/lib/libz.a" -DCURL_INCLUDE_DIR="$CURL/include" -DCURL_LIBRARY="$CURL/lib/libcurl.a" -DKICAD_AMIGA=ON -DKICAD_AMIGA_SDK_INCLUDE="$WORK/src/opensdk/Include" -DKICAD_WAYLAND=OFF -DKICAD_HOST_LEMON="$HOST/bin/lemon" \
        -DKICAD_SCRIPTING_WXPYTHON=OFF -DKICAD_IPC_API=OFF -DKICAD_BUILD_QA_TESTS=OFF \
        -DKICAD_BUILD_I18N=OFF -DKICAD_SPICE_QA=OFF -DKICAD_USE_SENTRY=OFF -DKICAD_UPDATE_CHECK=OFF \
        -DKICAD_USE_PCH=OFF -DKICAD_INSTALL_DEMOS=OFF -DKICAD_SIGNAL_INTEGRITY=OFF \
        -DProtobuf_PROTOC_EXECUTABLE="$HOST/bin/protoc" -DProtobuf_INCLUDE_DIR="$DEPS/include" \
        -DProtobuf_LIBRARY="$DEPS/lib/libprotobuf.a" -DKICAD_USE_CMAKE_FINDPROTOBUF=ON \
        -DNGSPICE_INCLUDE_DIR="$DEPS/include" -DNGSPICE_LIBRARY="$DEPS/lib/libngspice_none.a" \
        -DBoost_INCLUDE_DIR="$DEPS/include" -DBoost_LOCALE_LIBRARY_RELEASE="$DEPS/lib/libboost_locale.a" \
        -DBoost_NO_SYSTEM_PATHS=ON -DBoost_USE_STATIC_LIBS=ON \
        -DGLM_INCLUDE_DIR="$DEPS/include" \
        -DZSTD_INCLUDE_DIR="$DEPS/include" -DZSTD_LIBRARY="$DEPS/lib/libzstd.a" \
        -DLIBGIT2_INCLUDE_DIR="$DEPS/include" -DLIBGIT2_LIBRARY="$DEPS/lib/libgit2.a;$DEPS/lib/libgfamiga_compat.a" \
        -DCMAKE_INSTALL_PREFIX="$WORK/kicad-install" \
        > "$WORK/kicad-configure.log" 2>&1 || { tail -40 "$WORK/kicad-configure.log"; exit 1; }
    echo "$HERE_STAMP" > "$B/.gfamiga-here"
fi
cmake --build "$B" -j"$JOBS" -- -k 0 "$@"
