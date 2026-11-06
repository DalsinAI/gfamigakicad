/*
 * KiCad's outline fonts on the Amiga: gfopenfont.library finds, shapes and
 * outlines the glyphs in place of FreeType and HarfBuzz. The glyph geometry,
 * caches, tofu boxes and fake styles follow upstream's outline_font.cpp.
 * Copyright (c) 2026 AmiCradle Team (this file); KiCad's own code it follows
 * is Copyright The KiCad Developers, GPL 3 or later.
 */
#include <limits>
#include <cmath>
#include <bezier_curves.h>
#include <geometry/shape_poly_set.h>
#include <font/fontconfig.h>
#include <font/outline_font.h>
#include <trigo.h>
#include <core/utf8.h>
#include <hash.h>

#include <font/gfopenfont_kicad.h>

#include <exec/types.h>
#include <libraries/gfopenfont.h>
#include <proto/gfopenfont.h>

using namespace KIFONT;

std::mutex OUTLINE_FONT::m_freeTypeMutex;

/// FindFont's answer for an installed family (see fontconfig_gfopenfont.cpp)
static const wxString s_installedPrefix = wxS( "gfopenfont:" );


OUTLINE_FONT::OUTLINE_FONT() :
        m_face( nullptr ),
        m_faceBold( false ),
        m_faceItalic( false ),
        m_faceFixed( false ),
        m_faceSize( 16 ),
        m_fakeBold( false ),
        m_fakeItal( false ),
        m_forDrawingSheet( false )
{
}


OUTLINE_FONT::~OUTLINE_FONT()
{
    if( m_face )
    {
        std::lock_guard<std::mutex> guard( m_freeTypeMutex );
        OF_CloseFace( m_face );
    }
}


OUTLINE_FONT::EMBEDDING_PERMISSION OUTLINE_FONT::GetEmbeddingPermission() const
{
    // OS/2 fsType, at offset 8
    unsigned fsType = gfopenfont::TableU16( m_face, "OS/2", 8, 0xffff );

    if( fsType == 0xffff )
        return EMBEDDING_PERMISSION::RESTRICTED;

    if( fsType & 0x0200 )                   // bitmap embedding only
        return EMBEDDING_PERMISSION::RESTRICTED;

    unsigned embeddingBits = fsType & 0x000F;

    if( embeddingBits == 0 )                // installable
        return EMBEDDING_PERMISSION::INSTALLABLE;

    if( embeddingBits & 0x0008 )            // editable
        return EMBEDDING_PERMISSION::EDITABLE;

    if( embeddingBits & 0x0004 )            // preview and print
        return EMBEDDING_PERMISSION::PRINT_PREVIEW_ONLY;

    return EMBEDDING_PERMISSION::RESTRICTED;
}


OUTLINE_FONT* OUTLINE_FONT::LoadFont( const wxString& aFontName, bool aBold, bool aItalic,
                                      const std::vector<wxString>* aEmbeddedFiles,
                                      bool aForDrawingSheet )
{
    std::unique_ptr<OUTLINE_FONT> font = std::make_unique<OUTLINE_FONT>();

    wxString fontFile;
    int      faceIndex;
    using fc = fontconfig::FONTCONFIG;

    fc::FF_RESULT retval = Fontconfig()->FindFont( aFontName, fontFile, faceIndex, aBold, aItalic,
                                                   aEmbeddedFiles );

    if( retval == fc::FF_RESULT::FF_ERROR )
        return nullptr;

    if( retval == fc::FF_RESULT::FF_MISSING_BOLD || retval == fc::FF_RESULT::FF_MISSING_BOLD_ITAL )
        font->SetFakeBold();

    if( retval == fc::FF_RESULT::FF_MISSING_ITAL || retval == fc::FF_RESULT::FF_MISSING_BOLD_ITAL )
        font->SetFakeItal();

    if( font->loadFace( fontFile, faceIndex, aBold, aItalic ) != 0 )
        return nullptr;

    // an installed family's answer carries its file after a newline
    if( fontFile.StartsWith( s_installedPrefix ) )
        fontFile = fontFile.AfterFirst( '\n' );

    font->m_fontName = aFontName;       // Keep asked-for name, even if we substituted.
    font->m_fontFileName = fontFile;
    font->m_forDrawingSheet = aForDrawingSheet;

    return font.release();
}


