/*
 * KiCad platform layer for AmigaOS 3 (gfamigakicad).
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, as KiCad.
 */
#include <printing.h>

// no PDF print service (Poppler) on the Amiga yet; wx's own printing is used
namespace KIPLATFORM::PRINTING
{
PRINT_RESULT PrintPDF( const std::string& )
{
    return PRINT_RESULT::UNSUPPORTED;
}

void ResetPrintToFilePath( wxPrintData& )
{
}
}
