/*
 * KiCad's glyph contours from gfopenfont.library's outlines.
 * Copyright (c) 2026 AmiCradle Team
 */
#include <advanced_config.h>
#include <font/outline_decomposer.h>
#include <bezier_curves.h>

#include <exec/types.h>
#include <libraries/gfopenfont.h>

using namespace KIFONT;


OUTLINE_DECOMPOSER::OUTLINE_DECOMPOSER( const OFOutline& aOutline, double aScale, double aShear ) :
        m_outline( aOutline ),
        m_scale( aScale ),
        m_shear( aShear ),
        m_contours( nullptr )
{
}


VECTOR2D OUTLINE_DECOMPOSER::point( int aIndex ) const
{
    double x = m_outline.ol_Points[aIndex * 2];
    double y = m_outline.ol_Points[aIndex * 2 + 1];

    return VECTOR2D( ( x + m_shear * y ) * m_scale, y * m_scale );
}


void OUTLINE_DECOMPOSER::addContourPoint( const VECTOR2D& p )
{
    if( m_contours->back().m_Points.empty() || m_contours->back().m_Points.back() != p )
        m_contours->back().m_Points.push_back( p );
}


void OUTLINE_DECOMPOSER::curveTo( const std::vector<VECTOR2D>& aBezier )
{
    std::vector<VECTOR2D> result;
    BEZIER_POLY           converter( aBezier );
    converter.GetPoly( result, ADVANCED_CFG::GetCfg().m_FontErrorSize );

    for( const VECTOR2D& p : result )
        addContourPoint( p );

    addContourPoint( aBezier.back() );
}


bool OUTLINE_DECOMPOSER::OutlineToSegments( std::vector<CONTOUR>* aContours )
{
    m_contours = aContours;

    if( !m_outline.ol_Verbs || m_outline.ol_NumVerbs == 0
            || m_outline.ol_NumVerbs > m_outline.ol_MaxVerbs
            || m_outline.ol_NumPoints > m_outline.ol_MaxPoints )
    {
        return false;
    }

    VECTOR2D last;
    int      pt = 0;

    for( ULONG v = 0; v < m_outline.ol_NumVerbs; v++ )
    {
        switch( m_outline.ol_Verbs[v] )
        {
        case OFPV_MOVE:
            last = point( pt++ );
            m_contours->emplace_back();
            addContourPoint( last );
            break;

        case OFPV_LINE:
            if( m_contours->empty() )
                return false;

            last = point( pt++ );
            addContourPoint( last );
            break;

        case OFPV_QUAD:
            if( m_contours->empty() )
                return false;

            curveTo( { last, point( pt ), point( pt + 1 ) } );
            last = point( pt + 1 );
            pt += 2;
            break;

        case OFPV_CUBIC:
            if( m_contours->empty() )
                return false;

            curveTo( { last, point( pt ), point( pt + 1 ), point( pt + 2 ) } );
            last = point( pt + 2 );
            pt += 3;
            break;

        case OFPV_CLOSE:
            break;

        default:
            return false;
        }
    }

    // FreeType's FT_Outline_Get_Orientation: the sign of the whole outline's area
    double total = 0.0;

    for( CONTOUR& c : *m_contours )
    {
        double sum = 0.0;
        c.m_Winding = winding( c.m_Points, &sum );
        total += sum;
    }

    CONTOUR_ORIENTATION orientation = total > 0.0 ? CONTOUR_ORIENTATION::CLOCKWISE_FILLED
                                    : total < 0.0 ? CONTOUR_ORIENTATION::ANTICLOCKWISE_FILLED
                                                  : CONTOUR_ORIENTATION::NONE;

    for( CONTOUR& c : *m_contours )
        c.m_Orientation = orientation;

    return !m_contours->empty();
}


int OUTLINE_DECOMPOSER::winding( const std::vector<VECTOR2D>& aContour, double* aSum ) const
{
    // -1 == counterclockwise, 1 == clockwise (y up), as upstream
    if( aContour.size() < 2 )
        return 0;

    double sum = 0.0;
    size_t len = aContour.size();

    for( size_t i = 0; i < len - 1; i++ )
    {
        const VECTOR2D& p1 = aContour[i];
        const VECTOR2D& p2 = aContour[i + 1];
        sum += ( p2.x - p1.x ) * ( p2.y + p1.y );
    }

    sum += ( aContour[0].x - aContour[len - 1].x ) * ( aContour[0].y + aContour[len - 1].y );

    if( aSum )
        *aSum = sum;

    return sum > 0.0 ? 1 : sum < 0.0 ? -1 : 0;
}