/// the pixel size FreeType would use for a char size of aCharSize (26.6
/// points) at GLYPH_RESOLUTION dpi, and gfopenfont's whole-pixel size for it
static ULONG pixelSize( double aCharSize, double* aCorrection )
{
    double exact = aCharSize / 64.0 * GLYPH_RESOLUTION / 72.0;
    ULONG  px = std::max<ULONG>( 1, (ULONG) std::lround( exact ) );

    if( aCorrection )
        *aCorrection = exact / px;

    return px;
}


int OUTLINE_FONT::loadFace( const wxString& aFontFileName, int aFaceIndex, bool aBold, bool aItalic )
{
    if( !gfopenfont::Ready() )
        return -1;

    std::lock_guard<std::mutex> guard( m_freeTypeMutex );

    ULONG px = pixelSize( faceSize(), nullptr );

    if( aFontFileName.StartsWith( s_installedPrefix ) )
    {
        wxString family = aFontFileName.Mid( s_installedPrefix.length() ).BeforeFirst( '\n' );

        // the default fallback chain stays on, so text the face lacks still shows
        m_face = OF_FindFace( (CONST_STRPTR) family.utf8_str().data(),
                              OFA_Weight, aBold ? 700 : 400, OFA_Italic, aItalic ? TRUE : FALSE,
                              OFA_Size, px, TAG_DONE );
    }
    else
    {
        m_face = OF_OpenFace( (CONST_STRPTR) aFontFileName.utf8_str().data(),
                              OFA_Index, (ULONG) aFaceIndex, OFA_Size, px, TAG_DONE );
    }

    if( !m_face )
        return -1;

    struct OFFaceInfo info;

    if( OF_GetFaceInfo( m_face, &info ) == OFERR_OK )
    {
        m_faceStyle = wxString::FromUTF8( info.fi_Style );
        m_faceItalic = ( info.fi_Flags & OFFF_ITALIC ) != 0;
        m_faceFixed = ( info.fi_Flags & OFFF_FIXED ) != 0;
        m_faceBold = info.fi_Weight >= 600;
    }

    return 0;
}


double OUTLINE_FONT::GetInterline( double aGlyphHeight, const METRICS& aFontMetrics ) const
{
    return aFontMetrics.GetInterline( aGlyphHeight );
}


static bool contourIsFilled( const CONTOUR& c )
{
    switch( c.m_Orientation )
    {
    case CONTOUR_ORIENTATION::CLOCKWISE_FILLED:     return c.m_Winding == 1;
    case CONTOUR_ORIENTATION::ANTICLOCKWISE_FILLED: return c.m_Winding == -1;
    default:                                        return false;
    }
}


static bool contourIsHole( const CONTOUR& c )
{
    return !contourIsFilled( c );
}


/**
 * FreeType's FT_Outline_Embolden, for a fake bold: every contour moves out
 * from the ink by half of aStrength, at each corner along the mitre.
 */
static void embolden( std::vector<CONTOUR>& aContours, double aStrength )
{
    const double d = aStrength / 2.0;

    for( CONTOUR& c : aContours )
    {
        size_t n = c.m_Points.size();

        if( n < 3 )
            continue;

        // the ink is to the right of the way a TrueType contour goes, y up
        double side = c.m_Orientation == CONTOUR_ORIENTATION::ANTICLOCKWISE_FILLED ? -1.0 : 1.0;

        std::vector<VECTOR2D> out( n );

        for( size_t i = 0; i < n; i++ )
        {
            const VECTOR2D& p0 = c.m_Points[( i + n - 1 ) % n];
            const VECTOR2D& p1 = c.m_Points[i];
            const VECTOR2D& p2 = c.m_Points[( i + 1 ) % n];

            VECTOR2D e1 = p1 - p0;
            VECTOR2D e2 = p2 - p1;
            double   l1 = e1.EuclideanNorm();
            double   l2 = e2.EuclideanNorm();

            if( l1 == 0.0 || l2 == 0.0 )
            {
                out[i] = p1;
                continue;
            }

            // the left normals, away from the ink
            VECTOR2D n1( -e1.y / l1 * side, e1.x / l1 * side );
            VECTOR2D n2( -e2.y / l2 * side, e2.x / l2 * side );
            double   dot = n1.x * n2.x + n1.y * n2.y;
            double   k = 1.0 + dot;

            // a mitre no longer than four times the shift, as a sharp corner would spike
            if( k < 0.125 )
                k = 0.125;

            out[i] = p1 + VECTOR2D( ( n1.x + n2.x ) * d / k, ( n1.y + n2.y ) * d / k );
        }

        c.m_Points = std::move( out );
    }
}


