# GFAmigaKiCad

KiCad 10.0.8 for AmigaOS 3.2, 68040, on the GFOpenWX Toolkit. This
repository is the Amiga port: the build, the compatibility layer, the
patches and the packaging. KiCad itself is fetched at build time and
checked by SHA-256 (`deps.lock`).

The latest source version is **0.1.0-RC5**. The latest tagged package is [0.1.0-RC4](https://github.com/DalsinAI/gfamigakicad/releases/tag/v0.1.0-rc4).

## What it ships

The LhA holds these programs, each a complete binary (AmigaOS has no
shared-object loading for KiCad's kifaces):

- `kicad` — the project manager
- `eeschema` — schematics
- `pcbnew` — boards
- `gerbview`
- `pl_editor` — drawing sheets
- `pcb_calculator`
- `bitmap2component`
- `kicad-cli`

Two of KiCad's demo boards (the ColdFire 5213 kit and the video board)
are packed under `share/kicad/demos`. The official symbol, footprint and
3D-model libraries are not in the package; install those yourself under
`PROGDIR:share/kicad` if you have them.

Not in this port yet: Python plugins, the 3D viewer, the ngspice
simulator, and STEP / other 3D export. The editors do not open each
other from inside one another yet.

## Runtime

In `LIBS:`:

- `gfopenfont.library`
- `gfopengpu.library` 0.18 or later (without it the canvas stays empty)
- `z.library` (GFOpenZlib; 53.12 and 53.13 or later are both used
  correctly, each with its own jump table)
- `bsdsocket.library` for network use
- `gfopentls.library` for HTTPS

Give the programs 1 MB of stack (`Stack 1000000`) when started from the
Shell.

## Building

On a Cradle stove with the 68040 soft-float compiler:

```
STOVE=~/AmiCradle/stoves/os32-gcc16-open6 OUT=out ./build-amiga.sh
```

`deps.lock` pins every source. The script builds in `WORK` and can be
run again if a stove job runs out of time. `scripts/package.sh` writes
`GFAmigaKiCad-<version>.lha` and `.zip`.

A host check that needs no Amiga compiler:

```
cc -I compat/include tests/host/zlib_id_test.c -o zlib_id_test
./zlib_id_test
```

## Licence

KiCad is GPL-3.0-or-later. The binary package and this repository ship
`COPYING` (the GNU GPL version 3). The AmiCradle Team's original files
in `compat/`, `tests/` and the build scripts are also available under
the MIT licence in `LICENSE.MIT`. Because they are combined with KiCad
in the programs, the complete programs are GPL-3.0-or-later.

Third-party notices (wxWidgets, curl, Boost, protobuf, libgit2, zstd
and others) are in `THIRD-PARTY.md`. Corresponding source for a binary
release is this repository plus the KiCad 10.0.8 tarball named in
`deps.lock`; the packaged `ReadMe.txt` repeats that offer.

Designed, tested and delivered by The AmiCradle Team.
Copyright (c) 2026 AmiCradle Team.
