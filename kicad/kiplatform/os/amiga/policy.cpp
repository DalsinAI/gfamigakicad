/*
 * KiCad platform layer for AmigaOS 3 (gfamigakicad).
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, as KiCad.
 */
#include <kiplatform/policy.h>

// no system policy store on the Amiga: nothing is set by policy
KIPLATFORM::POLICY::PBOOL KIPLATFORM::POLICY::GetPolicyBool( const wxString& )
{
    return PBOOL::NOT_CONFIGURED;
}

std::optional<std::uint32_t> KIPLATFORM::POLICY::GetPolicyEnumUInt( const wxString& )
{
    return std::nullopt;
}