BOX2I OUTLINE_FONT::getBoundingBox( const std::vector<std::unique_ptr<GLYPH>>& aGlyphs ) const
{
    int minX = INT_MAX;
    int minY = INT_MAX;
    int maxX = INT_MIN;
    int maxY = INT_MIN;

    for( const std::unique_ptr<KIFONT::GLYPH>& glyph : aGlyphs )
    {
        BOX2D bbox = glyph->BoundingBox();
        bbox.Normalize();

        if( minX > bbox.GetX() )
            minX = bbox.GetX();

        if( minY > bbox.GetY() )
            minY = bbox.GetY();

        if( maxX < bbox.GetRight() )
            maxX = bbox.GetRight();

        if( maxY < bbox.GetBottom() )
            maxY = bbox.GetBottom();
    }

    BOX2I ret;
    ret.SetOrigin( minX, minY );
    ret.SetEnd( maxX, maxY );
    return ret;
}


void OUTLINE_FONT::GetLinesAsGlyphs( std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                                     const wxString& aText, const VECTOR2I& aPosition,
                                     const TEXT_ATTRIBUTES& aAttrs,
                                     const METRICS& aFontMetrics ) const
{
    wxArrayString         strings;
    std::vector<VECTOR2I> positions;
    std::vector<VECTOR2I> extents;
    TEXT_STYLE_FLAGS      textStyle = 0;

    if( aAttrs.m_Italic )
        textStyle |= TEXT_STYLE::ITALIC;

    getLinePositions( aText, aPosition, strings, positions, extents, aAttrs, aFontMetrics );

    for( size_t i = 0; i < strings.GetCount(); i++ )
    {
        (void) drawMarkup( nullptr, aGlyphs, strings.Item( i ), positions[i], aAttrs.m_Size,
                           aAttrs.m_Angle, aAttrs.m_Mirrored, aPosition, textStyle, aFontMetrics );
    }
}


VECTOR2I OUTLINE_FONT::GetTextAsGlyphs( BOX2I* aBBox, std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                                        const wxString& aText, const VECTOR2I& aSize,
                                        const VECTOR2I& aPosition, const EDA_ANGLE& aAngle,
                                        bool aMirror, const VECTOR2I& aOrigin,
                                        TEXT_STYLE_FLAGS aTextStyle ) const
{
    // Width of a tab in em's, as upstream
    constexpr double TAB_WIDTH = 4 * 0.6;

    VECTOR2I position = aPosition;
    wxString textRun;

    if( aBBox )
    {
        aBBox->SetOrigin( aPosition );
        aBBox->SetEnd( aPosition );
    }

    for( wxUniChar c : aText )
    {
        if( c == '\t' )
        {
            if( !textRun.IsEmpty() )
            {
                position = getTextAsGlyphs( aBBox, aGlyphs, textRun, aSize, position, aAngle,
                                            aMirror, aOrigin, aTextStyle );
                textRun.clear();
            }

            int tabWidth = KiROUND( aSize.x * TAB_WIDTH );
            int currentIntrusion = ( position.x - aOrigin.x ) % tabWidth;

            position.x += tabWidth - currentIntrusion;
        }
        else
        {
            textRun += c;
        }
    }

    if( !textRun.IsEmpty() )
    {
        position = getTextAsGlyphs( aBBox, aGlyphs, textRun, aSize, position, aAngle, aMirror,
                                    aOrigin, aTextStyle );
    }

    return position;
}


