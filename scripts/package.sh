#!/bin/sh
# Packs the built programs into GFAmigaKiCad-<version>.lha (and .zip):
# the programs stripped, a ReadMe and an Install script for the Shell.
#   TC=<compiler prefix> BIN=<folder with the programs> VERSION=0.1.0-RC1 OUT=<folder> ./package.sh
# Copyright (c) 2026 AmiCradle Team
set -eu
TC=${TC:?}; BIN=${BIN:?}; VERSION=${VERSION:?}; OUT=${OUT:?}
NAME=GFAmigaKiCad
STAGE=$(mktemp -d); D="$STAGE/$NAME"
mkdir -p "$D/Programs"
for p in kicad kicad-cli eeschema pcbnew gerbview pl_editor bitmap2component pcb_calculator; do
    [ -f "$BIN/$p" ] && "$TC/bin/m68k-amigaos-strip" -o "$D/Programs/$p" "$BIN/$p"
done
cat > "$D/ReadMe.txt" <<TXT
GFAmigaKiCad $VERSION
KiCad 10.0.8 for AmigaOS 3.2, 68040, on the GFOpenWX Toolkit.

Programs: eeschema (schematics), pcbnew (boards), gerbview, pl_editor
(drawing sheets), pcb_calculator, bitmap2component, kicad (project
manager) and kicad-cli.

Needs in LIBS: gfopenfont.library, gfopengpu.library (0.18 or later),
z.library, bsdsocket.library for network use.
Each editor is a whole program (AmigaOS has no shared object loading), so
the editors do not open each other from inside one another yet.
No Python plugins, no 3D view, no simulator, no STEP export yet.

Install: in the Shell, CD to this drawer and Execute Install (copies to Work:KiCad, or give
another drawer: Execute Install DH1:Apps/KiCad).
Give the programs 1 MB of stack (Stack 1000000) when started from the Shell.

Designed, tested and delivered by The AmigaChrome Team.
Copyright (c) 2026 AmiCradle Team. KiCad is GPL-3.0-or-later.
TXT
cat > "$D/Install" <<'TXT'
.KEY DEST
.BRA {
.KET }
; Installs GFAmigaKiCad: Execute Install [drawer]
IF "{DEST}" EQ ""
    SET kcdest "Work:KiCad"
ELSE
    SET kcdest "{DEST}"
ENDIF
IF NOT EXISTS "$kcdest"
    MakeDir "$kcdest" ALL
ENDIF
Copy Programs/#? "$kcdest" CLONE QUIET
Copy ReadMe.txt "$kcdest" CLONE QUIET
Echo "GFAmigaKiCad is in $kcdest"
UnSet kcdest
TXT
mkdir -p "$OUT"
rm -f "$OUT/$NAME-$VERSION.lha" "$OUT/$NAME-$VERSION.zip"
if command -v lha >/dev/null 2>&1; then (cd "$STAGE" && lha -aq "$OUT/$NAME-$VERSION.lha" "$NAME"); fi
if command -v zip >/dev/null 2>&1; then (cd "$STAGE" && zip -qr9 "$OUT/$NAME-$VERSION.zip" "$NAME"); fi
(cd "$OUT" && sha256sum $NAME-$VERSION.* > SHA256SUMS-$VERSION)
rm -rf "$STAGE"
ls -l "$OUT"
