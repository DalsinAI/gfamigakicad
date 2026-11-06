/*
 * KiCad's font lookup on the Amiga: gfopenfont.library's font index and
 * OF_FindFaceA in place of Fontconfig. The answers follow upstream's
 * fontconfig.cpp: the same FF_RESULT codes, the same "bold" and "italic"
 * words in style names, the same reports.
 * Copyright (c) 2026 AmiCradle Team
 */
#include <mutex>
#include <font/fontconfig.h>
#include <wx/log.h>
#include <trace_helpers.h>
#include <string_utils.h>
#include <macros.h>
#include <reporter.h>

#include <font/gfopenfont_kicad.h>

#include <exec/types.h>
#include <libraries/gfopenfont.h>
#include <proto/gfopenfont.h>

using namespace fontconfig;

static FONTCONFIG* g_config = nullptr;

REPORTER* FONTCONFIG::s_reporter = nullptr;
static std::mutex g_fontConfigMutex;

/// what loadFace reads from FindFont's answer for an installed family
static const wxString s_installedPrefix = wxS( "gfopenfont:" );


wxString FONTCONFIG::Version()
{
    return wxString::FromUTF8( "GFOpenFont " + gfopenfont::Version() );
}


FONTCONFIG::FONTCONFIG()
{
}


void FONTCONFIG::SetReporter( REPORTER* aReporter )
{
    std::lock_guard lock( g_fontConfigMutex );
    s_reporter = aReporter;
}


REPORTER* FONTCONFIG::GetReporter()
{
    std::lock_guard lock( g_fontConfigMutex );
    return s_reporter;
}


FONTCONFIG* Fontconfig()
{
    if( !g_config )
        g_config = new FONTCONFIG();

    return g_config;
}


/// the file of an indexed face, or "" (the index is read once)
static wxString indexedPath( const wxString& aFamily, const wxString& aStyle )
{
    static std::vector<gfopenfont::FACE_RECORD> s_faces;
    static bool                                 s_read = false;

    if( !s_read )
    {
        s_faces = gfopenfont::IndexedFaces();
        s_read = true;
    }

    std::string family = aFamily.utf8_string();
    std::string style = aStyle.utf8_string();

    for( const gfopenfont::FACE_RECORD& r : s_faces )
    {
        if( r.family == family && r.style == style )
            return wxString::FromUTF8( r.path );
    }

    return wxEmptyString;
}


static bool styleIsBold( const wxString& aStyle, unsigned aWeight, bool aWantBold )
{
    wxString s = aStyle.Lower();

    if( s.Contains( wxS( "thin" ) ) || s.Contains( wxS( "light" ) ) || s.Contains( wxS( "regular" ) )
            || s.Contains( wxS( "roman" ) ) || s.Contains( wxS( "book" ) ) )
    {
        return false;
    }

    if( s.Contains( wxS( "medium" ) ) || s.Contains( wxS( "semibold" ) )
            || s.Contains( wxS( "demibold" ) ) )
    {
        return aWantBold;
    }

    if( s.Contains( wxS( "bold" ) ) || s.Contains( wxS( "heavy" ) ) || s.Contains( wxS( "black" ) )
            || s.Contains( wxS( "thick" ) ) || s.Contains( wxS( "dark" ) ) )
    {
        return true;
    }

    return aWeight >= 600;
}


static bool styleIsItalic( const wxString& aStyle, bool aFlag )
{
    wxString s = aStyle.Lower();

    return aFlag || s.Contains( wxS( "italic" ) ) || s.Contains( wxS( "oblique" ) )
           || s.Contains( wxS( "slant" ) );
}


static FONTCONFIG::FF_RESULT styleResult( bool aBold, bool aItalic, bool aHasBold, bool aHasItal )
{
    using FF = FONTCONFIG::FF_RESULT;

    if( ( aBold && !aHasBold ) && ( aItalic && !aHasItal ) )
        return FF::FF_MISSING_BOLD_ITAL;
    else if( aBold && !aHasBold )
        return FF::FF_MISSING_BOLD;
    else if( aItalic && !aHasItal )
        return FF::FF_MISSING_ITAL;
    else if( ( aBold != aHasBold ) || ( aItalic != aHasItal ) )
        return FF::FF_SUBSTITUTE;

    return FF::FF_OK;
}


bool FONTCONFIG::findEmbedded( const wxString& aFontName, bool aBold, bool aItalic,
                               const std::vector<wxString>& aEmbeddedFiles, wxString& aFontFile,
                               int& aFaceIndex, FF_RESULT& aResult )
{
    int bestScore = -1;

    for( const wxString& file : aEmbeddedFiles )
    {
        OFFace* face = OF_OpenFace( (CONST_STRPTR) file.utf8_str().data(), OFA_Size, 0, TAG_DONE );

        if( !face )
            continue;

        struct OFFaceInfo info;

        if( OF_GetFaceInfo( face, &info ) == OFERR_OK )
        {
            wxString family = wxString::FromUTF8( info.fi_Family );
            wxString style = wxString::FromUTF8( info.fi_Style );

            if( family.Lower().StartsWith( aFontName.Lower() ) )
            {
                bool hasBold = styleIsBold( style, info.fi_Weight, aBold );
                bool hasItal = styleIsItalic( style, ( info.fi_Flags & OFFF_ITALIC ) != 0 );
                int  score = ( hasBold == aBold ? 2 : 0 ) + ( hasItal == aItalic ? 1 : 0 );

                if( score > bestScore )
                {
                    bestScore = score;
                    aFontFile = file;
                    aFaceIndex = 0;
                    aResult = styleResult( aBold, aItalic, hasBold, hasItal );
                }
            }
        }

        OF_CloseFace( face );
    }

    return bestScore >= 0;
}


