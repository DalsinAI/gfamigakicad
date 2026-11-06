/*
 * KiCad platform layer for AmigaOS 3 (gfamigakicad).
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, as KiCad.
 */
#include <kiplatform/app.h>
#include <wx/string.h>
#include <wx/tarstrm.h>
#include <wx/utils.h>

#include <dos/dosextens.h>
#include <proto/exec.h>

/* The C library's user and group lookups (getpwuid, getgrgid) read
 * inet:db/passwd and inet:db/group when no usergroup.library is running.
 * Without an inet: assign that open asks for "volume inet:" and waits for
 * ever, and wx asks for the user and group the first time it makes a tar
 * entry (KiCad's icon archive) or a lock file. Ask once here with DOS
 * requesters off, so a missing inet: just answers "unknown"; wx keeps what
 * it got for the rest of the run. */
static void askUserOnceQuietly()
{
    struct Process* me = (struct Process*) FindTask( nullptr );
    APTR            oldWindow = me->pr_WindowPtr;

    me->pr_WindowPtr = (APTR) -1;
    {
        wxTarEntry userAndGroup;  // wx caches the user and group names
        (void) wxGetUserId();
    }
    me->pr_WindowPtr = oldWindow;
}

bool KIPLATFORM::APP::Init()
{
    askUserOnceQuietly();
    return true;
}
void KIPLATFORM::APP::EnableDarkMode( bool ) {}
bool KIPLATFORM::APP::AttachConsole( bool ) { return true; }
bool KIPLATFORM::APP::IsOperatingSystemUnsupported() { return false; }
bool KIPLATFORM::APP::RegisterApplicationRestart( const wxString& ) { return true; }
bool KIPLATFORM::APP::UnregisterApplicationRestart() { return true; }
bool KIPLATFORM::APP::SupportsShutdownBlockReason() { return false; }
void KIPLATFORM::APP::RemoveShutdownBlockReason( wxWindow* ) {}
void KIPLATFORM::APP::SetShutdownBlockReason( wxWindow*, const wxString& ) {}
void KIPLATFORM::APP::ForceTimerMessagesToBeCreatedIfNecessary() {}
// kifaces are linked into the program: there is no library path to add
void KIPLATFORM::APP::AddDynamicLibrarySearchPath( const wxString& ) {}
