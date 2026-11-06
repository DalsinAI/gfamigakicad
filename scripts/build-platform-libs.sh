#!/bin/sh
# Builds the platform libraries KiCad links, from the Team's own ports, for
# the 68040 with soft float (the browser stack builds them hard float, which
# a soft-float program can't link):
#   zlib, libpng       upstream, same versions as gfopencurl / gfopenimage
#   Expat              the GFOpenWX Toolkit's (its symbols are not prefixed,
#                      so a second copy would clash in the link)
#   FreeType           DalsinAI/gfopenfreetype build.sh
#   Fontconfig         DalsinAI/gfopenfontconfig build.sh
#   HarfBuzz           DalsinAI/gfopenharfbuzz's single-file build (no hb-icu)
#   cairo + pixman     DalsinAI/gfopencairo build.sh (FreeType font backend)
#   libcurl            DalsinAI/gfopencurl build.sh (HTTP/HTTPS, GFOpenTLS)
#
#   TC=<compiler prefix> WORK=<build folder> WXSDK=<toolkit sdk/>
#   NDK=<OpenNDK Include_H> ./build-platform-libs.sh
# Result: $WORK/platform/{include,lib}, the SYSLIBS that build-kicad.sh takes.
# Copyright (c) 2026 AmiCradle Team
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
. "$HERE/deps.lock"
TC=${TC:?}; WORK=${WORK:?}; WXSDK=${WXSDK:?}; NDK=${NDK:?}
DL=${DL:-$WORK/dl}; JOBS=${JOBS:-$(nproc)}
PLAT=$WORK/platform; SRC=$WORK/platform-src
mkdir -p "$DL" "$PLAT/include" "$PLAT/lib" "$SRC"
CC="$TC/bin/m68k-amigaos-gcc"; CXX="$TC/bin/m68k-amigaos-g++"; AR="$TC/bin/m68k-amigaos-ar"
CPU="-m68040 -noixemul -idirafter $NDK"
CFLAGS="-O2 $CPU -fno-delete-null-pointer-checks -D_DEFAULT_SOURCE=1 -fno-common"
# the ports' scripts take the compiler root (holding prefix/) and CPU flags
export OS32_GCC16="$(dirname "$TC")" OS32_CPU_FLAGS="$CPU" JOBS
[ "$OS32_GCC16/prefix" = "$TC" ] || { echo "TC must be <root>/prefix" >&2; exit 1; }
log() { echo "platform: $*"; }
done_for() { [ "$(cat "$PLAT/.$1" 2>/dev/null || true)" = "$2" ]; }
mark() { echo "$2" > "$PLAT/.$1"; }

