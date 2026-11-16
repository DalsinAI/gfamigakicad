# Third-party notices

The GFAmigaKiCad programs link the works below. Their licences apply to
those parts. This file is a notice, not a substitute for the licence texts.

## KiCad 10.0.8

GNU General Public License, version 3 or later. The complete corresponding
source is the official tarball plus this repository (the Amiga port, patches
and build).

- https://gitlab.com/kicad/code/kicad/-/archive/10.0.8/kicad-10.0.8.tar.gz
- SHA-256: `0723fedd82522fcd33cb9d8ad53979a35f6e4d3ec627c4e2e2708dfcba360574`
- https://github.com/DalsinAI/gfamigakicad

`COPYING` in this repository and in the binary package is the GNU GPL
version 3.

## GFOpenWX Toolkit (wxWidgets)

The programs are built with the GFOpenWX Toolkit, a port of wxWidgets.
wxWidgets and the toolkit are under the wxWindows Library Licence (the
GNU LGPL 2.1 or later, with a linking exception). See
https://github.com/DalsinAI/gfopenwxtoolkit and the wxWindows licence
shipped with wxWidgets.

## libcurl (through gfopencurl) and nghttp2

libcurl is under the curl licence (a permissive, ISC-style licence):
https://curl.se/docs/copyright.html

nghttp2 is MIT: https://github.com/nghttp2/nghttp2

At run time, HTTPS goes through gfopentls.library.

## Boost (GFOpenBoost)

Boost Software License 1.0: https://www.boost.org/LICENSE_1_0.txt

## Protocol Buffers 3.21.12

BSD 3-Clause: https://github.com/protocolbuffers/protobuf

## libgit2

GPL-2.0-only with a linking exception:
https://github.com/libgit2/libgit2/blob/main/COPYING

## zstd 1.5.7

Dual-licensed BSD and GPL-2.0: https://github.com/facebook/zstd

## GLM

Happy Bunny Licence / MIT: https://github.com/g-truc/glm

## zlib / z.library

The programs call the system's z.library (GFOpenZlib) rather than shipping
a copy of zlib. zlib itself is under the zlib licence:
https://www.zlib.net/zlib_license.html

## OpenNDK (Open SDK)

Used at build time for AmigaOS headers. See
https://github.com/DalsinAI/gfopensdk

## ngspice

Only `sharedspice.h` is used at build time. No ngspice library is built or
packed, and the simulator is not in this port. ngspice's own licence is
in its source: https://sourceforge.net/projects/ngspice/
