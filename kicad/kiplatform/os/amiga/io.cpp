/*
 * KiCad platform layer for AmigaOS 3 (gfamigakicad): file calls on libnix.
 * No mmap (files are read into memory), no permission bits to copy (the
 * AmigaDOS protection bits stay as the file system sets them), and folder
 * names joined with wxFileName, since "Work:" + "/" + name names the parent.
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, as KiCad.
 */
#include <kiplatform/io.h>
#include <wx/crt.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/string.h>

#include <cstdio>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

FILE* KIPLATFORM::IO::SeqFOpen( const wxString& aPath, const wxString& aMode )
{
    return wxFopen( aPath, aMode );
}

bool KIPLATFORM::IO::DuplicatePermissions( const wxString&, const wxString& )
{
    return true;
}

bool KIPLATFORM::IO::MakeWriteable( const wxString& aFilePath )
{
    struct stat fileStat;

    if( stat( aFilePath.fn_str(), &fileStat ) != 0 )
        return false;

    return chmod( aFilePath.fn_str(), fileStat.st_mode | S_IWUSR ) == 0;
}

KIPLATFORM::IO::TARGET_ATTRS KIPLATFORM::IO::CaptureTargetAttributes( const wxString& )
{
    return TARGET_ATTRS{};
}

bool KIPLATFORM::IO::ApplyTargetAttributes( const wxString&, const TARGET_ATTRS& )
{
    return true;
}

bool KIPLATFORM::IO::IsFileHidden( const wxString& aFileName )
{
    wxFileName fn( aFileName );
    return fn.GetName().StartsWith( wxT( "." ) );
}

void KIPLATFORM::IO::LongPathAdjustment( wxFileName& )
{
}

long long KIPLATFORM::IO::TimestampDir( const wxString& aDirPath, const wxString& aFilespec )
{
    long long timestamp = 0;
    wxDir     dir( aDirPath );

    if( !dir.IsOpened() )
        return 0;

    wxString name;

    for( bool ok = dir.GetFirst( &name, aFilespec, wxDIR_FILES ); ok; ok = dir.GetNext( &name ) )
    {
        wxFileName  fn( aDirPath, name );
        struct stat st;

        if( stat( fn.GetFullPath().fn_str(), &st ) == 0 )
            timestamp += (long long) st.st_mtime * 1000 + st.st_size;
        else
            timestamp += (signed) std::hash<std::string>{}( std::string( name.fn_str() ) );
    }

    return timestamp;
}

bool KIPLATFORM::IO::FlushToDisk( FILE* aFp )
{
    // fflush hands the data to AmigaDOS, which writes it out itself
    return aFp && std::fflush( aFp ) == 0;
}

KIPLATFORM::IO::MAPPED_FILE::MAPPED_FILE( const wxString& aFileName )
{
    readIntoBuffer( aFileName );
}

KIPLATFORM::IO::MAPPED_FILE::~MAPPED_FILE()
{
}
