#!/bin/sh
# Checks the NDK workarounds applied to the toolkit's wx tree before make.
#   sh tests/host/ndk_wx_fixes_test.sh
# Copyright (c) 2026 AmiCradle Team. MIT.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
FIX="$ROOT/scripts/amiga-wx-ndk-fixes.sh"
fails=0
say() { printf '%s %s\n' "$1" "$2"; }

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/src/common" "$tmp/build"

cat > "$tmp/src/common/webrequest_curl.cpp" << 'SRC'
#ifdef __WINDOWS__
    #include "wx/hashset.h"
    #include "wx/msw/wrapwin.h"
#elif defined(__AMIGA__)
    #include "wx/evtloopsrc.h"
    #include "wx/evtloop.h"
    // curl's sockets are bsdsocket.library's, on the C library's SocketBase
    #include <errno.h>
    #include <proto/bsdsocket.h>
    #include <amitcp/socketbasetags.h>
#else
    #include "wx/evtloopsrc.h"
    #include "wx/evtloop.h"
#endif
SRC

printf 'CXXFLAGS = -noixemul -m68040 -Os\nCFLAGS = -noixemul\n' > "$tmp/build/Makefile"

sh "$FIX" "$tmp" "$tmp/build"

if grep -q '#undef m_next' "$tmp/src/common/webrequest_curl.cpp" \
   && grep -q '#undef m_len' "$tmp/src/common/webrequest_curl.cpp"; then
    say ok "webrequest_curl.cpp drops the mbuf aliases"
else
    say FAIL "webrequest_curl.cpp was not patched"
    fails=$((fails + 1))
fi

if grep -q '^CXXFLAGS = -fpermissive ' "$tmp/build/Makefile" \
   && grep -q '^CFLAGS = -noixemul$' "$tmp/build/Makefile"; then
    say ok "Makefile CXXFLAGS got -fpermissive, CFLAGS unchanged"
else
    say FAIL "Makefile flags ($tmp/build/Makefile)"
    fails=$((fails + 1))
fi

# a second run must not duplicate the edits
sh "$FIX" "$tmp" "$tmp/build"
n=$(grep -c '^[[:space:]]*#undef m_next$' "$tmp/src/common/webrequest_curl.cpp")
c=$(grep -c 'fpermissive' "$tmp/build/Makefile")
if [ "$n" = 1 ] && [ "$c" = 1 ]; then
    say ok "the helper is idempotent"
else
    say FAIL "second run duplicated edits (undef=$n fpermissive=$c)"
    fails=$((fails + 1))
fi

if grep -q 'GFAMIGA_NDK_FIXES' "$ROOT/build-amiga.sh" \
   && grep -q 'amiga-wx-ndk-fixes.sh' "$ROOT/build-amiga.sh" \
   && grep -q -- '--hook-toolkit-scripts' "$ROOT/build-amiga.sh"; then
    say ok "build-amiga.sh runs the helper on the fetched toolkit"
else
    say FAIL "build-amiga.sh does not hook the helper"
    fails=$((fails + 1))
fi

# --hook-toolkit-scripts inserts the pre-make call into the toolkit scripts
mkdir -p "$tmp/tk/scripts"
printf '(cd "$B" && nice -n 19 make -j"$JOBS" > make.log 2>&1)\n' > "$tmp/tk/scripts/build-base.sh"
printf '(cd "$G" && nice -n 19 make -k -j"$JOBS" all)\n' > "$tmp/tk/scripts/build-gui.sh"
chmod +x "$tmp/tk/scripts/build-base.sh" "$tmp/tk/scripts/build-gui.sh"
sh "$FIX" --hook-toolkit-scripts "$tmp/tk"
sh "$FIX" --hook-toolkit-scripts "$tmp/tk"
b=$(grep -c 'GFAMIGA_NDK_FIXES:-' "$tmp/tk/scripts/build-base.sh")
g=$(grep -c 'GFAMIGA_NDK_FIXES:-' "$tmp/tk/scripts/build-gui.sh")
if [ "$b" = 1 ] && [ "$g" = 1 ]; then
    say ok "--hook-toolkit-scripts is idempotent on base and gui"
else
    say FAIL "toolkit script hook (base=$b gui=$g)"
    fails=$((fails + 1))
fi

if grep -q 'fpermissive' "$ROOT/cmake/m68k-amigaos.cmake"; then
    say ok "the KiCad toolchain passes -fpermissive to C++"
else
    say FAIL "cmake/m68k-amigaos.cmake has no -fpermissive"
    fails=$((fails + 1))
fi

if [ "$fails" -ne 0 ]; then
    echo FAILED
    exit 1
fi
echo all ok
