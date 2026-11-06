/* Amiga copy of KiCad's common/gal/cairo/cairo_print.cpp: drawn by gfopengpu.library through
 * gal/ogpu_canvas.h, not cairo (Copyright (c) 2026 AmiCradle Team for the changes) */
/*
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gal/cairo/cairo_print.h>

#include <algorithm>
#include <stdexcept>
#include <wx/bitmap.h>
#include <wx/dc.h>
#include <wx/image.h>
#include <wx/dcclient.h>
#include <wx/dcmemory.h>
#include <wx/dcprint.h>
#include <wx/rawbmp.h>


using namespace KIGFX;


CAIRO_PRINT_CTX::CAIRO_PRINT_CTX( wxImage* aImage, double aDPI ) :
        m_targetImage( aImage ),
        m_dpi( aDPI )
{
    wxCHECK( aImage, /* void */ );

    m_surface = ogc_image_surface_create( OGC_FORMAT_ARGB32, aImage->GetWidth(), aImage->GetHeight() );

    if( !m_surface || ogc_surface_status( m_surface ) != OGC_STATUS_SUCCESS )
        throw std::runtime_error( "Could not create Cairo surface" );

    m_ctx = ogc_create( m_surface );

    if( !m_ctx || ogc_status( m_ctx ) != OGC_STATUS_SUCCESS )
        throw std::runtime_error( "Could not create Cairo context" );

    ogc_set_antialias( m_ctx, OGC_ANTIALIAS_GOOD );
}


CAIRO_PRINT_CTX::CAIRO_PRINT_CTX( wxDC* aDC )
{
    // Amiga: drawn into a picture of the DC's size and resolution, which goes
    // onto the DC as a bitmap when the printout is done
    wxCHECK( aDC, /* void */ );

    wxSize size = aDC->GetSize();
    wxSize ppi = aDC->GetPPI();
    m_dpi = ppi.x > 0 ? ppi.x : 72.0;
    m_targetDC = aDC;
    m_ownImage = new wxImage( std::max( size.x, 1 ), std::max( size.y, 1 ), false );
    m_ownImage->SetAlpha();
    m_targetImage = m_ownImage;

    m_surface = ogc_image_surface_create( OGC_FORMAT_ARGB32, m_ownImage->GetWidth(),
                                          m_ownImage->GetHeight() );

    if( !m_surface || ogc_surface_status( m_surface ) != OGC_STATUS_SUCCESS )
        throw std::runtime_error( "Could not create the drawing surface" );

    m_ctx = ogc_create( m_surface );
    ogc_set_antialias( m_ctx, OGC_ANTIALIAS_GOOD );
}


CAIRO_PRINT_CTX::~CAIRO_PRINT_CTX()
{
    if( m_surface )
    {
        ogc_surface_flush( m_surface );

        if( m_targetImage )
        {
            // Convert data from cairo to wxImage
            unsigned char* srcData = ogc_image_surface_get_data( m_surface );
            int            height = ogc_image_surface_get_height( m_surface );
            int            stride = ogc_image_surface_get_stride( m_surface );

            unsigned char* dstRGB = m_targetImage->GetData();
            unsigned char* dstAlpha = m_targetImage->GetAlpha();
            unsigned char* srcRow = srcData;

            for( int y = 0; y < height; y++ )
            {
                for( int x = 0; x < stride; x += 4 )
                {
                    const uint32_t pix = *(uint32_t*) ( srcRow + x );

                    const uint8_t b = pix >> 0;
                    const uint8_t g = pix >> 8;
                    const uint8_t r = pix >> 16;
                    const uint8_t alpha = pix >> 24;

                    // Un-premultiply alpha
                    if( alpha == 0 )
                    {
                        dstRGB[0] = dstRGB[1] = dstRGB[2] = 0;
                        dstAlpha[0] = 0;
                    }
                    else
                    {
                        dstRGB[0] = ( (uint32_t) r * 255 ) / alpha;
                        dstRGB[1] = ( (uint32_t) g * 255 ) / alpha;
                        dstRGB[2] = ( (uint32_t) b * 255 ) / alpha;
                        dstAlpha[0] = alpha;
                    }

                    dstRGB += 3;
                    dstAlpha += 1;
                }

                srcRow += stride;
            }
        }

        ogc_surface_destroy( m_surface );
    }

    if( m_ctx )
        ogc_destroy( m_ctx );

    if( m_targetDC && m_ownImage )
    {
        m_targetDC->DrawBitmap( wxBitmap( *m_ownImage ), 0, 0, true );
        delete m_ownImage;
    }
}


