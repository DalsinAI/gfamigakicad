/* Amiga copy of KiCad's common/gal/cairo/cairo_compositor.cpp: drawn by gfopengpu.library through
 * gal/ogpu_canvas.h, not cairo (Copyright (c) 2026 AmiCradle Team for the changes) */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http:O//www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file cairo_compositor.cpp
 * @brief Class that handles multitarget rendering (ie. to different textures/surfaces) and
 * later compositing into a single image (Cairo flavour).
 */

#include <gal/cairo/cairo_compositor.h>
#include <wx/log.h>

using namespace KIGFX;

CAIRO_COMPOSITOR::CAIRO_COMPOSITOR( ogc_t** aMainContext ) :
        m_current( 0 ),
        m_currentContext( aMainContext ),
        m_mainContext( *aMainContext ),
        m_currentAntialiasingMode( OGC_ANTIALIAS_DEFAULT )
{
    // Do not have uninitialized members:
    ogc_matrix_init_identity( &m_matrix );
    m_stride = 0;
    m_bufferSize = 0;
}


CAIRO_COMPOSITOR::~CAIRO_COMPOSITOR()
{
    clean();
}


void CAIRO_COMPOSITOR::Initialize()
{
    // Nothing has to be done
}


void CAIRO_COMPOSITOR::SetAntialiasingMode( GAL_ANTIALIASING_MODE aMode )
{
    switch( aMode )
    {
    case GAL_ANTIALIASING_MODE::AA_FAST: m_currentAntialiasingMode = OGC_ANTIALIAS_FAST; break;
    case GAL_ANTIALIASING_MODE::AA_HIGHQUALITY: m_currentAntialiasingMode = OGC_ANTIALIAS_GOOD; break;
    default: m_currentAntialiasingMode = OGC_ANTIALIAS_NONE;
    }

    clean();
}


void CAIRO_COMPOSITOR::Resize( unsigned int aWidth, unsigned int aHeight )
{
    clean();

    m_width = aWidth;
    m_height = aHeight;

    m_stride = ogc_format_stride_for_width( OGC_FORMAT_ARGB32, m_width );
    m_bufferSize = m_stride * m_height;
}


unsigned int CAIRO_COMPOSITOR::CreateBuffer()
{
    // Pixel storage
    BitmapPtr bitmap = new uint8_t[m_bufferSize]();

    // Create the Cairo surface
    ogc_surface_t* surface = ogc_image_surface_create_for_data(
            (unsigned char*) bitmap, OGC_FORMAT_ARGB32, m_width, m_height, m_stride );
    ogc_t* context = ogc_create( surface );

#ifdef DEBUG
    ogc_status_t status = ogc_status( context );
    wxASSERT_MSG( status == OGC_STATUS_SUCCESS, wxT( "Cairo context creation error" ) );
#endif /* DEBUG */

    // Set default settings for the buffer
    ogc_set_antialias( context, m_currentAntialiasingMode );

    // Use the same transformation matrix as the main context
    ogc_get_matrix( m_mainContext, &m_matrix );
    ogc_set_matrix( context, &m_matrix );

    // Store the new buffer
    CAIRO_BUFFER buffer = { context, surface, bitmap };
    m_buffers.push_back( buffer );

    return usedBuffers();
}


void CAIRO_COMPOSITOR::SetBuffer( unsigned int aBufferHandle )
{
    wxASSERT_MSG( aBufferHandle <= usedBuffers(), wxT( "Tried to use a not existing buffer" ) );

    // Get currently used transformation matrix, so it can be applied to the new buffer
    ogc_get_matrix( *m_currentContext, &m_matrix );

    m_current = aBufferHandle - 1;
    *m_currentContext = m_buffers[m_current].context;

    // Apply the current transformation matrix
    ogc_set_matrix( *m_currentContext, &m_matrix );
}


void CAIRO_COMPOSITOR::Begin()
{
}


void CAIRO_COMPOSITOR::ClearBuffer( const COLOR4D& aColor )
{
    // Clear the pixel storage (Amiga: in the stream, after what draws there now)
    ogc_surface_clear( m_buffers[m_current].surface );
}


void CAIRO_COMPOSITOR::DrawBuffer( unsigned int aSourceHandle, unsigned int aDestHandle,
                                   ogc_operator_t op )
{
    wxASSERT_MSG( aSourceHandle <= usedBuffers() && aDestHandle <= usedBuffers(),
                  wxT( "Tried to use a not existing buffer" ) );

    // Reset the transformation matrix, so it is possible to composite images using
    // screen coordinates instead of world coordinates
    ogc_get_matrix( m_mainContext, &m_matrix );
    ogc_identity_matrix( m_mainContext );

    // Draw the selected buffer contents
    ogc_t* ct = ogc_create( m_buffers[aDestHandle - 1].surface );
    ogc_set_operator( ct, op );
    ogc_set_source_surface( ct, m_buffers[aSourceHandle - 1].surface, 0.0, 0.0 );
    ogc_paint( ct );
    ogc_destroy( ct );

    // Restore the transformation matrix
    ogc_set_matrix( m_mainContext, &m_matrix );
}


void CAIRO_COMPOSITOR::DrawBuffer( unsigned int aBufferHandle )
{
    wxASSERT_MSG( aBufferHandle <= usedBuffers(), wxT( "Tried to use a not existing buffer" ) );

    // Reset the transformation matrix, so it is possible to composite images using
    // screen coordinates instead of world coordinates
    ogc_get_matrix( m_mainContext, &m_matrix );
    ogc_identity_matrix( m_mainContext );

    // Draw the selected buffer contents
    ogc_set_source_surface( m_mainContext, m_buffers[aBufferHandle - 1].surface, 0.0, 0.0 );
    ogc_paint( m_mainContext );

    // Restore the transformation matrix
    ogc_set_matrix( m_mainContext, &m_matrix );
}


void CAIRO_COMPOSITOR::Present()
{
}


void CAIRO_COMPOSITOR::clean()
{
    CAIRO_BUFFERS::const_iterator it;

    ogc_sync();     // Amiga: nothing still drawing into them

    for( it = m_buffers.begin(); it != m_buffers.end(); ++it )
    {
        ogc_destroy( it->context );
        ogc_surface_destroy( it->surface );
        delete[] it->bitmap;
    }

    m_buffers.clear();
}