VECTOR2I OUTLINE_FONT::getTextAsGlyphs( BOX2I* aBBox, std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                                        const wxString& aText, const VECTOR2I& aSize,
                                        const VECTOR2I& aPosition, const EDA_ANGLE& aAngle,
                                        bool aMirror, const VECTOR2I& aOrigin,
                                        TEXT_STYLE_FLAGS aTextStyle ) const
{
    std::lock_guard<std::mutex> guard( m_freeTypeMutex );

    return getTextAsGlyphsUnlocked( aBBox, aGlyphs, aText, aSize, aPosition, aAngle, aMirror,
                                    aOrigin, aTextStyle );
}


struct SHAPE_CACHE_KEY
{
    OFFace*     face;
    std::string text;
    ULONG       pixels;

    bool operator==( const SHAPE_CACHE_KEY& rhs ) const
    {
        return face == rhs.face && pixels == rhs.pixels && text == rhs.text;
    }
};


struct SHAPE_CACHE_ENTRY
{
    std::vector<struct OFGlyph> m_Glyphs;
    bool                        m_Initialized = false;
};


namespace std
{
    template <>
    struct hash<SHAPE_CACHE_KEY>
    {
        std::size_t operator()( const SHAPE_CACHE_KEY& k ) const
        {
            return hash_val( k.face, k.pixels, k.text );
        }
    };
}


static const SHAPE_CACHE_ENTRY& getShape( OFFace* aFace, const wxString& aText, ULONG aPixels )
{
    static std::unordered_map<SHAPE_CACHE_KEY, SHAPE_CACHE_ENTRY> s_shapeCache;

    std::string     textUtf8 = UTF8( aText );
    SHAPE_CACHE_KEY key = { aFace, textUtf8, aPixels };

    SHAPE_CACHE_ENTRY& entry = s_shapeCache[key];

    if( !entry.m_Initialized )
    {
        ULONG count = OF_ShapeText( aFace, (CONST_STRPTR) textUtf8.c_str(), textUtf8.size(),
                                    nullptr, 0 );

        if( count )
        {
            entry.m_Glyphs.resize( count );
            count = OF_ShapeText( aFace, (CONST_STRPTR) textUtf8.c_str(), textUtf8.size(),
                                  entry.m_Glyphs.data(), count );
            entry.m_Glyphs.resize( std::min<size_t>( count, entry.m_Glyphs.size() ) );
        }

        entry.m_Initialized = true;
    }

    return entry;
}


struct GLYPH_CACHE_KEY {
    OFFace*        face;
    ULONG          glyph;
    VECTOR2D       scale;
    bool           forDrawingSheet;
    bool           fakeItalic;
    bool           fakeBold;
    bool           mirror;
    bool           supersub;
    EDA_ANGLE      angle;

    bool operator==(const GLYPH_CACHE_KEY& rhs ) const
    {
        return face == rhs.face
                && glyph == rhs.glyph
                && scale == rhs.scale
                && forDrawingSheet == rhs.forDrawingSheet
                && fakeItalic == rhs.fakeItalic
                && fakeBold == rhs.fakeBold
                && mirror == rhs.mirror
                && supersub == rhs.supersub
                && angle == rhs.angle;
    }
};


namespace std
{
    template <>
    struct hash<GLYPH_CACHE_KEY>
    {
        std::size_t operator()( const GLYPH_CACHE_KEY& k ) const
        {
            return hash_val( k.face, k.glyph, k.scale.x, k.scale.y, k.forDrawingSheet,
                             k.fakeItalic, k.fakeBold, k.mirror, k.supersub, k.angle.AsDegrees() );
        }
    };
}


/// a glyph's outline as contours in KiCad's glyph units
static bool glyphContours( OFFace* aFace, ULONG aGlyph, double aScale, double aShear,
                           std::vector<CONTOUR>* aContours )
{
    struct OFOutline outline = {};

    // measure, then fill
    LONG rc = OF_GlyphOutline( aFace, aGlyph, &outline );

    if( ( rc != OFERR_OK && rc != OFERR_BUFFER ) || outline.ol_NumVerbs == 0 )
        return false;

    std::vector<UBYTE> verbs( outline.ol_NumVerbs );
    std::vector<LONG>  points( outline.ol_NumPoints * 2 + 2 );

    outline.ol_Verbs = verbs.data();
    outline.ol_Points = points.data();
    outline.ol_MaxVerbs = verbs.size();
    outline.ol_MaxPoints = outline.ol_NumPoints;

    if( OF_GlyphOutline( aFace, aGlyph, &outline ) != OFERR_OK )
        return false;

    OUTLINE_DECOMPOSER decomposer( outline, aScale, aShear );
    return decomposer.OutlineToSegments( aContours );
}


