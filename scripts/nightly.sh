#!/bin/sh
# The nightly build's entry for KiCad (autobuild.md): one long run of
# build-amiga.sh instead of the stove's 840 s slices, which kill KiCad's
# compiles in flight. Runs build-amiga.sh again while it says "not finished,
# run again" (exit 3) and the night's minutes allow.
#   sh scripts/nightly.sh MINUTES
# STOVE (a stove's path or name) and STOVES come from the nightly; the
# packed .lha goes to NIGHTLY_OUT. WORK is build-amiga.sh's own cache, so a
# night that runs out of time carries on the next night.
# Copyright (c) 2026 AmiCradle Team. MIT.
set -eu
HERE=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
MINUTES=${1:-300}
BUILD=${NIGHTLY_BUILD:-"$HERE/build-amiga.sh"}
WANT=os32-gcc16-open9
if [ -n "${STOVES:-}" ] && [ -d "$STOVES/$WANT" ]; then
    STOVE="$STOVES/$WANT"
elif [ -n "${STOVE:-}" ] && [ ! -d "$STOVE" ] && [ -n "${STOVES:-}" ]; then
    STOVE="$STOVES/$STOVE"
fi
[ -n "${STOVE:-}" ] && [ -d "$STOVE" ] || { echo "kicad-nightly: no stove (STOVE=${STOVE:-})"; exit 1; }
export STOVE
OUT=${NIGHTLY_OUT:-"$HERE/out"}; export OUT
END=$(( $(date +%s) + MINUTES * 60 - 300 ))   # five minutes kept for packing up
tries=0
while :; do
    LIMIT=$(( END - $(date +%s) ))
    [ "$LIMIT" -gt 900 ] || { echo "kicad-nightly: out of time, carries on next night"; exit 3; }
    tries=$((tries + 1))
    echo "kicad-nightly: run $tries, $LIMIT s, stove $STOVE"
    rc=0; LIMIT=$LIMIT sh "$BUILD" || rc=$?
    case $rc in
        0) ls "$OUT"/*.lha >/dev/null 2>&1 || { echo "kicad-nightly: finished but no .lha in $OUT"; exit 1; }
           echo "kicad-nightly: built"; exit 0 ;;
        3) [ "$tries" -lt 20 ] || { echo "kicad-nightly: 20 runs, giving up"; exit 3; } ;;
        *) echo "kicad-nightly: build-amiga.sh failed ($rc)"; exit "$rc" ;;
    esac
done
