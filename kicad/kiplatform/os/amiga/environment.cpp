/*
 * KiCad platform layer for AmigaOS 3 (gfamigakicad).
 * Settings, libraries' caches and new projects live beside the program,
 * in PROGDIR:User, as the Team's ports keep them.
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, as KiCad.
 */
#include <kiplatform/environment.h>
#include <wx/filename.h>
#include <wx/utils.h>

static wxString userDir( const wxChar* aSub )
{
    wxFileName fn;
    fn.AssignDir( wxT( "PROGDIR:User" ) );

    if( aSub && *aSub )
        fn.AppendDir( aSub );

    if( !fn.DirExists() )
        fn.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

    return fn.GetPath();
}

void KIPLATFORM::ENV::Init()
{
}

bool KIPLATFORM::ENV::MoveToTrash( const wxString& aPath, wxString& aError )
{
    // AmigaOS has no wastebasket for programs to move files to
    aError = _( "AmigaOS has no trash to move files to." );
    return false;
}

bool KIPLATFORM::ENV::IsNetworkPath( const wxString& )
{
    return false;
}

wxString KIPLATFORM::ENV::GetDocumentsPath()
{
    return userDir( wxT( "Projects" ) );
}

wxString KIPLATFORM::ENV::GetUserConfigPath()
{
    return userDir( wxT( "Config" ) );
}

wxString KIPLATFORM::ENV::GetUserDataPath()
{
    return userDir( wxT( "Data" ) );
}

wxString KIPLATFORM::ENV::GetUserLocalDataPath()
{
    return userDir( wxT( "Data" ) );
}

wxString KIPLATFORM::ENV::GetUserCachePath()
{
    return userDir( wxT( "Cache" ) );
}

bool KIPLATFORM::ENV::GetSystemProxyConfig( const wxString&, PROXY_CONFIG& )
{
    return false;
}

bool KIPLATFORM::ENV::VerifyFileSignature( const wxString& )
{
    return true;
}

wxString KIPLATFORM::ENV::GetAppUserModelId()
{
    return wxEmptyString;
}

void KIPLATFORM::ENV::SetAppDetailsForWindow( wxWindow*, const wxString&, const wxString& )
{
}

wxString KIPLATFORM::ENV::GetCommandLineStr()
{
    return wxEmptyString;
}

void KIPLATFORM::ENV::AddToRecentDocs( const wxString& )
{
}
