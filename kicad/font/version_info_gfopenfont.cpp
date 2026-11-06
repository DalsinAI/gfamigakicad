/*
 * The font library versions KiCad reports, on the Amiga: gfopenfont.library
 * does FreeType's, HarfBuzz's and Fontconfig's work.
 * Copyright (c) 2026 AmiCradle Team
 */
#include <font/version_info.h>
#include <font/fontconfig.h>

#include <font/gfopenfont_kicad.h>

using namespace KIFONT;

wxString VERSION_INFO::FreeType()
{
    return wxS( "not used (gfopenfont.library)" );
}


wxString VERSION_INFO::HarfBuzz()
{
    return wxS( "not used (gfopenfont.library)" );
}


wxString VERSION_INFO::FontConfig()
{
    return fontconfig::FONTCONFIG::Version();
}


wxString VERSION_INFO::FontLibrary()
{
    std::string v = gfopenfont::Version();

    return v.empty() ? wxString( wxS( "gfopenfont.library is missing" ) )
                     : wxString::FromUTF8( "GFOpenFont " + v );
}
