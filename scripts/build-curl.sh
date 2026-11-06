#!/bin/sh
# libcurl, as the Open SDK has it: DalsinAI/gfopencurl's build (HTTP/HTTPS
# through gfopentls.library at run time, nghttp2), soft float, with zlib's
# calls on the system's z.library (build-deps.sh's libz.a). The only
# library KiCad links that is built from fetched upstream source: fonts are
# gfopenfont.library's, drawing gfopengpu.library's, zlib z.library's.
#   TC=<compiler prefix> WORK=<build folder> DEPS=<build-deps.sh's prefix>
#   [NDK=<OpenNDK Include_H, when the compiler has none>] [UPSTREAM_TARBALLS=<folder
#   holding the upstream tarballs, used before downloading>] ./build-curl.sh
# Result: $WORK/curl/{include,lib}
# Copyright (c) 2026 AmiCradle Team
set -eu
HERE=$(cd "$(dirname "$0")/.." && pwd)
. "$HERE/deps.lock"
TC=${TC:?}; WORK=${WORK:?}; DEPS=${DEPS:?}; NDK=${NDK:-}
DL=${DL:-$WORK/dl}; JOBS=${JOBS:-$(nproc)}
OUT=$WORK/curl; SRC=$WORK/curl-src
mkdir -p "$DL" "$OUT/include" "$OUT/lib" "$SRC"
CPU="-m68040 -noixemul${NDK:+ -idirafter $NDK}"
# gfopencurl's script takes the compiler root (holding prefix/) and CPU flags
export OS32_GCC16="$(dirname "$TC")" OS32_CPU_FLAGS="$CPU" JOBS
[ "$OS32_GCC16/prefix" = "$TC" ] || { echo "TC must be <root>/prefix" >&2; exit 1; }
log() { echo "curl: $*"; }

fetch() { # url sha256 -> $DL/<name>
    f="$DL/$(basename "$1")"
    if [ -n "${UPSTREAM_TARBALLS:-}" ] && [ -f "$UPSTREAM_TARBALLS/$(basename "$f")" ] && [ ! -f "$f" ]; then
        cp "$UPSTREAM_TARBALLS/$(basename "$f")" "$f"
    fi
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
}

MARK="$GFOPENCURL_COMMIT-zlibrary"
if [ "$(cat "$OUT/.curl" 2>/dev/null || true)" != "$MARK" ]; then
    port gfopentls "$GFOPENTLS_COMMIT"
    port gfopencurl "$GFOPENCURL_COMMIT"; d="$SRC/gfopencurl"
    rm -rf "$d/work" "$d/tarballs"; mkdir -p "$d/tarballs"
    fetch "$PLAT_CURL_URL" "$PLAT_CURL_SHA256"; cp "$DL/$(basename "$PLAT_CURL_URL")" "$d/tarballs/"
    fetch "$PLAT_NGHTTP2_URL" "$PLAT_NGHTTP2_SHA256"; cp "$DL/$(basename "$PLAT_NGHTTP2_URL")" "$d/tarballs/"
    # the port's own smoke test links hard float maths (no -lm), which a
    # soft-float link can't resolve; the library is made before it
    PREFIX="$OUT" TARBALLS="$d/tarballs" DEPS_PREFIX="$DEPS" OPENTLS_INCLUDE="$SRC/gfopentls/include" sh "$d/build.sh" \
        || [ "$d/work/curl/build/lib/libcurl.a" -nt "$d/tarballs" ] || exit 1
    [ -f "$OUT/lib/libcurl.a" ] || cp "$d/work/curl/build/lib/libcurl.a" "$OUT/lib/"
    echo "$MARK" > "$OUT/.curl"; log "done"
fi
