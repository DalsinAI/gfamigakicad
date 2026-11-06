#!/bin/sh
# KiCad for AmigaOS 3 (68040, soft float) on a Cradle stove:
#   STOVE=.../stoves/os32-gcc16-open6 OUT=out ./build-amiga.sh
# Everything is built in WORK and kept there, so a stove job that runs out of
# time is simply run again: each step that finished is skipped, and the KiCad
# compile carries on where it stopped. The script stops by itself before the
# stove's limit and says "not finished, run again" (exit 3).
# Copyright (c) 2026 AmiCradle Team
set -eu
HERE=$(cd "$(dirname "$0")" && pwd)
. "$HERE/deps.lock"
START=${GFAMIGA_START:-$(date +%s)}
STOVE=${STOVE:-"$HOME/AmiCradle/stoves/os32-gcc16-open6"}
WORK=${WORK:-"$HOME/AmiCradle-dev/cradle-builds/lab-cache/gfamigakicad"}
# A stove job checks the repo out afresh each run, so every file in it is
# new and ninja would rebuild all of KiCad (the compile flags name the
# prefix header in compat/). Build from a copy in WORK instead, where only
# files whose contents changed get a new time.
if [ "${GFAMIGA_TREE:-}" != 1 ]; then
    T="$WORK/tree"; mkdir -p "$T"
    (cd "$HERE" && find . -path ./.git -prune -o -type f -print) | while read -r f; do
        d="$T/${f#./}"
        cmp -s "$HERE/$f" "$d" 2>/dev/null || { mkdir -p "$(dirname "$d")"; cp "$HERE/$f" "$d"; }
    done
    GFAMIGA_TREE=1 GFAMIGA_START=$START WORK="$WORK" exec sh "$T/build-amiga.sh" "$@"
fi
OUT=${OUT:-"$WORK/out"}; JOBS=${JOBS:-12}
LIMIT=${LIMIT:-840}   # seconds this run may take (the stove allows 900)
# the upstream tarballs kept on the dev machine, used before downloading
UPSTREAM_TARBALLS=${UPSTREAM_TARBALLS:-/run/media/da1ek/WorkingSpace/AmigaChrome/team4/salvor-offload-20261012/dev/upstream-tarballs}
TC="$STOVE/prefix"
mkdir -p "$WORK" "$OUT" "$WORK/tmp"
# the compiler's temporary files go in WORK, not the shared /tmp
export TMPDIR="$WORK/tmp"
export UPSTREAM_TARBALLS JOBS
left() { echo $((LIMIT - ($(date +%s) - START))); }
step() { echo "kicad-amiga: $* ($(( $(date +%s) - START )) s)"; }
again() { step "not finished, run again"; exit 3; }

# ---- the GFOpenWX Toolkit SDK at the pinned commit: the toolkit's own
# lab build of it when this machine has that (deps.lock TOOLKIT_SDK_*),
# else built here
WX="$WORK/wx"
SDKTAR=${TOOLKIT_SDK_TARBALL:-"$HOME/AmiCradle-dev/cradle-builds/jobs/$TOOLKIT_SDK_JOB/out/gfopenwx-sdk.tar.gz"}
if [ "$(cat "$WX/.commit" 2>/dev/null || true)" != "$TOOLKIT_COMMIT" ] && [ -f "$SDKTAR" ] \
   && [ "$(sha256sum "$SDKTAR" | cut -d' ' -f1)" = "$TOOLKIT_SDK_SHA256" ]; then
    step "toolkit SDK from $SDKTAR"
    rm -rf "$WX/out/sdk"; mkdir -p "$WX/out/sdk.tmp"
    tar xzf "$SDKTAR" -C "$WX/out/sdk.tmp"
    top=$(find "$WX/out/sdk.tmp" -name wx-config -path '*/bin/*' | head -1)
    [ -n "$top" ] || { echo "no bin/wx-config in $SDKTAR" >&2; exit 1; }
    mv "$(dirname "$(dirname "$top")")" "$WX/out/sdk"; rm -rf "$WX/out/sdk.tmp"
    echo "$TOOLKIT_COMMIT" > "$WX/.commit"