fetch() { # url sha256 [name] -> $DL/<name>
    f="$DL/${3:-$(basename "$1")}"
    if [ ! -f "$f" ] || [ "$(sha256sum "$f" | cut -d' ' -f1)" != "$2" ]; then
        curl -fsSL --retry 3 -o "$f.part" "$1"; mv "$f.part" "$f"
    fi
    [ "$(sha256sum "$f" | cut -d' ' -f1)" = "$2" ] || { echo "$f: sha256 mismatch" >&2; exit 1; }
}
port() { # name commit -> $SRC/name at that commit
    d="$SRC/$1"
    if [ "$(git -C "$d" rev-parse HEAD 2>/dev/null || true)" != "$2" ]; then
        rm -rf "$d"; git init -q "$d"
        git -C "$d" fetch -q --depth 1 "https://github.com/DalsinAI/$1" "$2"
        git -C "$d" checkout -q FETCH_HEAD
    fi
    rm -rf "$d/work" "$d/tarballs"; mkdir -p "$d/tarballs"
}
archive() { # name objdir file... ($XFLAGS added)
    name=$1; obj=$2; shift 2
    rm -rf "$obj"; mkdir -p "$obj"
    for f in "$@"; do
        o="$obj/$(echo "$f" | tr '/' '_' | sed 's/\.[a-z]*$//').o"
        case "$f" in
            *.cc|*.cpp) $CXX $CFLAGS ${XFLAGS:-} -c "$f" -o "$o" ;;
            *) $CC $CFLAGS ${XFLAGS:-} -c "$f" -o "$o" ;;
        esac
    done
    rm -f "$PLAT/lib/lib$name.a"; $AR rcs "$PLAT/lib/lib$name.a" "$obj"/*.o
}

# ---- zlib
if ! done_for zlib "$PLAT_ZLIB_SHA256"; then
    fetch "$PLAT_ZLIB_URL" "$PLAT_ZLIB_SHA256"
    rm -rf "$SRC/zlib"; mkdir -p "$SRC/zlib"; tar xf "$DL/$(basename "$PLAT_ZLIB_URL")" -C "$SRC/zlib" --strip-components=1
    (cd "$SRC/zlib" && XFLAGS="-DHAVE_UNISTD_H -DHAVE_STDARG_H" archive z "$SRC/zlib/obj" \
        adler32.c compress.c crc32.c deflate.c gzclose.c gzlib.c gzread.c gzwrite.c infback.c \
        inffast.c inflate.c inftrees.c trees.c uncompr.c zutil.c)
    cp "$SRC/zlib/zlib.h" "$SRC/zlib/zconf.h" "$PLAT/include/"
    mark zlib "$PLAT_ZLIB_SHA256"; log "zlib done"
fi

# ---- libpng
if ! done_for png "$PLAT_PNG_SHA256"; then
    fetch "$PLAT_PNG_URL" "$PLAT_PNG_SHA256"
    rm -rf "$SRC/png"; mkdir -p "$SRC/png"; tar xf "$DL/$(basename "$PLAT_PNG_URL")" -C "$SRC/png" --strip-components=1
    cp "$SRC/png/scripts/pnglibconf.h.prebuilt" "$SRC/png/pnglibconf.h"
    (cd "$SRC/png" && XFLAGS="-I$PLAT/include -DPNG_ARM_NEON_OPT=0" archive png "$SRC/png/obj" \
        png.c pngerror.c pngget.c pngmem.c pngpread.c pngread.c pngrio.c pngrtran.c pngrutil.c \
        pngset.c pngtrans.c pngwio.c pngwrite.c pngwtran.c pngwutil.c)
    cp "$SRC/png/png.h" "$SRC/png/pngconf.h" "$SRC/png/pnglibconf.h" "$PLAT/include/"
    mark png "$PLAT_PNG_SHA256"; log "libpng done"
fi

# ---- Expat: the toolkit's own copy and headers
WXSRC=${WXSRC:?WXSRC is the wxWidgets source the toolkit SDK was built from}
cp "$WXSRC/src/expat/expat/lib/expat.h" "$WXSRC/src/expat/expat/lib/expat_external.h" "$PLAT/include/"
cp "$WXSDK/lib/libwxexpat-3.2.a" "$PLAT/lib/libexpat.a"

# ---- FreeType
if ! done_for freetype "$GFOPENFREETYPE_COMMIT"; then
    port gfopenfreetype "$GFOPENFREETYPE_COMMIT"; d="$SRC/gfopenfreetype"
    fetch "$PLAT_FREETYPE_URL" "$PLAT_FREETYPE_SHA256"; cp "$DL/$(basename "$PLAT_FREETYPE_URL")" "$d/tarballs/"
    PREFIX="$PLAT" TARBALLS="$d/tarballs" DEPS_PREFIX="$PLAT" sh "$d/build.sh"
    mark freetype "$GFOPENFREETYPE_COMMIT"; log "freetype done"
fi

# ---- HarfBuzz: gfopenharfbuzz's single-file build with FreeType; KiCad
# doesn't use hb-icu, so its build (which needs ICU) is left out
if ! done_for harfbuzz "$GFOPENHARFBUZZ_COMMIT"; then
    port gfopenharfbuzz "$GFOPENHARFBUZZ_COMMIT"; d="$SRC/gfopenharfbuzz"
    fetch "$PLAT_HARFBUZZ_URL" "$PLAT_HARFBUZZ_SHA256"
    rm -rf "$d/work"; mkdir -p "$d/work"; tar xf "$DL/$(basename "$PLAT_HARFBUZZ_URL")" -C "$d/work" --strip-components=1
    (cd "$d/work" && XFLAGS="-Isrc -I$PLAT/include/freetype2 -DHAVE_FREETYPE=1 -DHAVE_FT_GET_VAR_BLEND_COORDINATES=1 \
        -DHAVE_FT_SET_VAR_BLEND_COORDINATES=1 -DHAVE_FT_DONE_MM_VAR=1 -DHAVE_FT_GET_TRANSFORM=1 -DHAVE_PTHREAD=1 \
        -std=c++17 -fno-exceptions -fno-rtti -fno-threadsafe-statics" archive harfbuzz "$d/work/obj" src/harfbuzz.cc
     mkdir -p "$PLAT/include/harfbuzz"
     for h in $(sed -n '/^for h in/,/; do$/p' "$d/build.sh" | tr -d '\\' | sed 's/for h in//; s/; do//'); do
         [ "$h" = hb-icu.h ] || cp "src/$h" "$PLAT/include/harfbuzz/"
     done)
    mark harfbuzz "$GFOPENHARFBUZZ_COMMIT"; log "harfbuzz done"
fi

# ---- libcurl (opens gfopentls.library at run time; needs only its headers)
if ! done_for curl "$GFOPENCURL_COMMIT"; then
    port gfopentls "$GFOPENTLS_COMMIT"
    port gfopencurl "$GFOPENCURL_COMMIT"; d="$SRC/gfopencurl"
    fetch "$PLAT_CURL_URL" "$PLAT_CURL_SHA256"; cp "$DL/$(basename "$PLAT_CURL_URL")" "$d/tarballs/"
    fetch "$PLAT_NGHTTP2_URL" "$PLAT_NGHTTP2_SHA256"; cp "$DL/$(basename "$PLAT_NGHTTP2_URL")" "$d/tarballs/"
    # the port's own smoke test links hard float maths (no -lm), which a
    # soft-float link can't resolve; the library is copied out before it
    PREFIX="$PLAT" TARBALLS="$d/tarballs" DEPS_PREFIX="$PLAT" OPENTLS_INCLUDE="$SRC/gfopentls/include" sh "$d/build.sh" \
        || [ "$d/work/curl/build/lib/libcurl.a" -nt "$d/tarballs" ] || exit 1
    mark curl "$GFOPENCURL_COMMIT"; log "curl done"
fi
# ---- Fontconfig
if ! done_for fontconfig "$GFOPENFONTCONFIG_COMMIT"; then
    port gfopenfontconfig "$GFOPENFONTCONFIG_COMMIT"; d="$SRC/gfopenfontconfig"
    fetch "$PLAT_FONTCONFIG_URL" "$PLAT_FONTCONFIG_SHA256"; cp "$DL/$(basename "$PLAT_FONTCONFIG_URL")" "$d/tarballs/"
    PREFIX="$PLAT" TARBALLS="$d/tarballs" DEPS_PREFIX="$PLAT" sh "$d/build.sh"
    mark fontconfig "$GFOPENFONTCONFIG_COMMIT"; log "fontconfig done"
fi

# ---- cairo + pixman
if ! done_for cairo "$GFOPENCAIRO_COMMIT"; then
    port gfopencairo "$GFOPENCAIRO_COMMIT"; d="$SRC/gfopencairo"
    fetch "$PLAT_CAIRO_URL" "$PLAT_CAIRO_SHA256"; cp "$DL/$(basename "$PLAT_CAIRO_URL")" "$d/tarballs/"
    fetch "$PLAT_PIXMAN_URL" "$PLAT_PIXMAN_SHA256"; cp "$DL/$(basename "$PLAT_PIXMAN_URL")" "$d/tarballs/"
    PREFIX="$PLAT" TARBALLS="$d/tarballs" DEPS_PREFIX="$PLAT" CAIRO_FONTS=ft sh "$d/build.sh"
    mark cairo "$GFOPENCAIRO_COMMIT"; log "cairo done"
fi

log "all done: $(ls "$PLAT/lib" | tr '\n' ' ')"
