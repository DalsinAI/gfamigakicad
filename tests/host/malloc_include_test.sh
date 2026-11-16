#!/bin/sh
# Checks that build-deps.sh compiles gfamiga_malloc.c with the size header
# on its include path.
#   sh tests/host/malloc_include_test.sh
# Copyright (c) 2026 AmiCradle Team. MIT.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
fails=0
say() { printf '%s %s\n' "$1" "$2"; }

if grep -q 'gfamiga_malloc.c' "$ROOT/scripts/build-deps.sh" \
   && grep 'gfamiga_malloc.c' "$ROOT/scripts/build-deps.sh" | grep -q 'compat/include'; then
    say ok "build-deps.sh compiles malloc with -I compat/include"
else
    say FAIL "build-deps.sh does not put compat/include on the malloc compile line"
    fails=$((fails + 1))
fi

if grep -q 'gfamiga_malloc_size.h' "$ROOT/compat/src/gfamiga_malloc.c"; then
    say ok "gfamiga_malloc.c includes the size header"
else
    say FAIL "gfamiga_malloc.c does not include gfamiga_malloc_size.h"
    fails=$((fails + 1))
fi

if [ "$fails" -ne 0 ]; then
    echo FAILED
    exit 1
fi
echo all ok