fi
if [ "$(cat "$WX/.commit" 2>/dev/null || true)" != "$TOOLKIT_COMMIT" ]; then
    step "toolkit $TOOLKIT_COMMIT"
    S="$WORK/src/gfopenwxtoolkit"
    if [ "$(git -C "$S" rev-parse HEAD 2>/dev/null || true)" != "$TOOLKIT_COMMIT" ]; then
        rm -rf "$S"; git init -q "$S"
        git -C "$S" fetch -q --depth 1 "$TOOLKIT_REPO" "$TOOLKIT_COMMIT"; git -C "$S" checkout -q FETCH_HEAD
    fi
    mkdir -p "$WX"
    rc=0
    STOVE="$STOVE" WORK="$WX" OUT="$WX/out" JOBS=$JOBS timeout "$(left)" sh "$S/build-amiga.sh" \
        > "$WORK/toolkit.log" 2>&1 || rc=$?
    [ "$rc" = 124 ] && { tail -8 "$WORK/toolkit.log"; again; }
    [ "$rc" = 0 ] || { tail -30 "$WORK/toolkit.log"; exit 1; }
    echo "$TOOLKIT_COMMIT" > "$WX/.commit"
fi
WXSDK="$WX/out/sdk"
[ "$(left)" -gt 120 ] || again

# ---- Boost.Locale, protobuf, libgit2, zstd, GLM, zlib on z.library and the host tools
step "dependencies"
rc=0; TC="$TC" WORK="$WORK" timeout "$(left)" sh "$HERE/scripts/build-deps.sh" > "$WORK/deps.log" 2>&1 || rc=$?
[ "$rc" = 124 ] && again
[ "$rc" = 0 ] || { tail -30 "$WORK/deps.log"; exit 1; }
[ "$(left)" -gt 120 ] || again

# ---- libcurl (gfopencurl's build; everything else is the system's libraries)
step "curl"
NDK=
[ -d "$TC/m68k-amigaos/ndk-include" ] || NDK="$WORK/src/opensdk/OpenNDK/Include_H"
rc=0; TC="$TC" WORK="$WORK" DEPS="$WORK/deps" NDK="$NDK" timeout "$(left)" \
    sh "$HERE/scripts/build-curl.sh" > "$WORK/curl.log" 2>&1 || rc=$?
[ "$rc" = 124 ] && again
[ "$rc" = 0 ] || { tail -30 "$WORK/curl.log"; exit 1; }
[ "$(left)" -gt 120 ] || again

# ---- KiCad, for what time is left
step "kicad"
rc=0
TC="$TC" WORK="$WORK" WXSDK="$WXSDK" CURL="$WORK/curl" \
    timeout "$(( $(left) - 30 ))" sh "$HERE/scripts/build-kicad.sh" > "$WORK/kicad.log" 2>&1 || rc=$?
grep -E '^FAILED|error:|undefined reference' "$WORK/kicad.log" | sort -u | head -60 || true
tail -5 "$WORK/kicad.log"
echo "kicad-amiga: ninja reached $(grep -o '^\[[0-9]*/[0-9]*\]' "$WORK/kicad.log" | tail -1)"
[ "$rc" = 124 ] && again
[ "$rc" = 0 ] || exit 1

# ---- the programs
B="$WORK/kicad-build"
for p in kicad/kicad kicad/kicad-cli eeschema/eeschema pcbnew/pcbnew gerbview/gerbview \
         pagelayout_editor/pl_editor bitmap2component/bitmap2component pcb_calculator/pcb_calculator; do
    [ -f "$B/$p" ] && cp "$B/$p" "$OUT/"
done
TC="$TC" BIN="$OUT" VERSION="$KICAD_RELEASE" OUT="$OUT" sh "$HERE/scripts/package.sh"
step "done"
