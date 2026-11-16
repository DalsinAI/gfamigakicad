#!/bin/sh
# Checks that the Amiga build links a no-op wxGLCanvas when the toolkit
# SDK has no libwx_openlayoutu_gl.
#   sh tests/host/wxgl_stub_test.sh
# Copyright (c) 2026 AmiCradle Team. MIT.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
fails=0
say() { printf '%s %s\n' "$1" "$2"; }

if [ -f "$ROOT/compat/src/gfamiga_wxgl_stub.cpp" ] \
   && grep -q 'IsDisplaySupported' "$ROOT/compat/src/gfamiga_wxgl_stub.cpp" \
   && grep -q 'return false' "$ROOT/compat/src/gfamiga_wxgl_stub.cpp"; then
    say ok "the wxGL stub exists and IsDisplaySupported is false"
else
    say FAIL "compat/src/gfamiga_wxgl_stub.cpp is missing or still claims a display"
    fails=$((fails + 1))
fi

if grep -q 'gfamiga_wxgl_stub' "$ROOT/scripts/build-kicad.sh" \
   && grep -q 'libwx_openlayoutu_gl-3.2.a' "$ROOT/scripts/build-kicad.sh" \
   && grep -q 'WXGL_STUB' "$ROOT/scripts/build-kicad.sh"; then
    say ok "build-kicad.sh links the stub when the toolkit GL library is absent"
else
    say FAIL "build-kicad.sh does not hook the wxGL stub"
    fails=$((fails + 1))
fi

if [ "$fails" -ne 0 ]; then
    echo FAILED
    exit 1
fi
echo all ok
