/*
 * KiCad platform layer for AmigaOS 3 (gfamigakicad).
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, as KiCad.
 */
#include <kiplatform/app.h>
#include <wx/string.h>

bool KIPLATFORM::APP::Init() { return true; }
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
