/*
 * gfopenfont.library for KiCad's fonts: opened once, on first use.
 * Copyright (c) 2026 AmiCradle Team
 */
#include <font/gfopenfont_kicad.h>

#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <exec/types.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <libraries/gfopenfont.h>
#include <proto/gfopenfont.h>

extern "C" {
struct Library* OpenFontBase = nullptr;
}

namespace
{
std::once_flag s_open;

void closeLibrary()
{
    if( OpenFontBase )
    {
        CloseLibrary( OpenFontBase );
        OpenFontBase = nullptr;
    }
}
}


bool gfopenfont::Ready()
{
    std::call_once( s_open,
                    []()
                    {
                        OpenFontBase = OpenLibrary( (CONST_STRPTR) OPENFONTNAME, OPENFONT_VERSION );

                        // OF_FindFaceA's OFA_Exact and OF_GetFaceMetrics came with revision 3
                        if( OpenFontBase && OpenFontBase->lib_Revision < OPENFONT_REVISION_LAYOUT )
                            closeLibrary();

                        if( OpenFontBase )
                            atexit( closeLibrary );
                    } );

    return OpenFontBase != nullptr;
}


std::string gfopenfont::Version()
{
    if( !Ready() )
        return std::string();

    ULONG v = OF_Version();
    char  buf[24];
    snprintf( buf, sizeof buf, "%lu.%lu", (unsigned long) ( v >> 16 ), (unsigned long) ( v & 0xffff ) );
    return buf;
}


static const UBYTE* table( OFFace* aFace, const char aTag[5], ULONG* aLength )
{
    if( !aFace || !gfopenfont::Ready() )
        return nullptr;

    ULONG tag = OF_MAKETAG( aTag[0], aTag[1], aTag[2], aTag[3] );
    return (const UBYTE*) OF_GetFontTable( aFace, tag, aLength );
}


unsigned gfopenfont::TableU16( OFFace* aFace, const char aTag[5], size_t aOffset, unsigned aDefault )
{
    ULONG        length = 0;
    const UBYTE* t = table( aFace, aTag, &length );

    if( !t || aOffset + 2 > length )
        return aDefault;

    return ( (unsigned) t[aOffset] << 8 ) | t[aOffset + 1];
}


int gfopenfont::TableS16( OFFace* aFace, const char aTag[5], size_t aOffset, int aDefault )
{
    ULONG        length = 0;
    const UBYTE* t = table( aFace, aTag, &length );

    if( !t || aOffset + 2 > length )
        return aDefault;

    return (int16_t) ( ( t[aOffset] << 8 ) | t[aOffset + 1] );
}


std::vector<gfopenfont::FACE_RECORD> gfopenfont::IndexedFaces()
{
    std::vector<FACE_RECORD> faces;

    if( !Ready() )
        return faces;

    // finding a face brings the index up to date first
    if( OFFace* f = OF_FindFace( nullptr, OFA_Fallbacks, FALSE, TAG_DONE ) )
        OF_CloseFace( f );

    // ENVARC:GFOpenFont/FontIndex, "OFIX 2": D lines (folders) and F lines
    // (face index, weight, width, flags, four Unicode ranges in hex; then
    // family, style and path, tab separated), ended by "E count"
    FILE* fp = fopen( "ENVARC:GFOpenFont/FontIndex", "r" );

    if( !fp )
        return faces;

    char line[1024];
    bool versionOk = false;
    bool ended = false;

    while( fgets( line, sizeof line, fp ) )
    {
        line[strcspn( line, "\r\n" )] = 0;

        if( !versionOk )
        {
            versionOk = strncmp( line, "OFIX 2", 6 ) == 0;

            if( !versionOk )
                break;

            continue;
        }

        if( line[0] == 'E' )
        {
            ended = true;
            break;
        }

        if( line[0] != 'F' || line[1] != ' ' )
            continue;

        unsigned long words[8];
        char*         p = line + 2;
        int           n = 0;

        for( ; n < 8; n++ )
        {
            char* end = nullptr;
            words[n] = strtoul( p, &end, 16 );

            if( end == p )
                break;

            p = end;

            while( *p == ' ' )
                p++;
        }

        if( n < 8 || *p != '\t' )
            continue;

        char* family = p + 1;
        char* style = strchr( family, '\t' );

        if( !style )
            continue;

        *style++ = 0;
        char* path = strchr( style, '\t' );

        if( !path )
            continue;

        *path++ = 0;

        FACE_RECORD r;
        r.faceIndex = (unsigned) words[0];
        r.weight = (unsigned) words[1];
        r.italic = ( words[3] & OFFF_ITALIC ) != 0;
        r.family = family;
        r.style = style;
        r.path = path;
        faces.push_back( std::move( r ) );
    }

    fclose( fp );

    if( !ended )
        faces.clear();

    return faces;
}
