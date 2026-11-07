// icontest: why the KiCad toolbars show no icons. Reads two icons out of
// PROGDIR:share/kicad/resources/images.tar.gz the way BITMAP_STORE does
// (gunzip, tar, wxImage from memory as PNG), then puts them on a
// wxAuiToolBar (KiCad's ACTION_TOOLBAR) and a plain wxToolBar, and writes
// what it finds to T:icontest.log. The window stays open for a screenshot.
//
// Copyright (c) 2026 AmiCradle Team

#include <wx/wx.h>
#include <wx/zstream.h>
#include <wx/wfstream.h>
#include <wx/mstream.h>
#include <wx/aui/auibar.h>
#include <cstdio>

#include <map>
#include <string>
#include <vector>
#include <cstring>

// opened and closed per line, so the log can be read while the test runs
static void Log( const wxString& aText )
{
    wxString line = aText + "\n";
    fputs( line.mb_str(), stdout );
    fflush( stdout );

    if( FILE* f = fopen( "T:icontest.log", "a" ) )
    {
        fputs( line.mb_str(), f );
        fclose( f );
    }
}

static size_t Octal( const char* aField, int aLen )
{
    size_t v = 0;

    for( int i = 0; i < aLen && aField[i]; i++ )
    {
        if( aField[i] >= '0' && aField[i] <= '7' )
            v = v * 8 + ( aField[i] - '0' );
        else if( aField[i] != ' ' )
            break;
    }

    return v;
}

// every file in the archive whose name is in aWant
static std::map<std::string, std::vector<char>> ReadTar( const wxString& aPath,
                                                         const std::vector<std::string>& aWant )
{
    std::map<std::string, std::vector<char>> found;
    wxFFileInputStream file( aPath );

    if( !file.IsOk() )
    {
        Log( "cannot open " + aPath );
        return found;
    }

    wxZlibInputStream z( file, wxZLIB_GZIP );
    char   hdr[512];
    size_t entries = 0;

    auto readFully = [&]( void* aDest, size_t aLen ) -> bool
    {
        z.Read( aDest, aLen );
        return z.LastRead() == aLen;
    };

    while( readFully( hdr, 512 ) && hdr[0] )
    {
        size_t      len = Octal( hdr + 124, 12 );
        size_t      padded = ( len + 511 ) & ~(size_t) 511;
        std::string name( hdr, strnlen( hdr, 100 ) );

        while( name.compare( 0, 2, "./" ) == 0 )
            name.erase( 0, 2 );

        std::vector<char> data( padded );

        if( padded && !readFully( data.data(), padded ) )
            break;

        entries++;

        if( entries % 500 == 0 )
            Log( wxString::Format( "tar: %lu entries", (unsigned long) entries ) );

        for( const std::string& w : aWant )
        {
            if( name == w )
            {
                data.resize( len );
                found[w] = data;
            }
        }
    }

    Log( wxString::Format( "tar: %lu entries read, %lu of %lu wanted found",
                           (unsigned long) entries, (unsigned long) found.size(),
                           (unsigned long) aWant.size() ) );
    return found;
}

