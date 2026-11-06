/*
 * gfopenfont.library for KiCad's fonts: opened once, on first use.
 * Copyright (c) 2026 AmiCradle Team
 */
#ifndef GFOPENFONT_KICAD_H
#define GFOPENFONT_KICAD_H

#include <string>
#include <vector>

struct OFFace;

namespace gfopenfont
{
/// true when gfopenfont.library (version 1, revision 3 or later) is open
bool Ready();

/// "major.minor" of GFOpenFont, or "" when the library isn't there
std::string Version();

/// a big-endian 16-bit field of a font table, or aDefault
unsigned TableU16( OFFace* aFace, const char aTag[5], size_t aOffset, unsigned aDefault );
int      TableS16( OFFace* aFace, const char aTag[5], size_t aOffset, int aDefault );

struct FACE_RECORD
{
    std::string family;
    std::string style;
    std::string path;
    unsigned    faceIndex = 0;
    unsigned    weight = 400;
    bool        italic = false;
};

/// every face in GFOpenFont's font index (FONTS: and the folders it knows)
std::vector<FACE_RECORD> IndexedFaces();
}

#endif
