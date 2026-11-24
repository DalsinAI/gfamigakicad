#!/bin/sh
# Checks scripts/nightly.sh with a stand-in build-amiga.sh: it reruns on
# exit 3, stops on a real failure, needs an .lha to call a build good, and
# picks the open9 stove from STOVES. Also checks autobuild.md's entry.
#   sh tests/host/nightly_test.sh
# Copyright (c) 2026 AmiCradle Team. MIT.
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
fails=0
say() { printf '%s %s\n' "$1" "$2"; }
mkdir -p "$T/stoves/os32-gcc16-open9" "$T/out"

# the stand-in: exits with the codes listed in $T/codes, one per run, and
# writes an .lha on exit 0 unless NOLHA is set
cat > "$T/fake.sh" <<'FAKE'
n=$(cat "$T/n" 2>/dev/null || echo 0); n=$((n + 1)); echo $n > "$T/n"
echo "$STOVE $LIMIT" > "$T/seen"
rc=$(sed -n "${n}p" "$T/codes")
[ "$rc" = 0 ] && [ -z "${NOLHA:-}" ] && : > "$OUT/GFAmigaKiCad-test.lha"
exit "$rc"
FAKE

run() { # codes... ; prints the wrapper's exit code
    rm -f "$T/n" "$T/out"/*; printf '%s\n' "$@" > "$T/codes"
    rc=0; T=$T NIGHTLY_BUILD="$T/fake.sh" STOVES="$T/stoves" STOVE=other NIGHTLY_OUT="$T/out" \
        sh "$ROOT/scripts/nightly.sh" 60 >/dev/null 2>&1 || rc=$?
    echo $rc
}
check() { if [ "$2" = "$3" ]; then say ok "$1"; else say FAIL "$1 (got $2)"; fails=$((fails + 1)); fi; }

check "a run that says run again is run again, then good" "$(run 3 3 0)" 0
check "it ran three times" "$(cat "$T/n")" 3
check "the open9 stove is chosen from STOVES" "$(cut -d' ' -f1 "$T/seen")" "$T/stoves/os32-gcc16-open9"
lim=$(cut -d' ' -f2 "$T/seen")
[ "$lim" -gt 3000 ] && [ "$lim" -le 3300 ] && say ok "LIMIT is the minutes less five ($lim s)" || { say FAIL "LIMIT $lim"; fails=$((fails + 1)); }
check "a failure stops it with that code" "$(run 2 0)" 2
check "a good exit with no .lha is a failure" "$(NOLHA=1; export NOLHA; run 0)" 1

A="$ROOT/autobuild.md"
for k in "frequency: nightly" "script: scripts/nightly.sh" "keep: out/\*.lha" "minutes: 360"; do
    grep -q -- "- $k" "$A" && say ok "autobuild.md has $k" || { say FAIL "autobuild.md lacks $k"; fails=$((fails + 1)); }
done
[ -f "$ROOT/$(sed -n 's/^- script: //p' "$A")" ] && say ok "the script named exists" || { say FAIL "script missing"; fails=$((fails + 1)); }

[ "$fails" = 0 ] && echo "all passed" || { echo "$fails failed"; exit 1; }
