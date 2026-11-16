#!/bin/sh
# Checks the licence files are in the repository and that package.sh
# packs them and writes the source offer and the KiCad 10.0.8 SHA-256.
#   sh tests/host/licence_files_test.sh
# Copyright (c) 2026 AmiCradle Team. MIT.
set -eu
HERE=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
fails=0
say() { printf '%s %s\n' "$1" "$2"; }

check_file() {
    if [ -f "$HERE/$1" ]; then
        say ok "$1"
    else
        say FAIL "missing $1"
        fails=$((fails + 1))
    fi
}

contains() {
    file=$1; needle=$2; label=$3
    if grep -q -F "$needle" "$HERE/$file"; then
        say ok "$label"
    else
        say FAIL "$label"
        fails=$((fails + 1))
    fi
}

check_file README.md
check_file COPYING
check_file LICENSE.MIT
check_file THIRD-PARTY.md
contains COPYING "GNU GENERAL PUBLIC LICENSE" "COPYING is the GNU GPL"
contains COPYING "Version 3, 29 June 2007" "COPYING is version 3"
contains LICENSE.MIT "Copyright (c) 2026 AmiCradle Team" "LICENSE.MIT names the Team"
contains scripts/package.sh "COPYING" "package.sh packs COPYING"
contains scripts/package.sh "LICENSE.MIT" "package.sh packs LICENSE.MIT"
contains scripts/package.sh "THIRD-PARTY.md" "package.sh packs THIRD-PARTY.md"
contains scripts/package.sh "https://github.com/DalsinAI/gfamigakicad" "package.sh offers the source repo"
contains scripts/package.sh "0723fedd82522fcd33cb9d8ad53979a35f6e4d3ec627c4e2e2708dfcba360574" "package.sh names the KiCad SHA-256"
contains scripts/package.sh "https://gitlab.com/kicad/code/kicad/-/archive/10.0.8/kicad-10.0.8.tar.gz" "package.sh names the KiCad tarball"
contains README.md "GPL-3.0-or-later" "README names the GPL"
contains THIRD-PARTY.md "wxWindows Library Licence" "THIRD-PARTY names wx"

if [ "$fails" -ne 0 ]; then
    echo FAILED
    exit 1
fi
echo all ok
