# Changelog

## 0.1.0-RC5

Source in this repository, 16 November 2026. A new LhA has not been
packed yet: this machine has the 68040 compiler but not cmake, ninja,
a host C++ compiler, lha or zip, and no stove build cache.

- z.library 53.12 (the copy on most systems, still named "OpenZlib") is
  used through its own jump table, not the Aminet 53.13 table. 53.13 and
  later are unchanged. Host test: 9 of 9.
- README, COPYING (GNU GPL version 3), LICENSE.MIT and THIRD-PARTY.md
  are in the repository. The packaged ReadMe.txt offers the corresponding
  source (this repository and the KiCad 10.0.8 tarball with its SHA-256)
  and the licence files travel with the LhA.
- Pins: GFOpenWX Toolkit 1.0-rc4 (0dc65bc), gfopencurl 07e59ed (the
  GFOpenTLS hook puts back the program's A4), GFOpenBoost v1.89.0-rc3.
  The 1.0-rc4 toolkit release has no SDK tarball, so the Amiga build
  compiles the toolkit from that commit. Patch stamps for protobuf and
  libgit2 now hash the patch files, so a later edit is not skipped.
- malloc refuses a size that would wrap on 32-bit, returns 8-aligned
  small blocks, and takes Fast RAM first. Host test: 11 of 11.
- The open6 NDK in this compiler treats STRPTR as unsigned char*, and
  proto/bsdsocket.h aliases m_next through sys/mbuf.h. The Amiga build
  passes -fpermissive to C++ and drops those aliases in the toolkit's
  wx webrequest file so the toolkit can compile against this NDK.
- build-deps.sh compiles gfamiga_malloc.c with compat/include on the
  search path, so the size-class header is found.

Still open from the completeness review: no official symbol or
footprint libraries in the package; ODB++ export can still hang on
wxTar; fpuauto is not wired from this repository; kicad-cli and
cvpcb_kiface are not fully linked; Python, the 3D viewer, the
simulator and STEP export are still stubbed.

## 0.1.0-RC4

7 November 2026. KiCad builds on GFOpenBoost 1.89.0-rc1, the shared
Boost SDK. Floating-point maths runs on the 68040 FPU when one is
present (fpuauto), on the stove that packed this RC.

## 0.1.0-RC3

7 November 2026.

## 0.1.0-RC2

7 November 2026. Build on the toolkit with the toolbar fix.

## 0.1.0-RC1

7 November 2026. First package: the editors as complete programs, on
gfopenfont, the gfopengpu canvas, z.library and gfopencurl.