VECTOR2I OUTLINE_FONT::getTextAsGlyphsUnlocked( BOX2I* aBBox,
                                                std::vector<std::unique_ptr<GLYPH>>* aGlyphs,
                                                const wxString& aText, const VECTOR2I& aSize,
                                                const VECTOR2I& aPosition, const EDA_ANGLE& aAngle,
                                                bool aMirror, const VECTOR2I& aOrigin,
                                                TEXT_STYLE_FLAGS aTextStyle ) const
{
    VECTOR2D glyphSize = aSize;
    OFFace*  face = m_face;
    double   scaler = faceSize();
    bool     supersub = IsSuperscript( aTextStyle ) || IsSubscript( aTextStyle );

    if( supersub )
        scaler = subscriptSize();

    // gfopenfont sizes in whole pixels: what FreeType's 26.6 size gives is
    // made up by scaling the answers
    double correction = 1.0;
    ULONG  pixels = pixelSize( scaler, &correction );
    double unit = GLYPH_SIZE_SCALER * correction;   // one 26.6 unit in glyph units

    OF_SetSize( face, pixels );

    const SHAPE_CACHE_ENTRY& shape = getShape( face, aText, pixels );
    size_t                   glyphCount = shape.m_Glyphs.size();

    VECTOR2D scaleFactor( glyphSize.x / faceSize(), -glyphSize.y / faceSize() );
    scaleFactor = scaleFactor * m_outlineFontSizeCompensation;

    VECTOR2D origin( aPosition );

    if( supersub )
        origin.y += glyphSize.y * GetSuperSubSizeMultiplier() * GetSuperSubBaselineOffset( aTextStyle );

    VECTOR2D cursor( 0, 0 );

    if( aGlyphs )
        aGlyphs->reserve( glyphCount );

    // Cache of glyphs converted to straight segments, as upstream
    static std::unordered_map<GLYPH_CACHE_KEY, GLYPH_DATA> s_glyphCache;

    // FreeType's fake italic: a 12 degree shear
    const double shear = m_fakeItal ? std::tan( M_PI * 12.0 / 180.0 ) : 0.0;

    for( size_t i = 0; i < glyphCount; i++ )
    {
        const struct OFGlyph& g = shape.m_Glyphs[i];

        if( aGlyphs )
        {
            OFFace* glyphFace = g.gl_Face ? OF_GetFallback( face, g.gl_Face ) : face;

            if( !glyphFace )
                glyphFace = face;

            GLYPH_CACHE_KEY key = { glyphFace, g.gl_Glyph, scaleFactor, m_forDrawingSheet,
                                    m_fakeItal, m_fakeBold, aMirror, supersub, aAngle };
            GLYPH_DATA&     glyphData = s_glyphCache[ key ];

            if( glyphData.m_Contours.empty() )
            {
                if( glyphFace != face )
                    OF_SetSize( glyphFace, pixels );

                bool ok = !( g.gl_Flags & OFGF_MISSING )
                          && glyphContours( glyphFace, g.gl_Glyph, unit, shear,
                                            &glyphData.m_Contours );

                if( ok && m_fakeBold )
                    embolden( glyphData.m_Contours, 64.0 * unit );

                if( !ok )
                {
                    // a tofu box, as upstream
                    double  advance = g.gl_XAdvance * unit;
                    BOX2D   tofuBox( { scaler * 0.03, 0.0 },
                                     { advance - scaler * 0.02, scaler * 0.72 } );

                    glyphData.m_Contours.clear();

                    CONTOUR outline;
                    outline.m_Winding = 1;
                    outline.m_Orientation = CONTOUR_ORIENTATION::CLOCKWISE_FILLED;
                    outline.m_Points.push_back( tofuBox.GetPosition() );
                    outline.m_Points.push_back( { tofuBox.GetSize().x, tofuBox.GetPosition().y } );
                    outline.m_Points.push_back( tofuBox.GetSize() );
                    outline.m_Points.push_back( { tofuBox.GetPosition().x, tofuBox.GetSize().y } );
                    glyphData.m_Contours.push_back( std::move( outline ) );

                    CONTOUR hole;
                    tofuBox.Move( { scaler * 0.06, scaler * 0.06 } );
                    tofuBox.SetSize( { tofuBox.GetWidth() - scaler * 0.06,
                                       tofuBox.GetHeight() - scaler * 0.06 } );
                    hole.m_Winding = 1;
                    hole.m_Orientation = CONTOUR_ORIENTATION::NONE;
                    hole.m_Points.push_back( tofuBox.GetPosition() );
                    hole.m_Points.push_back( { tofuBox.GetSize().x, tofuBox.GetPosition().y } );
                    hole.m_Points.push_back( tofuBox.GetSize() );
                    hole.m_Points.push_back( { tofuBox.GetPosition().x, tofuBox.GetSize().y } );
                    glyphData.m_Contours.push_back( std::move( hole ) );
                }
            }

            std::unique_ptr<OUTLINE_GLYPH> glyph = std::make_unique<OUTLINE_GLYPH>();
            std::vector<SHAPE_LINE_CHAIN>  holes;

            // a mark's own offset from the pen
            VECTOR2D pen( cursor.x + g.gl_XOffset * unit, cursor.y + g.gl_YOffset * unit );

            for( CONTOUR& c : glyphData.m_Contours )
            {
                SHAPE_LINE_CHAIN shape;

                shape.ReservePoints( c.m_Points.size() );

                for( const VECTOR2D& v : c.m_Points )
                {
                    VECTOR2D pt( v + pen );

                    pt *= scaleFactor;
                    pt += origin;

                    if( aMirror )
                        pt.x = aOrigin.x - ( pt.x - aOrigin.x );

                    if( !aAngle.IsZero() )
                        RotatePoint( pt, aOrigin, aAngle );

                    shape.Append( pt.x, pt.y );
                }

                shape.SetClosed( true );

                if( contourIsHole( c ) )
                    holes.push_back( std::move( shape ) );
                else
                    glyph->AddOutline( std::move( shape ) );
            }

            for( SHAPE_LINE_CHAIN& hole : holes )
            {
                bool added_hole = false;

                if( hole.PointCount() )
                {
                    for( int ii = 0; ii < glyph->OutlineCount(); ++ii )
                    {
                        if( glyph->Outline( ii ).PointInside( hole.GetPoint( 0 ) ) )
                        {
                            glyph->AddHole( std::move( hole ), ii );
                            added_hole = true;
                            break;
                        }
                    }

                    if( !added_hole )
                        glyph->AddOutline( std::move( hole ) );
                }
            }

            if( glyphData.m_TriangulationData.empty() )
            {
                glyph->CacheTriangulation( false, false );
                glyphData.m_TriangulationData = glyph->GetTriangulationData();
            }
            else
            {
                glyph->CacheTriangulation( glyphData.m_TriangulationData );
            }

            aGlyphs->push_back( std::move( glyph ) );
        }

        cursor.x += g.gl_XAdvance * unit;
        cursor.y += g.gl_YAdvance * unit;
    }

    int ascender = 0;
    int descender = 0;
    struct OFFaceInfo info;

    if( OF_GetFaceInfo( face, &info ) == OFERR_OK )
    {
        ascender = abs( KiROUND( info.fi_Ascender * unit ) );
        descender = abs( KiROUND( info.fi_Descender * unit ) );
    }

    if( aBBox )
    {
        aBBox->Merge( aPosition - VECTOR2I( 0, ascender * abs( scaleFactor.y ) ) );
        aBBox->Merge( aPosition + VECTOR2I( cursor.x * scaleFactor.x, descender * abs( scaleFactor.y ) ) );
    }

    return VECTOR2I( aPosition.x + cursor.x * scaleFactor.x, aPosition.y - cursor.y * scaleFactor.y );
}
