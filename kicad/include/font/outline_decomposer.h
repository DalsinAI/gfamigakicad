/*
 * KiCad's glyph contours for the Amiga: made from gfopenfont.library's
 * outlines (OF_GlyphOutline) in place of FreeType's. Same CONTOUR and
 * GLYPH_DATA as upstream; the orientation is our own enum.
 * Copyright (c) 2026 AmiCradle Team
 */
#ifndef OUTLINE_DECOMPOSER_H
#define OUTLINE_DECOMPOSER_H

#include <vector>
#include <memory>
#include <math/box2.h>
#include <math/vector2d.h>
#include <font/glyph.h>
#include <geometry/shape_poly_set.h>

struct OFOutline;

namespace KIFONT
{

constexpr int GLYPH_DEFAULT_DPI = 72;

// as upstream: outlines are decomposed at 1152 dpi
constexpr int GLYPH_RESOLUTION  = 1152;
constexpr double GLYPH_SIZE_SCALER = GLYPH_DEFAULT_DPI / (double) GLYPH_RESOLUTION;

/// Which way round a glyph's filled contours go (FreeType's TrueType and
/// PostScript orientations)
enum class CONTOUR_ORIENTATION
{
    CLOCKWISE_FILLED,       // TrueType: outer contours clockwise, y up
    ANTICLOCKWISE_FILLED,   // PostScript/CFF
    NONE
};

struct CONTOUR
{
    std::vector<VECTOR2D> m_Points;
    int                   m_Winding = 0;
    CONTOUR_ORIENTATION   m_Orientation = CONTOUR_ORIENTATION::NONE;
};

struct GLYPH_DATA
{
    std::vector<CONTOUR> m_Contours;
    std::vector<std::unique_ptr<SHAPE_POLY_SET::TRIANGULATED_POLYGON>> m_TriangulationData;
};


class OUTLINE_DECOMPOSER
{
public:
    /**
     * @param aOutline   a glyph outline: 26.6 pixels, y up
     * @param aScale     what each 26.6 unit becomes in KiCad's glyph units
     * @param aShear     x += aShear * y (a fake italic), before scaling
     */
    OUTLINE_DECOMPOSER( const OFOutline& aOutline, double aScale, double aShear = 0.0 );

    bool OutlineToSegments( std::vector<CONTOUR>* aContours );

private:
    VECTOR2D point( int aIndex ) const;
    void     addContourPoint( const VECTOR2D& p );
    void     curveTo( const std::vector<VECTOR2D>& aBezier );
    int      winding( const std::vector<VECTOR2D>& aContour, double* aSum = nullptr ) const;

    const OFOutline&      m_outline;
    double                m_scale;
    double                m_shear;
    std::vector<CONTOUR>* m_contours;
};

} //namespace KIFONT

#endif      // OUTLINE_DECOMPOSER_H