class IconApp : public wxApp
{
public:
    bool OnInit() override
    {
        remove( "T:icontest.log" );
        Log( "icontest start" );
        wxInitAllImageHandlers();

        wxString path = argc > 1 ? argv[1] : wxString( "PROGDIR:share/kicad/resources/images.tar.gz" );
        std::vector<std::string> want = { "new_project_24.png", "zoom_fit_in_page_24.png",
                                          "new_project_16.png", "zoom_fit_in_page_16.png" };
        auto files = ReadTar( path, want );

        wxImage::AddHandler( new wxPNGHandler ); // harmless if already there
        Log( wxString::Format( "PNG handler: %s",
                               wxImage::FindHandler( wxBITMAP_TYPE_PNG ) ? "yes" : "NO" ) );

        std::vector<wxBitmap> bmps;

        for( const std::string& w : want )
        {
            auto it = files.find( w );

            if( it == files.end() )
            {
                Log( wxString( "missing " ) + w );
                continue;
            }

            wxMemoryInputStream is( it->second.data(), it->second.size() );
            wxImage             img( is, wxBITMAP_TYPE_PNG );
            Log( wxString::Format( "%s: %lu bytes, image ok %d, %dx%d, alpha %d", w.c_str(),
                                   (unsigned long) it->second.size(), img.IsOk(),
                                   img.IsOk() ? img.GetWidth() : 0, img.IsOk() ? img.GetHeight() : 0,
                                   img.IsOk() ? img.HasAlpha() : 0 ) );

            wxBitmap bmp( img );
            Log( wxString::Format( "  bitmap ok %d, %dx%d, depth %d", bmp.IsOk(),
                                   bmp.IsOk() ? bmp.GetWidth() : 0, bmp.IsOk() ? bmp.GetHeight() : 0,
                                   bmp.IsOk() ? bmp.GetDepth() : 0 ) );
            bmps.push_back( bmp );
        }

        while( bmps.size() < 4 )
            bmps.push_back( wxBitmap() );

        wxFrame* frame = new wxFrame( nullptr, wxID_ANY, "icontest", wxDefaultPosition, wxSize( 640, 300 ) );

        // the KiCad way: a bundle of the 24 and 16 pixel images per tool
        wxVector<wxBitmap> b1, b2;
        b1.push_back( bmps[0] );
        b1.push_back( bmps[2] );
        b2.push_back( bmps[1] );
        b2.push_back( bmps[3] );
        wxBitmapBundle bundle1 = wxBitmapBundle::FromBitmaps( b1 );
        wxBitmapBundle bundle2 = wxBitmapBundle::FromBitmaps( b2 );
        Log( wxString::Format( "bundle ok %d %d, default size %dx%d", bundle1.IsOk(), bundle2.IsOk(),
                               bundle1.GetDefaultSize().x, bundle1.GetDefaultSize().y ) );

        wxPanel*    panel = new wxPanel( frame );
        wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

        wxAuiToolBar* aui = new wxAuiToolBar( panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                              wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );
        aui->SetToolBitmapSize( wxSize( 24, 24 ) );
        aui->AddTool( 101, "New", bundle1 );
        aui->AddTool( 102, "Zoom", bundle2 );
        aui->AddSeparator();
        aui->AddControl( new wxChoice( aui, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       wxArrayString( 1, "Page 1" ) ) );
        aui->Realize();

        wxToolBar* tb = new wxToolBar( panel, wxID_ANY );
        tb->AddTool( 201, "New", bundle1 );
        tb->AddTool( 202, "Zoom", bundle2 );
        tb->Realize();

        sizer->Add( aui, 0, wxEXPAND );
        sizer->Add( tb, 0, wxEXPAND );
        sizer->Add( new wxStaticText( panel, wxID_ANY, "aui toolbar above, wxToolBar below" ), 1,
                    wxEXPAND | wxALL, 8 );
        panel->SetSizer( sizer );
        frame->Show();
        frame->Layout();

        wxSize as = aui->GetSize(), ts = tb->GetSize();
        wxRect r1 = aui->GetToolRect( 101 ), r2 = aui->GetToolRect( 102 );
        Log( wxString::Format( "aui toolbar %dx%d, %lu tools, tool 101 at %d,%d %dx%d, 102 at %d,%d %dx%d",
                               as.x, as.y, (unsigned long) aui->GetToolCount(), r1.x, r1.y, r1.width,
                               r1.height, r2.x, r2.y, r2.width, r2.height ) );
        wxAuiToolBarItem* it = aui->FindTool( 101 );
        Log( wxString::Format( "aui tool 101 bitmap ok %d, min size %dx%d",
                               it ? it->GetBitmapFor( aui ).IsOk() : -1,
                               it ? it->GetMinSize().x : -1, it ? it->GetMinSize().y : -1 ) );
        wxSize pref = bundle1.GetPreferredBitmapSizeFor( aui );
        wxBitmap forAui = bundle1.GetBitmapFor( aui );
        Log( wxString::Format( "DPI scale %.3f, content scale %.3f, bundle size for aui %dx%d, bitmap %d %dx%d",
                               aui->GetDPIScaleFactor(), aui->GetContentScaleFactor(), pref.x, pref.y,
                               forAui.IsOk(), forAui.IsOk() ? forAui.GetWidth() : 0,
                               forAui.IsOk() ? forAui.GetHeight() : 0 ) );
        Log( wxString::Format( "wxToolBar %dx%d, %lu tools", ts.x, ts.y, (unsigned long) tb->GetToolsCount() ) );
        Log( "done; close the window to quit" );
        return true;
    }
};

wxIMPLEMENT_APP( IconApp );
