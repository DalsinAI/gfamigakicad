/*
 * STEP, BREP, XAO, GLB, PLY, STL, U3D and PDF 3D export for the Amiga build
 * before OpenCASCADE is ported (stage 3): every export reports that it is
 * not available yet.
 * Copyright (c) 2026 AmiCradle Team
 */
#include "exporters/step/exporter_step.h"
#include <filename_resolver.h>
#include <pcb_track.h>

class STEP_PCB_MODEL {};

EXPORTER_STEP::EXPORTER_STEP( BOARD* aBoard, const EXPORTER_STEP_PARAMS& aParams,
                              REPORTER* aReporter ) :
        m_params( aParams ),
        m_reporter( aReporter ),
        m_board( aBoard ),
        m_platingThickness( 0 )
{
}

EXPORTER_STEP::~EXPORTER_STEP()
{
}

bool EXPORTER_STEP::Export()
{
    if( m_reporter )
        m_reporter->Report( _( "3D model export is not available in this Amiga build yet." ),
                            RPT_SEVERITY_ERROR );

    return false;
}

bool EXPORTER_STEP::IsViaHoleOpen( const PCB_VIA& aVia, bool aFillAllVias )
{
    if( aFillAllVias )
        return false;

    return aVia.GetFillingMode() != FILLING_MODE::FILLED
           && aVia.GetCappingMode() != CAPPING_MODE::CAPPED;
}