CAIRO_PRINT_GAL::CAIRO_PRINT_GAL( GAL_DISPLAY_OPTIONS&             aDisplayOptions,
                                  std::unique_ptr<CAIRO_PRINT_CTX> aContext ) :
        CAIRO_GAL_BASE( aDisplayOptions )
{
    m_printCtx = std::move( aContext );
    m_context = m_currentContext = m_printCtx->GetContext();
    m_surface = m_printCtx->GetSurface();
    ogc_reference( m_context );
    ogc_surface_reference( m_surface );
    m_clearColor = COLOR4D( 1.0, 1.0, 1.0, 1.0 );
    m_hasNativeLandscapeRotation = false;
    resetContext();

    SetScreenDPI( m_printCtx->GetNativeDPI() );
}


void CAIRO_PRINT_GAL::ComputeWorldScreenMatrix()
{
    m_worldScale = m_screenDPI * m_worldUnitLength * m_zoomFactor;
    const VECTOR2D paperSizeIU = VECTOR2D( m_nativePaperSize.y, m_nativePaperSize.x ) /* inches */
                                 / m_worldUnitLength;                                 /* 1" in IU */
    const VECTOR2D paperSizeIUTransposed( paperSizeIU.y, paperSizeIU.x );

    MATRIX3x3D scale, translation, flip, rotate, lookat;

    scale.SetIdentity();
    translation.SetIdentity();
    flip.SetIdentity();
    rotate.SetIdentity();
    lookat.SetIdentity();

    if( m_hasNativeLandscapeRotation )
    {
        translation.SetTranslation( 0.5 / m_zoomFactor * paperSizeIUTransposed );
    }
    else
    {
        if( isLandscape() )
        {
            translation.SetTranslation( 0.5 / m_zoomFactor * paperSizeIU );
            rotate.SetRotation( 90.0 * M_PI / 180.0 );
        }
        else
        {
            translation.SetTranslation( 0.5 / m_zoomFactor * paperSizeIUTransposed );
        }
    }

    scale.SetScale( VECTOR2D( m_worldScale, m_worldScale ) );
    flip.SetScale( VECTOR2D( m_globalFlipX ? -1.0 : 1.0, m_globalFlipY ? -1.0 : 1.0 ) );
    lookat.SetTranslation( -m_lookAtPoint );

    m_worldScreenMatrix = scale * translation * flip * rotate * lookat;
    m_screenWorldMatrix = m_worldScreenMatrix.Inverse();
}


void CAIRO_PRINT_GAL::SetNativePaperSize( const VECTOR2D& aSize, bool aHasNativeLandscapeRotation )
{
    m_nativePaperSize = aSize;
    m_hasNativeLandscapeRotation = aHasNativeLandscapeRotation;
}


void CAIRO_PRINT_GAL::SetSheetSize( const VECTOR2D& aSize )
{
    // Convert aSize (inches) to pixels
    SetScreenSize( VECTOR2I( std::ceil( aSize.x * m_screenDPI ) * 2,
                             std::ceil( aSize.y * m_screenDPI ) * 2 ) );
}


std::unique_ptr<GAL_PRINT> GAL_PRINT::Create( GAL_DISPLAY_OPTIONS& aOptions, wxDC* aDC )
{
    auto printCtx = std::make_unique<CAIRO_PRINT_CTX>( aDC );
    return std::make_unique<CAIRO_PRINT_GAL>( aOptions, std::move( printCtx ) );
}


std::unique_ptr<CAIRO_PRINT_GAL> CAIRO_PRINT_GAL::Create( GAL_DISPLAY_OPTIONS& aOptions, wxImage* aImage, double aDPI )
{
    auto printCtx = std::make_unique<CAIRO_PRINT_CTX>( aImage, aDPI );
    return std::make_unique<CAIRO_PRINT_GAL>( aOptions, std::move( printCtx ) );
}