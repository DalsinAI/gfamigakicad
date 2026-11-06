/*
 * KiCad platform layer for AmigaOS 3 (gfamigakicad).
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, as KiCad.
 */
#include <kiplatform/secrets.h>

// no keyring on the Amiga: secrets are not kept, so KiCad asks again
bool KIPLATFORM::SECRETS::StoreSecret( const wxString&, const wxString&, const wxString& ) { return false; }
bool KIPLATFORM::SECRETS::GetSecret( const wxString&, const wxString&, wxString& ) { return false; }
bool KIPLATFORM::SECRETS::DeleteSecret( const wxString&, const wxString& ) { return false; }
