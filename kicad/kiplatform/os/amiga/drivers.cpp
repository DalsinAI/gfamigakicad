/*
 * KiCad platform layer for AmigaOS 3 (gfamigakicad).
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, as KiCad.
 */
#include <kiplatform/drivers.h>

// no 3Dconnexion driver on the Amiga
bool KIPLATFORM::DRIVERS::Valid3DConnexionDriverVersion() { return false; }
