#!/bin/sh
# Builds the libraries KiCad needs that the platform does not supply yet,
# as static m68k-amigaos libraries in $DEPS (include/, lib/), plus the host
# protoc. Each part is skipped when its stamp matches the lock.
#   TC=<compiler prefix> WORK=<build folder> DL=<download cache> ./build-deps.sh
# Copyright (c) 2026 AmiCradle Team
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
. "$HERE/deps.lock"
TC=${TC:?TC is the compiler prefix}
WORK=${WORK:?}
DL=${DL:-$WORK/dl}
JOBS=${JOBS:-$(nproc)}
DEPS=$WORK/deps
HOST=$WORK/host
mkdir -p "$DL" "$DEPS/include" "$DEPS/lib" "$WORK/src" "$HOST"
export PATH="$TC/bin:$PATH"
export STOVE_PREFIX="$TC" DEPS_PREFIX="$DEPS"
CC=m68k-amigaos-gcc; CXX=m68k-amigaos-g++; AR=m68k-amigaos-ar
OPT="-noixemul -m68040 -O2 -fno-delete-null-pointer-checks"
TOOLCHAIN="$HERE/cmake/m68k-amigaos.cmake"

log() { echo "deps: $*"; }

# our own small headers libnix lacks (endian.h)
cp "$HERE"/compat/include/*.h "$DEPS/include/"

# the POSIX calls libgit2 needs that libnix lacks (pread, pwrite, getpgid, getsid)
mkdir -p "$WORK/obj/compat"
$CC $OPT -include "$HERE/compat/include/gfamiga_prefix.h" -c -o "$WORK/obj/compat/gfamiga_posix.o" "$HERE/compat/src/gfamiga_posix.c"
rm -f "$DEPS/lib/libgfamiga_compat.a"; $AR rcs "$DEPS/lib/libgfamiga_compat.a" "$WORK/obj/compat/gfamiga_posix.o"

fetch() { # name url sha256 -> $DL/<file>
    f="$DL/$(basename "$2")"
    if [ ! -f "$f" ] || [ "$(sha256sum "$f" | cut -d' ' -f1)" != "$3" ]; then
        log "fetch $1"
        curl -fsSL --retry 3 -o "$f.part" "$2"
        mv "$f.part" "$f"
    fi
    [ "$(sha256sum "$f" | cut -d' ' -f1)" = "$3" ] || { echo "deps: $1 sha256 mismatch" >&2; exit 1; }
    echo "$f"
}

gitfetch() { # name repo commit -> $WORK/src/<name>
    d="$WORK/src/$1"
    if [ "$(git -C "$d" rev-parse HEAD 2>/dev/null || true)" != "$3" ]; then
        rm -rf "$d"; mkdir -p "$d"
        git -C "$d" init -q; git -C "$d" fetch -q --depth 1 "$2" "$3"; git -C "$d" checkout -q FETCH_HEAD
    fi
    echo "$d"
}

unpack() { # tarball -> $WORK/src/<topdir>
    top=$(tar tf "$1" | head -1 | cut -d/ -f1)
    [ -d "$WORK/src/$top" ] || tar xf "$1" -C "$WORK/src"
    echo "$WORK/src/$top"
}

patch_tree() { # name dir: apply patches/<name>/*.patch once
    [ -d "$HERE/patches/$1" ] || return 0
    [ -f "$2/.gfamiga-patched" ] && return 0
    for p in "$HERE/patches/$1"/*.patch; do
        patch -d "$2" -p1 -s --forward < "$p" || { echo "deps: $p does not apply" >&2; exit 1; }
    done
    touch "$2/.gfamiga-patched"
}

done_for() { [ "$(cat "$DEPS/.$1" 2>/dev/null || true)" = "$2" ]; }
mark() { echo "$2" > "$DEPS/.$1"; }

# ---- OpenNDK: the AmigaOS and socket headers. A stove has them as the
# compiler's ndk-include; the GFOpenAmigaGCC release alone does not.
NDK=${NDK:-}
if [ -z "$NDK" ]; then
    if [ -d "$TC/m68k-amigaos/ndk-include/sys" ] && [ -f "$TC/m68k-amigaos/ndk-include/sys/socket.h" ]; then
        NDK="$TC/m68k-amigaos/ndk-include"
    else
        NDK="$(gitfetch opensdk "$OPENSDK_REPO" "$OPENSDK_COMMIT")/OpenNDK/Include_H"
    fi
fi
export KC_NDK="$NDK"
log "NDK: $NDK"

# ---- zstd: libzstd.a
if ! done_for zstd "$ZSTD_SHA256"; then
    S=$(unpack "$(fetch zstd "$ZSTD_URL" "$ZSTD_SHA256")")
    make -C "$S/lib" -j"$JOBS" CC=$CC AR=$AR CFLAGS="$OPT -DZSTD_MULTITHREAD=0 -DZSTD_NO_ASM" libzstd.a >/dev/null
    cp "$S/lib/libzstd.a" "$DEPS/lib/"
    cp "$S/lib/zstd.h" "$S/lib/zstd_errors.h" "$S/lib/zdict.h" "$DEPS/include/"
    make -C "$S/lib" clean >/dev/null
    mark zstd "$ZSTD_SHA256"; log "zstd done"
fi

# ---- GLM: headers only
if ! done_for glm "$GLM_COMMIT"; then
    S=$(gitfetch glm "$GLM_REPO" "$GLM_COMMIT")
    rm -rf "$DEPS/include/glm"; cp -r "$S/glm" "$DEPS/include/glm"
    find "$DEPS/include/glm" -name '*.cpp' -delete; rm -f "$DEPS/include/glm/CMakeLists.txt"
    mark glm "$GLM_COMMIT"; log "glm done"
fi

# ---- ngspice: only sharedspice.h at build time (KiCad loads the library when the simulator starts)
if ! done_for ngspice "$NGSPICE_SHA256"; then
    S=$(unpack "$(fetch ngspice "$NGSPICE_URL" "$NGSPICE_SHA256")")
    mkdir -p "$DEPS/include/ngspice"; cp "$S/src/include/ngspice/sharedspice.h" "$DEPS/include/ngspice/"
    mark ngspice "$NGSPICE_SHA256"; log "ngspice header done"
fi

# ---- Boost: headers, and Boost.Locale on its std backend (no ICU, no iconv)
if ! done_for boost "$BOOST_SHA256"; then
    S=$(unpack "$(fetch boost "$BOOST_URL" "$BOOST_SHA256")")
    rm -rf "$DEPS/include/boost"
    # the b2 release keeps each library's headers in libs/*/include
    for d in "$S"/libs/*/include "$S"/libs/numeric/*/include; do
        [ -d "$d/boost" ] && cp -r "$d/boost" "$DEPS/include/" 2>/dev/null || true
    done
    [ -d "$S/boost" ] && cp -r "$S/boost" "$DEPS/include/" || true
    # libpthread's condition variables time out on the real-time clock and it has
    # no pthread_condattr_setclock(), so Boost.Thread must not take CLOCK_MONOTONIC
    sed -i 's|^  #if defined(CLOCK_MONOTONIC)$|  #if defined(CLOCK_MONOTONIC) \&\& !defined(__amigaos__)|' \
        "$DEPS/include/boost/thread/detail/config.hpp"
    grep -q 'CLOCK_MONOTONIC) && !defined(__amigaos__)' "$DEPS/include/boost/thread/detail/config.hpp"
    BCFG="-DBOOST_PLATFORM_CONFIG=<$HERE/compat/boost_amiga_platform.hpp>"
    L="$S/libs/locale/src"; O="$WORK/obj/boost_locale"; rm -rf "$O"; mkdir -p "$O"
    SRCS=$(cd "$L" && ls encoding/codepage.cpp shared/*.cpp std/*.cpp util/*.cpp | grep -v iconv)
    for s in $SRCS; do
        $CXX $OPT -std=gnu++17 -DBOOST_LOCALE_NO_POSIX_BACKEND -DBOOST_LOCALE_NO_WINAPI_BACKEND \
            "$BCFG" -DBOOST_LOCALE_STATIC_LINK -DBOOST_LOCALE_SOURCE -DBOOST_LOCALE_WITH_ICONV=0 -DBOOST_THREAD_USE_LIB \
            -I"$DEPS/include" -I"$L" -c "$L/$s" -o "$O/$(echo "$s" | tr / _).o" &
        while [ "$(jobs -p | wc -l)" -ge "$JOBS" ]; do sleep 1; done
    done
    wait
    [ "$(ls "$O" | wc -l)" -eq "$(echo "$SRCS" | wc -w)" ] || { echo "deps: Boost.Locale did not compile" >&2; exit 1; }
    rm -f "$DEPS/lib/libboost_locale.a"; $AR rcs "$DEPS/lib/libboost_locale.a" "$O"/*.o
    mark boost "$BOOST_SHA256"; log "boost done ($(ls "$O" | wc -l) locale objects)"
fi

# ---- protobuf 3.21 (the last without Abseil): host protoc and the m68k library
if ! done_for protobuf "$PROTOBUF_SHA256"; then
    S=$(unpack "$(fetch protobuf "$PROTOBUF_URL" "$PROTOBUF_SHA256")"); patch_tree protobuf "$S"
    cmake -S "$S" -B "$WORK/obj/protoc-host" -G Ninja -DCMAKE_BUILD_TYPE=Release \
        -Dprotobuf_BUILD_TESTS=OFF -Dprotobuf_WITH_ZLIB=OFF -DCMAKE_INSTALL_PREFIX="$HOST" >/dev/null
    cmake --build "$WORK/obj/protoc-host" -j"$JOBS" --target protoc >/dev/null
    mkdir -p "$HOST/bin"; cp "$WORK/obj/protoc-host/protoc" "$HOST/bin/"
    # protoc finds the well-known .proto files (any.proto ...) in ../include
    ln -sfn "$DEPS/include" "$HOST/include"
    cmake -S "$S" -B "$WORK/obj/protobuf" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS_RELEASE="-O2 -I$DEPS/include" -Dprotobuf_BUILD_TESTS=OFF \
        -Dprotobuf_BUILD_PROTOC_BINARIES=OFF -Dprotobuf_BUILD_SHARED_LIBS=OFF \
        -Dprotobuf_WITH_ZLIB=OFF -DCMAKE_INSTALL_PREFIX="$DEPS" >/dev/null
    cmake --build "$WORK/obj/protobuf" -j"$JOBS" --target libprotobuf >/dev/null
    cmake --install "$WORK/obj/protobuf" >/dev/null 2>&1 || {
        cp "$WORK/obj/protobuf/libprotobuf.a" "$DEPS/lib/"
        (cd "$S/src" && find google \( -name '*.h' -o -name '*.inc' -o -name '*.proto' \) -print | tar cf - -T -) | tar xf - -C "$DEPS/include"; }
    mark protobuf "$PROTOBUF_SHA256"; log "protobuf done"
fi

# ---- libgit2: local repositories only (no HTTPS, no SSH)
if ! done_for libgit2 "$LIBGIT2_COMMIT-o2"; then
    S=$(gitfetch libgit2 "$LIBGIT2_REPO" "$LIBGIT2_COMMIT"); patch_tree libgit2 "$S"
    cmake -S "$S" -B "$WORK/obj/libgit2" -G Ninja -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS_RELEASE="-O2 -DNO_MMAP" -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTS=OFF -DCMAKE_C_STANDARD=99 -DCMAKE_C_EXTENSIONS=ON \
        -DBUILD_CLI=OFF -DUSE_HTTPS=OFF -DUSE_SSH=OFF -DUSE_GSSAPI=OFF -DUSE_NTLMCLIENT=OFF \
        -DREGEX_BACKEND=builtin -DUSE_HTTP_PARSER=builtin -DUSE_BUNDLED_ZLIB=ON -DUSE_THREADS=ON \
        -DUSE_SHA1=CollisionDetection -DUSE_SHA256=Builtin -DUSE_ICONV=OFF -DUSE_NSEC=OFF \
        -DGIT_QSORT_GNU=0 -DGIT_RAND_GETENTROPY=0 -DGIT_IO_POLL=0 \
        -DCMAKE_INSTALL_PREFIX="$DEPS" >/dev/null
    cmake --build "$WORK/obj/libgit2" -j"$JOBS" >/dev/null
    cmake --install "$WORK/obj/libgit2" >/dev/null
    mark libgit2 "$LIBGIT2_COMMIT-o2"; log "libgit2 done"
fi

log "all done: $(ls "$DEPS/lib" | tr '\n' ' ')"
