#!/bin/sh
# Checks the toolkit, curl and Boost pins, and that a change to a patch
# file changes the stamp hash used in scripts/build-deps.sh.
#   sh tests/host/pins_test.sh
# Copyright (c) 2026 AmiCradle Team. MIT.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
. "$ROOT/deps.lock"
fails=0
say() { printf '%s %s\n' "$1" "$2"; }

check() {
    if [ "$2" = "$3" ]; then
        say ok "$1"
    else
        say FAIL "$1 (got $2)"
        fails=$((fails + 1))
    fi
}

check "toolkit is main 2b2db78 (GFOpenLayout UI through gfopenlayout.library)" "$TOOLKIT_COMMIT" "2b2db788892b0379438ae7201821b93789610861"
check "gfopencurl is the A4 TLS-hook fix (07e59ed)" "$GFOPENCURL_COMMIT" "07e59ed451ff325bf8c4772a02ffba0596064920"
check "GFOpenBoost is v1.89.0-rc3" "$GFOPENBOOST_SHA256" "d9ebfc56ed5922c9fc15450366eb0426e9f19a016dd8bbb2fc46da2c99ff3f4b"
case "$GFOPENBOOST_URL" in
    *v1.89.0-rc3*) say ok "GFOpenBoost URL is rc3" ;;
    *) say FAIL "GFOpenBoost URL is not rc3"; fails=$((fails + 1)) ;;
esac

# the same hash helper as build-deps.sh
patch_sum_in() {
    tree=$1
    name=$2
    if [ ! -d "$tree/patches/$name" ]; then
        echo none
        return 0
    fi
    find "$tree/patches/$name" -type f | sort | xargs cat | sha256sum | cut -c1-16
}

a=$(patch_sum_in "$ROOT" protobuf)
b=$(patch_sum_in "$ROOT" libgit2)
c=$(patch_sum_in "$ROOT" nosuch)
if [ "$c" = none ] && [ ${#a} -eq 16 ] && [ ${#b} -eq 16 ] && [ "$a" != "$b" ]; then
    say ok "patch_sum hashes protobuf and libgit2 differently"
else
    say FAIL "patch_sum ($a / $b / $c)"
    fails=$((fails + 1))
fi

# a later edit of the patches must change the stamp (the old presence-only
# stamp would have stayed the same)
tmp=$(mktemp -d)
mkdir -p "$tmp/patches/demo"
printf 'a\n' > "$tmp/patches/demo/0001.patch"
h1=$(patch_sum_in "$tmp" demo)
printf 'b\n' > "$tmp/patches/demo/0001.patch"
h2=$(patch_sum_in "$tmp" demo)
rm -rf "$tmp"
if [ "$h1" != "$h2" ]; then
    say ok "patch_sum changes when a patch file is edited"
else
    say FAIL "patch_sum ignored an edit ($h1)"
    fails=$((fails + 1))
fi

if grep -q 'patch_sum protobuf' "$ROOT/scripts/build-deps.sh" \
   && grep -q 'patch_sum libgit2' "$ROOT/scripts/build-deps.sh"; then
    say ok "build-deps.sh stamps protobuf and libgit2 with patch_sum"
else
    say FAIL "build-deps.sh does not stamp with patch_sum"
    fails=$((fails + 1))
fi

if [ "$fails" -ne 0 ]; then
    echo FAILED
    exit 1
fi
echo all ok
