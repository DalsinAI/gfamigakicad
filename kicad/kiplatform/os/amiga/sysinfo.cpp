/*
 * KiCad platform layer for AmigaOS 3 (gfamigakicad).
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, as KiCad.
 */
#include <kiplatform/sysinfo.h>
#include <proto/exec.h>
#include <exec/memory.h>

namespace KIPLATFORM {
SYSINFO::SYSINFO() {}
SYSINFO::~SYSINFO() {}

bool SYSINFO::GetMemoryInfo( MEMORY_INFO& aInfo )
{
    aInfo = MEMORY_INFO();
    aInfo.TotalPhysical = AvailMem( MEMF_TOTAL );
    aInfo.FreePhysical  = AvailMem( MEMF_ANY );
    if( aInfo.TotalPhysical )
        aInfo.Usage = (uint32_t) ( 100 - aInfo.FreePhysical * 100 / aInfo.TotalPhysical );
    return true;
}

bool SYSINFO::GetCPUInfo( std::vector<CPU_INFO>& aCpuInfos )
{
    CPU_INFO cpu;
    UWORD    flags = SysBase->AttnFlags;
    cpu.Manufacturer = "Motorola";
    cpu.Name = ( flags & 0x80 ) ? "68060" : ( flags & AFF_68040 ) ? "68040" : "68k";
    cpu.NumberCores = 1;
    cpu.NumberLogical = 1;
    aCpuInfos.push_back( cpu );
    return true;
}

bool SYSINFO::GetGPUInfo( std::vector<GPU_INFO>& ) { return false; }
}
