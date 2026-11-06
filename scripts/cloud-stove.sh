#!/bin/sh
# Makes a stove-shaped compiler folder from a GFOpenAmigaGCC release and
# OpenNDK, for building where there is no Cradle stove (a cloud machine):
# $STOVE/prefix is the release, with gcc and g++ given OpenNDK's headers
# (and the socket headers) after their own, as a stove's ndk-include is.
#   REL=<unpacked release> NDK=<OpenNDK Include_H> STOVE=<new folder> ./cloud-stove.sh
# Copyright (c) 2026 AmiCradle Team
set -eu
REL=${REL:?}; NDK=${NDK:?}; STOVE=${STOVE:?}
P="$STOVE/prefix"; rm -rf "$P"; mkdir -p "$P/bin"
for d in "$REL"/*; do [ "$(basename "$d")" = bin ] || ln -s "$d" "$P/$(basename "$d")"; done
for t in "$REL"/bin/*; do
    n=$(basename "$t")
    case "$n" in
        m68k-amigaos-gcc|m68k-amigaos-g++|m68k-amigaos-c++|m68k-amigaos-cpp)
            printf '#!/bin/sh\nexec "%s" "$@" -idirafter "%s"\n' "$t" "$NDK" > "$P/bin/$n"; chmod +x "$P/bin/$n" ;;
        *) ln -s "$t" "$P/bin/$n" ;;
    esac
done
"$P/bin/m68k-amigaos-gcc" --version | head -1
