#!/bin/sh
# Workarounds for this NDK when compiling the GFOpenWX Toolkit's wxWidgets
# tree. Used from build-amiga.sh just before the toolkit's make, so an
# already-configured cache is not thrown away.
#
#   sh scripts/amiga-wx-ndk-fixes.sh WX_SRC [BUILD_DIR ...]
#   sh scripts/amiga-wx-ndk-fixes.sh --hook-toolkit-scripts TOOLKIT_SRC
#
# - Roadshow <netdb.h> uses STRPTR (unsigned char*) for hostent names;
#   POSIX code expects char*. -fpermissive is added to each build
#   directory's CXXFLAGS so those conversions are warnings, not errors.
# - proto/bsdsocket.h includes <sys/mbuf.h>, which aliases m_next to
#   m_hdr.mh_next and breaks wx/hashmap.h members of the same name.
#   The aliases are dropped after that include in webrequest_curl.cpp.
# Copyright (c) 2026 AmiCradle Team. MIT.
set -eu

fix_webrequest() {
    f=$1
    [ -f "$f" ] || return 0
    grep -q 'undef m_next' "$f" && return 0
    python3 - "$f" << 'PY'
import sys
from pathlib import Path
p = Path(sys.argv[1])
t = p.read_text()
old = """    #include <errno.h>
    #include <proto/bsdsocket.h>
    #include <amitcp/socketbasetags.h>
#else
"""
new = """    #include <errno.h>
    #include <proto/bsdsocket.h>
    #include <amitcp/socketbasetags.h>
    /* Roadshow sys/mbuf.h (via proto/bsdsocket.h) aliases m_next to
       m_hdr.mh_next, which breaks wx/hashmap.h members of the same name. */
    #undef m_next
    #undef m_len
    #undef m_data
    #undef m_type
    #undef m_flags
    #undef m_nextpkt
    #undef m_act
#else
"""
if old not in t:
    sys.exit("amiga-wx-ndk-fixes: Amiga include block not found in %s" % p)
p.write_text(t.replace(old, new, 1))
PY
}

fix_makefile() {
    f=$1
    [ -f "$f" ] || return 0
    # bakefile uses CXXFLAGS; the toolkit's GUI Makefile uses CXXF
    for var in CXXFLAGS CXXF; do
        if grep -q "^$var = " "$f" && ! grep -q "^$var = .*fpermissive" "$f"; then
            sed -i "s/^$var = /$var = -fpermissive /" "$f"
        fi
    done
}

hook_toolkit_scripts() {
    dir=$1
    python3 - "$dir" << 'PY'
import sys
from pathlib import Path
root = Path(sys.argv[1])
pairs = [("build-base.sh", "B"), ("build-gui.sh", "G")]
for name, var in pairs:
    p = root / "scripts" / name
    if not p.is_file():
        continue
    t = p.read_text()
    if "GFAMIGA_NDK_FIXES" in t:
        continue
    needle = '(cd "$%s" && nice' % var
    if needle not in t:
        sys.exit("amiga-wx-ndk-fixes: make line not found in %s" % p)
    block = (
        'if [ -n "${GFAMIGA_NDK_FIXES:-}" ]; then\n'
        '    sh "$GFAMIGA_NDK_FIXES" "$SRC" "$%s" || exit 1\n'
        'fi\n' % var
    )
    p.write_text(t.replace(needle, block + needle, 1))
    p.chmod(p.stat().st_mode)
PY
}

if [ "${1:-}" = --hook-toolkit-scripts ]; then
    [ $# -eq 2 ] || { echo "usage: $0 --hook-toolkit-scripts TOOLKIT_SRC" >&2; exit 2; }
    hook_toolkit_scripts "$2"
    exit 0
fi

[ $# -ge 1 ] || { echo "usage: $0 WX_SRC [BUILD_DIR ...]" >&2; exit 2; }
SRC=$1
shift
fix_webrequest "$SRC/src/common/webrequest_curl.cpp"
for b in "$@"; do
    fix_makefile "$b/Makefile"
done