FONTCONFIG::FF_RESULT FONTCONFIG::FindFont( const wxString& aFontName, wxString& aFontFile,
                                            int& aFaceIndex, bool aBold, bool aItalic,
                                            const std::vector<wxString>* aEmbeddedFiles )
{
    FF_RESULT retval = FF_RESULT::FF_ERROR;

    if( !gfopenfont::Ready() )
    {
        if( s_reporter )
            s_reporter->Report( _( "gfopenfont.library is missing: outline fonts are not shown." ) );

        return retval;
    }

    wxString lowerName = aFontName.Lower();

    if( lowerName.Contains( wxS( "bold" ) ) || lowerName.Contains( wxS( "heavy" ) )
            || lowerName.Contains( wxS( "black" ) ) || lowerName.Contains( wxS( "thick" ) )
            || lowerName.Contains( wxS( "dark" ) ) )
    {
        aBold = true;
    }

    // a font the document carries comes first, as Fontconfig's app fonts do
    if( aEmbeddedFiles
            && findEmbedded( aFontName, aBold, aItalic, *aEmbeddedFiles, aFontFile, aFaceIndex, retval ) )
    {
        return retval;
    }

    wxString qualifiedFontName = aFontName;
    wxString fontName;

    OFFace* face = OF_FindFace( (CONST_STRPTR) aFontName.utf8_str().data(),
                                OFA_Weight, aBold ? 700 : 400, OFA_Italic, aItalic ? TRUE : FALSE,
                                OFA_Fallbacks, FALSE, OFA_Size, 0, TAG_DONE );

    if( face )
    {
        struct OFFaceInfo info;

        if( OF_GetFaceInfo( face, &info ) == OFERR_OK )
        {
            wxString family = wxString::FromUTF8( info.fi_Family );
            wxString style = wxString::FromUTF8( info.fi_Style );

            // "gfopenfont:<name>\n<file>": loadFace finds the face by name again (with
            // the fallback chain); the file, from the font index, is the one a
            // plot embeds
            aFontFile = s_installedPrefix + aFontName + wxS( "\n" ) + indexedPath( family, style );
            aFaceIndex = 0;

            fontName = family;

            if( !style.IsEmpty() )
                fontName += ' ' + style;

            retval = FF_RESULT::FF_SUBSTITUTE;

            if( family.Lower().StartsWith( aFontName.Lower() ) )
            {
                bool hasBold = styleIsBold( style, info.fi_Weight, aBold );
                bool hasItal = styleIsItalic( style, ( info.fi_Flags & OFFF_ITALIC ) != 0 );
                retval = styleResult( aBold, aItalic, hasBold, hasItal );
            }
        }

        OF_CloseFace( face );
    }

    if( retval == FF_RESULT::FF_ERROR )
    {
        if( s_reporter )
            s_reporter->Report( wxString::Format( _( "Error loading font '%s'." ), qualifiedFontName ) );
    }
    else if( retval == FF_RESULT::FF_SUBSTITUTE )
    {
        if( fontName.CmpNoCase( qualifiedFontName ) == 0 )
            retval = FF_RESULT::FF_OK;
        else if( s_reporter )
            s_reporter->Report( wxString::Format( _( "Font '%s' not found; substituting '%s'." ),
                                                  qualifiedFontName, fontName ) );
    }

    return retval;
}


void FONTCONFIG::ListFonts( std::vector<std::string>& aFonts, const std::string& aDesiredLang,
                            const std::vector<wxString>* aEmbeddedFiles, bool aForce )
{
    if( !gfopenfont::Ready() )
        return;

    if( m_fontInfoCache.empty() || m_fontCacheLastLang != aDesiredLang || aForce )
    {
        m_fontInfoCache.clear();

        std::vector<gfopenfont::FACE_RECORD> faces = gfopenfont::IndexedFaces();

        if( aEmbeddedFiles )
        {
            for( const wxString& file : *aEmbeddedFiles )
            {
                OFFace* face = OF_OpenFace( (CONST_STRPTR) file.utf8_str().data(), OFA_Size, 0,
                                            TAG_DONE );

                if( !face )
                    continue;

                struct OFFaceInfo info;

                if( OF_GetFaceInfo( face, &info ) == OFERR_OK )
                {
                    gfopenfont::FACE_RECORD r;
                    r.family = info.fi_Family;
                    r.style = info.fi_Style;
                    r.path = file.utf8_string();
                    faces.push_back( std::move( r ) );
                }

                OF_CloseFace( face );
            }
        }

        for( gfopenfont::FACE_RECORD& r : faces )
        {
            if( r.family.empty() || r.family.front() == '.' )
                continue;

            FONTINFO fontInfo( std::string( r.path ), std::string( r.style ), r.family );

            auto it = m_fontInfoCache.find( r.family );

            if( it == m_fontInfoCache.end() )
                m_fontInfoCache.emplace( r.family, fontInfo );
            else
                it->second.Children().push_back( fontInfo );
        }

        m_fontCacheLastLang = aDesiredLang;
    }

    for( const std::pair<const std::string, FONTINFO>& entry : m_fontInfoCache )
        aFonts.push_back( entry.second.Family() );
}
