/* KiCad's canvas on gfopengpu.library: see ogpu_canvas.h.
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, as KiCad. */

#include <gal/ogpu_canvas.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/libraries.h>
#include <proto/exec.h>
#include <gfopengpu/gfopengpu.h>
#include <proto/gfopengpu.h>

struct Library* OpenGPUBase = nullptr;

static constexpr double kPi = 3.14159265358979323846;

namespace
{

enum : unsigned char
{
    V_MOVE = OGPU_PATH_MOVE,
    V_LINE = OGPU_PATH_LINE,
    V_CUBIC = OGPU_PATH_CUBIC,
    V_CLOSE = OGPU_PATH_CLOSE
};

struct ELEM
{
    unsigned char verb;
    double        p[6];
};

} // namespace


struct ogc_surface_t
{
    int            refs = 1;
    int            width = 0;
    int            height = 0;
    int            stride = 0;
    unsigned char* data = nullptr;
    bool           owned = false;
    unsigned long  batch = 0;   // the batch that last drew with it (0: none)
};


struct ogc_path_t
{
    std::vector<ELEM> elems;    // user space
};


namespace
{

struct STATE
{
    ogc_matrix_t    ctm;
    double          r = 0, g = 0, b = 0, a = 1;
    ogc_surface_t*  source = nullptr;   // a surface source (held), else the colour
    ogc_matrix_t    sourceMatrix;       // source pixels to device
    double          lineWidth = 2.0;
    ogc_line_cap_t  cap = OGC_LINE_CAP_BUTT;
    ogc_line_join_t join = OGC_LINE_JOIN_MITER;
    ogc_operator_t  op = OGC_OPERATOR_OVER;
    ogc_fill_rule_t rule = OGC_FILL_RULE_WINDING;
    ogc_antialias_t aa = OGC_ANTIALIAS_DEFAULT;
};

} // namespace


struct ogc_t
{
    int                refs = 1;
    ogc_surface_t*     target = nullptr;
    STATE              st;
    std::vector<STATE> stack;
    std::vector<ELEM>  path;            // device space
    bool               hasCurrent = false;
    double             cx = 0, cy = 0;  // the current point, device
    double             sx = 0, sy = 0;  // the subpath's start, device
};


namespace
{

// ---- the device: one stream for every surface ----------------------------

constexpr long STREAM_WORDS = 32768;
constexpr long BLOCK_WORDS = 65536;     // path, pattern and dash words
constexpr int  WORK_ROWS = 16;

struct BLOCK
{
    ULONG* words;
    long   cap;
    long   used;
};

struct DEVICE
{
    bool               tried = false;
    bool               ok = false;
    bool               aliased = false;
    ULONG*             stream = nullptr;
    long               used = 0;
    std::vector<BLOCK> blocks;
    size_t             block = 0;       // the block being filled
    ULONG*             work = nullptr;
    unsigned long      workBytes = 0;
    ogc_surface_t*     slots[OGPU_MAX_SLOTS] = {};
    int                nslots = 0;
    int                target = -1;
    unsigned long      batch = 1;
};

DEVICE g_dev;


void closeDevice()
{
    ogc_sync();

    for( BLOCK& b : g_dev.blocks )
        FreeVec( b.words );

    g_dev.blocks.clear();

    if( g_dev.work )
        FreeVec( g_dev.work );

    if( g_dev.stream )
        FreeVec( g_dev.stream );

    g_dev.work = nullptr;
    g_dev.stream = nullptr;

    if( OpenGPUBase )
        CloseLibrary( OpenGPUBase );

    OpenGPUBase = nullptr;
    g_dev.ok = false;
}


bool device()
{
    if( g_dev.tried )
        return g_dev.ok;

    g_dev.tried = true;
    OpenGPUBase = OpenLibrary( (CONST_STRPTR) OPENGPU_NAME, OPENGPU_VERSION );

    if( !OpenGPUBase )
    {
        fprintf( stderr, "KiCad: no %s, the canvas stays empty\n", OPENGPU_NAME );
        return false;
    }

    if( OGPU_ANSWER( OGPU_Query( OGPU_OP_PATH_FILL, OGPU_FMT_ARGB32 ) ) == OGPU_NONE )
    {
        fprintf( stderr, "KiCad: %s %d.%d has no vector drawing (stream v1.6, 0.18 or later)\n",
                 OPENGPU_NAME, (int) OpenGPUBase->lib_Version, (int) OpenGPUBase->lib_Revision );
        CloseLibrary( OpenGPUBase );
        OpenGPUBase = nullptr;
        return false;
    }

    g_dev.aliased = OGPU_ANSWER( OGPU_Query( OGPU_QUERY_FLAG( OGPU_OP_PATH_FILL,
                                                              OGPU_PATH_ALIASED_BIT ),
                                             OGPU_FMT_ARGB32 ) ) == OGPU_FULL;

    g_dev.stream = (ULONG*) AllocVec( STREAM_WORDS * 4, MEMF_PUBLIC );

    if( !g_dev.stream )
    {
        CloseLibrary( OpenGPUBase );
        OpenGPUBase = nullptr;
        return false;
    }

    g_dev.ok = true;
    atexit( closeDevice );
    return true;
}


void submit()
{
    if( g_dev.used > 0 )
    {
        ULONG fence = 0;
        LONG  rc = OGPU_Submit( g_dev.stream, (ULONG) g_dev.used, &fence );

        if( fence )
            OGPU_Wait( fence );

        if( rc != OGPU_OK )
        {
            static int reported = 0;

            if( reported++ < 4 )
                fprintf( stderr, "KiCad: gfopengpu.library batch %ld\n", (long) rc );
        }
    }

    g_dev.used = 0;

    for( BLOCK& b : g_dev.blocks )
        b.used = 0;

    g_dev.block = 0;

    for( ogc_surface_t*& s : g_dev.slots )
        s = nullptr;

    g_dev.nslots = 0;
    g_dev.target = -1;
    g_dev.batch++;
}


/// room for n stream words, sending the batch first when there isn't
void room( long aWords )
{
    if( g_dev.used + aWords > STREAM_WORDS )
        submit();
}


inline void word( ULONG w )
{
    g_dev.stream[g_dev.used++] = w;
}


/// n contiguous words of block memory, valid until the batch is sent
ULONG* blockWords( long aWords )
{
    for( ;; )
    {
        while( g_dev.block < g_dev.blocks.size() )
        {
            BLOCK& b = g_dev.blocks[g_dev.block];

            if( b.cap - b.used >= aWords )
            {
                ULONG* p = b.words + b.used;
                b.used += aWords;
                return p;
            }

            if( b.used == 0 )
                break;      // too small even empty: a bigger one goes in its place

            g_dev.block++;
        }

        long   cap = std::max( BLOCK_WORDS, aWords );
        ULONG* w = (ULONG*) AllocVec( cap * 4, MEMF_PUBLIC );

        if( !w )
        {
            if( g_dev.used == 0 )
                return nullptr;

            submit();       // the blocks are free again
            continue;
        }

        if( g_dev.block < g_dev.blocks.size() )
        {
            FreeVec( g_dev.blocks[g_dev.block].words );
            g_dev.blocks[g_dev.block] = { w, cap, 0 };
        }
        else
        {
            g_dev.blocks.push_back( { w, cap, 0 } );
        }
    }
}


bool work( int aWidth )
{
    unsigned long need = 32UL * (unsigned long) std::max( aWidth, 1 ) * WORK_ROWS;

    if( g_dev.workBytes >= need )
        return true;

    submit();       // the batch may use the old memory

    if( g_dev.work )
        FreeVec( g_dev.work );

    g_dev.work = (ULONG*) AllocVec( need, MEMF_PUBLIC );
    g_dev.workBytes = g_dev.work ? need : 0;
    return g_dev.work != nullptr;
}


/// the slot that holds s in this batch, described when it is new
int slotFor( ogc_surface_t* s )
{
    for( int i = 0; i < g_dev.nslots; i++ )
    {
        if( g_dev.slots[i] == s )
            return i;
    }

    if( g_dev.nslots == OGPU_MAX_SLOTS )
        submit();

    room( 7 );
    int i = g_dev.nslots++;
    g_dev.slots[i] = s;
    word( OGPU_HDR( OGPU_OP_SURFACE, 6 ) );
    word( (ULONG) i );
    word( (ULONG) (uintptr_t) s->data );
    word( (ULONG) s->stride );
    word( OGPU_XY( s->width, s->height ) );
    word( OGPU_FMT_ARGB32 );
    return i;
}


/// target s, with room for n more stream words (and a source's slot);
/// false when it can't be drawn
bool begin( ogc_surface_t* s, long aWords )
{
    if( !device() || !s || !s->data || s->width <= 0 || s->height <= 0
        || s->width > OGPU_MAX_SIZE || s->height > OGPU_MAX_SIZE
        || aWords + 16 > STREAM_WORDS )
    {
        return false;
    }

    if( !work( s->width ) )
        return false;

    room( aWords + 16 );    // its SURFACE, a source's SURFACE, TARGET
    int slot = slotFor( s );

    if( g_dev.target != slot )
    {
        word( OGPU_HDR( OGPU_OP_TARGET, 2 ) );
        word( (ULONG) slot );
        g_dev.target = slot;
    }

    s->batch = g_dev.batch;
    return true;
}


inline LONG fix8( double v )
{
    // 24.8, as the stream takes it (cairo's fixed point too)
    v = std::clamp( v, -8000000.0, 8000000.0 );
    return (LONG) std::lround( v * 256.0 );
}


inline LONG fix16( double v )
{
    v = std::clamp( v, -32000.0, 32000.0 );
    return (LONG) std::lround( v * 65536.0 );
}


ULONG premultiplied( double r, double g, double b, double a )
{
    a = std::clamp( a, 0.0, 1.0 );
    unsigned A = (unsigned) std::lround( a * 255.0 );
    unsigned R = (unsigned) std::lround( std::clamp( r, 0.0, 1.0 ) * a * 255.0 );
    unsigned G = (unsigned) std::lround( std::clamp( g, 0.0, 1.0 ) * a * 255.0 );
    unsigned B = (unsigned) std::lround( std::clamp( b, 0.0, 1.0 ) * a * 255.0 );
    return ( (ULONG) A << 24 ) | ( (ULONG) R << 16 ) | ( (ULONG) G << 8 ) | B;
}


void apply( const ogc_matrix_t& m, double x, double y, double& ox, double& oy )
{
    ox = m.xx * x + m.xy * y + m.x0;
    oy = m.yx * x + m.yy * y + m.y0;
}


bool invert( const ogc_matrix_t& m, ogc_matrix_t& inv )
{
    double det = m.xx * m.yy - m.xy * m.yx;

    if( det == 0.0 || !std::isfinite( det ) )
        return false;

    inv.xx = m.yy / det;
    inv.xy = -m.xy / det;
    inv.yx = -m.yx / det;
    inv.yy = m.xx / det;
    inv.x0 = -( inv.xx * m.x0 + inv.xy * m.y0 );
    inv.y0 = -( inv.yx * m.x0 + inv.yy * m.y0 );
    return true;
}


/// the path's words in block memory; nullptr when there's no path
ULONG* pathWords( const std::vector<ELEM>& aPath, long& aCount )
{
    long n = 0;

    for( const ELEM& e : aPath )
        n += 1 + ( e.verb == V_MOVE || e.verb == V_LINE ? 2 : e.verb == V_CUBIC ? 6 : 0 );

    aCount = n;

    if( n == 0 )
        return nullptr;

    ULONG* w = blockWords( n );

    if( !w )
        return nullptr;

    ULONG* p = w;

    for( const ELEM& e : aPath )
    {
        *p++ = e.verb;
        int pts = e.verb == V_MOVE || e.verb == V_LINE ? 2 : e.verb == V_CUBIC ? 6 : 0;

        for( int i = 0; i < pts; i++ )
            *p++ = (ULONG) fix8( e.p[i] );
    }

    return w;
}


/// a pattern (the source surface under its matrix) in block memory
ULONG* patternWords( ogc_t* c, int aSlot, double aAlpha )
{
    ULONG* w = blockWords( OGPU_PATTERN_WORDS );

    if( !w )
        return nullptr;

    const ogc_matrix_t& m = c->st.sourceMatrix;
    w[0] = (ULONG) aSlot;
    w[1] = OGPU_PATTERN_NONE | OGPU_PATTERN_BILINEAR;
    w[2] = OGPU_XY( 0, 0 );
    w[3] = OGPU_XY( 0, 0 );     // the whole slot
    w[4] = (ULONG) fix16( m.xx );
    w[5] = (ULONG) fix16( m.xy );
    w[6] = (ULONG) fix16( m.x0 );
    w[7] = (ULONG) fix16( m.yx );
    w[8] = (ULONG) fix16( m.yy );
    w[9] = (ULONG) fix16( m.y0 );
    w[10] = (ULONG) std::lround( std::clamp( aAlpha, 0.0, 1.0 ) * 255.0 );
    return w;
}


void emitPath( ogc_t* c, ULONG* path, long words, ULONG* pattern, bool aStroke, double aAlpha );


/// one PATH_FILL or PATH_STROKE of path with operator op and the source
void drawPath( ogc_t* c, const std::vector<ELEM>& aPath, bool aStroke, double aAlpha = 1.0 )
{
    if( aPath.empty() )
        return;

    const long   cmdWords = aStroke ? 22 : 16;
    const STATE& st = c->st;

    for( int attempt = 0; attempt < 2; attempt++ )
    {
        // the target and the source both need slots in this batch
        if( st.source && st.op != OGC_OPERATOR_CLEAR && g_dev.nslots >= OGPU_MAX_SLOTS - 1 )
            submit();

        if( !begin( c->target, cmdWords * 2 ) )
            return;

        unsigned long batch = g_dev.batch;
        int           srcSlot = -1;

        if( st.source && st.op != OGC_OPERATOR_CLEAR )
        {
            srcSlot = slotFor( st.source );
            st.source->batch = batch;
        }

        long   words = 0;
        ULONG* path = pathWords( aPath, words );
        ULONG* pattern = srcSlot >= 0 ? patternWords( c, srcSlot, aAlpha ) : nullptr;

        if( g_dev.batch != batch )
            continue;   // finding block memory sent the batch: describe it all again

        if( !path || ( srcSlot >= 0 && !pattern ) )
            return;

        emitPath( c, path, words, pattern, aStroke, aAlpha );
        return;
    }
}


void emitPath( ogc_t* c, ULONG* path, long words, ULONG* pattern, bool aStroke, double aAlpha )
{
    const long   cmdWords = aStroke ? 22 : 16;
    const STATE& st = c->st;
    // two commands for the operators a path bounds (cairo's CLEAR and SOURCE):
    // OUT_REVERSE with an opaque paint, then ADD with the paint
    const bool   bounded = st.op == OGC_OPERATOR_CLEAR || st.op == OGC_OPERATOR_SOURCE;

    ULONG aa = ( st.aa == OGC_ANTIALIAS_NONE && g_dev.aliased ) ? OGPU_PATH_ALIASED : 0;
    ULONG rule = ( !aStroke && st.rule == OGC_FILL_RULE_EVEN_ODD ) ? OGPU_PATH_EVENODD : 0;

    auto command = [&]( ULONG aOp, ULONG aFlags, ULONG aPaint )
    {
        word( OGPU_HDR( aStroke ? OGPU_OP_PATH_STROKE : OGPU_OP_PATH_FILL, cmdWords ) );
        word( aOp | aFlags | aa | rule );
        word( aPaint );
        word( OGPU_XY( 0, 0 ) );
        word( 0 );
        word( OGPU_XY( 0, 0 ) );
        word( (ULONG) (uintptr_t) path );
        word( (ULONG) words );
        word( 0x10000 );
        word( 0 );
        word( 0 );
        word( 0 );
        word( 0x10000 );
        word( 0 );
        word( (ULONG) (uintptr_t) g_dev.work );
        word( g_dev.workBytes );

        if( aStroke )
        {
            const ogc_matrix_t& m = st.ctm;
            double scale = std::sqrt( std::fabs( m.xx * m.yy - m.xy * m.yx ) );
            double width = st.lineWidth * scale;
            ULONG  style = ( st.cap == OGC_LINE_CAP_ROUND    ? OGPU_CAP_ROUND
                             : st.cap == OGC_LINE_CAP_SQUARE ? OGPU_CAP_SQUARE
                                                             : OGPU_CAP_BUTT )
                          | ( st.join == OGC_LINE_JOIN_ROUND   ? OGPU_JOIN_ROUND
                              : st.join == OGC_LINE_JOIN_BEVEL ? OGPU_JOIN_BEVEL
                                                               : OGPU_JOIN_MITER );
            word( (ULONG) fix16( width ) );
            word( style );
            word( 10UL << 16 );     // cairo's miter limit
            word( 0 );
            word( 0 );
            word( 0 );
        }
    };

    ULONG paint;
    ULONG paintFlags = 0;

    if( pattern )
    {
        paint = (ULONG) (uintptr_t) pattern;
        paintFlags = OGPU_PATH_PATTERN;
    }
    else
    {
        paint = premultiplied( st.r, st.g, st.b, st.a * aAlpha );
    }

    if( bounded )
    {
        command( OGPU_PD_OUT_REVERSE, 0, 0xFF000000UL );

        if( st.op == OGC_OPERATOR_SOURCE )
            command( OGPU_PD_ADD, paintFlags, paint );
    }
    else
    {
        ULONG op = st.op == OGC_OPERATOR_ADD          ? OGPU_PD_ADD
                   : st.op == OGC_OPERATOR_DIFFERENCE ? OGPU_BLEND_DIFFERENCE
                                                      : OGPU_PD_OVER;
        command( op, paintFlags, paint );
    }
}


void addPoint( ogc_t* c, unsigned char aVerb, const double* aUser, int aPoints )
{
    ELEM e;
    e.verb = aVerb;

    for( int i = 0; i < aPoints; i++ )
        apply( c->st.ctm, aUser[i * 2], aUser[i * 2 + 1], e.p[i * 2], e.p[i * 2 + 1] );

    c->path.push_back( e );

    if( aPoints )
    {
        c->cx = e.p[aPoints * 2 - 2];
        c->cy = e.p[aPoints * 2 - 1];
    }
}


void arcPath( ogc_t* c, double xc, double yc, double r, double a1, double a2 )
{
    double x0 = xc + r * std::cos( a1 );
    double y0 = yc + r * std::sin( a1 );

    if( c->hasCurrent )
        ogc_line_to( c, x0, y0 );
    else
        ogc_move_to( c, x0, y0 );

    if( r <= 0.0 || a1 == a2 )
        return;

    double sweep = a2 - a1;
    int    n = (int) std::ceil( std::fabs( sweep ) / ( kPi / 2.0 ) - 1e-9 );
    n = std::max( n, 1 );
    double step = sweep / n;
    double k = 4.0 / 3.0 * std::tan( step / 4.0 );
    double t = a1;

    for( int i = 0; i < n; i++ )
    {
        double t1 = t + step;
        double c0 = std::cos( t ), s0 = std::sin( t );
        double c1 = std::cos( t1 ), s1 = std::sin( t1 );
        ogc_curve_to( c, xc + r * ( c0 - k * s0 ), yc + r * ( s0 + k * c0 ),
                      xc + r * ( c1 + k * s1 ), yc + r * ( s1 - k * c1 ), xc + r * c1,
                      yc + r * s1 );
        t = t1;
    }
}

} // namespace


// ---- matrices --------------------------------------------------------------

void ogc_matrix_init( ogc_matrix_t* m, double xx, double yx, double xy, double yy, double x0,
                      double y0 )
{
    m->xx = xx;
    m->yx = yx;
    m->xy = xy;
    m->yy = yy;
    m->x0 = x0;
    m->y0 = y0;
}


void ogc_matrix_init_identity( ogc_matrix_t* m )
{
    ogc_matrix_init( m, 1, 0, 0, 1, 0, 0 );
}


void ogc_matrix_multiply( ogc_matrix_t* r, const ogc_matrix_t* a, const ogc_matrix_t* b )
{
    ogc_matrix_t t;
    t.xx = a->xx * b->xx + a->yx * b->xy;
    t.yx = a->xx * b->yx + a->yx * b->yy;
    t.xy = a->xy * b->xx + a->yy * b->xy;
    t.yy = a->xy * b->yx + a->yy * b->yy;
    t.x0 = a->x0 * b->xx + a->y0 * b->xy + b->x0;
    t.y0 = a->x0 * b->yx + a->y0 * b->yy + b->y0;
    *r = t;
}


void ogc_matrix_translate( ogc_matrix_t* m, double tx, double ty )
{
    ogc_matrix_t t;
    ogc_matrix_init( &t, 1, 0, 0, 1, tx, ty );
    ogc_matrix_multiply( m, &t, m );
}


void ogc_matrix_scale( ogc_matrix_t* m, double sx, double sy )
{
    ogc_matrix_t t;
    ogc_matrix_init( &t, sx, 0, 0, sy, 0, 0 );
    ogc_matrix_multiply( m, &t, m );
}


void ogc_matrix_rotate( ogc_matrix_t* m, double radians )
{
    double       s = std::sin( radians ), c = std::cos( radians );
    ogc_matrix_t t;
    ogc_matrix_init( &t, c, s, -s, c, 0, 0 );
    ogc_matrix_multiply( m, &t, m );
}


// ---- surfaces --------------------------------------------------------------

int ogc_format_stride_for_width( ogc_format_t, int width )
{
    return width * 4;
}


ogc_surface_t* ogc_image_surface_create( ogc_format_t, int width, int height )
{
    ogc_surface_t* s = new ogc_surface_t;
    s->width = std::max( width, 0 );
    s->height = std::max( height, 0 );
    s->stride = s->width * 4;
    s->data = new unsigned char[(size_t) s->stride * s->height + 4]();
    s->owned = true;
    return s;
}


ogc_surface_t* ogc_image_surface_create_for_data( unsigned char* data, ogc_format_t, int width,
                                                  int height, int stride )
{
    ogc_surface_t* s = new ogc_surface_t;
    s->width = width;
    s->height = height;
    s->stride = stride;
    s->data = data;
    return s;
}


unsigned char* ogc_image_surface_get_data( ogc_surface_t* s )
{
    return s ? s->data : nullptr;
}


int ogc_image_surface_get_width( ogc_surface_t* s )
{
    return s ? s->width : 0;
}


int ogc_image_surface_get_height( ogc_surface_t* s )
{
    return s ? s->height : 0;
}


int ogc_image_surface_get_stride( ogc_surface_t* s )
{
    return s ? s->stride : 0;
}


ogc_status_t ogc_surface_status( ogc_surface_t* s )
{
    return s && ( s->data || !s->width ) ? OGC_STATUS_SUCCESS : OGC_STATUS_NO_MEMORY;
}


void ogc_surface_flush( ogc_surface_t* s )
{
    if( s && s->batch == g_dev.batch )
        submit();
}


void ogc_surface_mark_dirty( ogc_surface_t* )
{
}


ogc_surface_t* ogc_surface_reference( ogc_surface_t* s )
{
    if( s )
        s->refs++;

    return s;
}


void ogc_surface_destroy( ogc_surface_t* s )
{
    if( !s || --s->refs > 0 )
        return;

    // a batch still to be sent may draw into it or from it
    if( s->batch == g_dev.batch )
        submit();

    if( s->owned )
        delete[] s->data;

    delete s;
}


void ogc_surface_clear( ogc_surface_t* s )
{
    if( begin( s, 4 ) )
    {
        word( OGPU_HDR( OGPU_OP_FILL, 4 ) );
        word( OGPU_XY( 0, 0 ) );
        word( OGPU_XY( s->width, s->height ) );
        word( 0 );
    }
    else if( s && s->data )
    {
        memset( s->data, 0, (size_t) s->stride * s->height );
    }
}


// ---- contexts --------------------------------------------------------------

ogc_t* ogc_create( ogc_surface_t* target )
{
    ogc_t* c = new ogc_t;
    c->target = ogc_surface_reference( target );
    ogc_matrix_init_identity( &c->st.ctm );
    ogc_matrix_init_identity( &c->st.sourceMatrix );
    return c;
}


ogc_t* ogc_reference( ogc_t* c )
{
    if( c )
        c->refs++;

    return c;
}


void ogc_destroy( ogc_t* c )
{
    if( !c || --c->refs > 0 )
        return;

    for( STATE& s : c->stack )
        ogc_surface_destroy( s.source );

    ogc_surface_destroy( c->st.source );
    ogc_surface_destroy( c->target );
    delete c;
}


ogc_status_t ogc_status( ogc_t* c )
{
    return c ? OGC_STATUS_SUCCESS : OGC_STATUS_NO_MEMORY;
}


ogc_surface_t* ogc_get_target( ogc_t* c )
{
    return c->target;
}


void ogc_save( ogc_t* c )
{
    c->stack.push_back( c->st );
    ogc_surface_reference( c->st.source );
}


void ogc_restore( ogc_t* c )
{
    if( c->stack.empty() )
        return;

    ogc_surface_destroy( c->st.source );
    c->st = c->stack.back();
    c->stack.pop_back();
}


void ogc_get_matrix( ogc_t* c, ogc_matrix_t* m )
{
    *m = c->st.ctm;
}


void ogc_set_matrix( ogc_t* c, const ogc_matrix_t* m )
{
    c->st.ctm = *m;
}


void ogc_identity_matrix( ogc_t* c )
{
    ogc_matrix_init_identity( &c->st.ctm );
}


void ogc_translate( ogc_t* c, double tx, double ty )
{
    ogc_matrix_translate( &c->st.ctm, tx, ty );
}


void ogc_scale( ogc_t* c, double sx, double sy )
{
    ogc_matrix_scale( &c->st.ctm, sx, sy );
}


void ogc_rotate( ogc_t* c, double radians )
{
    ogc_matrix_rotate( &c->st.ctm, radians );
}


void ogc_transform( ogc_t* c, const ogc_matrix_t* m )
{
    ogc_matrix_multiply( &c->st.ctm, m, &c->st.ctm );
}


void ogc_device_to_user_distance( ogc_t* c, double* dx, double* dy )
{
    ogc_matrix_t inv;

    if( !invert( c->st.ctm, inv ) )
        return;

    double x = *dx, y = *dy;
    *dx = inv.xx * x + inv.xy * y;
    *dy = inv.yx * x + inv.yy * y;
}


void ogc_set_source_rgba( ogc_t* c, double r, double g, double b, double a )
{
    ogc_surface_destroy( c->st.source );
    c->st.source = nullptr;
    c->st.r = r;
    c->st.g = g;
    c->st.b = b;
    c->st.a = a;
}


void ogc_set_source_surface( ogc_t* c, ogc_surface_t* s, double x, double y )
{
    ogc_surface_reference( s );
    ogc_surface_destroy( c->st.source );
    c->st.source = s;

    // the surface's pixel (0, 0) at user (x, y) under the matrix in force now
    ogc_matrix_t t;
    ogc_matrix_init( &t, 1, 0, 0, 1, x, y );
    ogc_matrix_multiply( &c->st.sourceMatrix, &t, &c->st.ctm );
}


void ogc_set_operator( ogc_t* c, ogc_operator_t op )
{
    c->st.op = op;
}


ogc_operator_t ogc_get_operator( ogc_t* c )
{
    return c->st.op;
}


void ogc_set_antialias( ogc_t* c, ogc_antialias_t aa )
{
    c->st.aa = aa;
}


void ogc_set_fill_rule( ogc_t* c, ogc_fill_rule_t rule )
{
    c->st.rule = rule;
}


void ogc_set_line_width( ogc_t* c, double width )
{
    c->st.lineWidth = width;
}


void ogc_set_line_cap( ogc_t* c, ogc_line_cap_t cap )
{
    c->st.cap = cap;
}


void ogc_set_line_join( ogc_t* c, ogc_line_join_t join )
{
    c->st.join = join;
}


// ---- paths -----------------------------------------------------------------

void ogc_new_path( ogc_t* c )
{
    c->path.clear();
    c->hasCurrent = false;
}


void ogc_new_sub_path( ogc_t* c )
{
    c->hasCurrent = false;
}


void ogc_move_to( ogc_t* c, double x, double y )
{
    // a move after a move replaces it
    if( !c->path.empty() && c->path.back().verb == V_MOVE )
        c->path.pop_back();

    double p[2] = { x, y };
    addPoint( c, V_MOVE, p, 1 );
    c->sx = c->cx;
    c->sy = c->cy;
    c->hasCurrent = true;
}


void ogc_line_to( ogc_t* c, double x, double y )
{
    if( !c->hasCurrent )
    {
        ogc_move_to( c, x, y );
        return;
    }

    double p[2] = { x, y };
    addPoint( c, V_LINE, p, 1 );
}


void ogc_curve_to( ogc_t* c, double x1, double y1, double x2, double y2, double x, double y )
{
    if( !c->hasCurrent )
        ogc_move_to( c, x1, y1 );

    double p[6] = { x1, y1, x2, y2, x, y };
    addPoint( c, V_CUBIC, p, 3 );
}


void ogc_arc( ogc_t* c, double xc, double yc, double r, double a1, double a2 )
{
    if( !std::isfinite( a1 ) || !std::isfinite( a2 ) )
        return;

    // as cairo: the end goes forward of the start, by at most a few turns
    if( a2 < a1 )
    {
        double turns = std::ceil( ( a1 - a2 ) / ( 2.0 * kPi ) );
        a2 += turns * 2.0 * kPi;
    }

    arcPath( c, xc, yc, r, a1, a2 );
}


void ogc_arc_negative( ogc_t* c, double xc, double yc, double r, double a1, double a2 )
{
    if( !std::isfinite( a1 ) || !std::isfinite( a2 ) )
        return;

    if( a2 > a1 )
    {
        double turns = std::ceil( ( a2 - a1 ) / ( 2.0 * kPi ) );
        a2 -= turns * 2.0 * kPi;
    }

    arcPath( c, xc, yc, r, a1, a2 );
}


void ogc_rectangle( ogc_t* c, double x, double y, double w, double h )
{
    ogc_move_to( c, x, y );
    ogc_line_to( c, x + w, y );
    ogc_line_to( c, x + w, y + h );
    ogc_line_to( c, x, y + h );
    ogc_close_path( c );
}


void ogc_close_path( ogc_t* c )
{
    if( !c->hasCurrent )
        return;

    ELEM e;
    e.verb = V_CLOSE;
    c->path.push_back( e );

    // as cairo: the current point goes back to the subpath's start
    ELEM m;
    m.verb = V_MOVE;
    m.p[0] = c->sx;
    m.p[1] = c->sy;
    c->path.push_back( m );
    c->cx = c->sx;
    c->cy = c->sy;
}


ogc_path_t* ogc_copy_path( ogc_t* c )
{
    ogc_path_t*  p = new ogc_path_t;
    ogc_matrix_t inv;

    if( !invert( c->st.ctm, inv ) )
        ogc_matrix_init_identity( &inv );

    p->elems.reserve( c->path.size() );

    for( const ELEM& e : c->path )
    {
        ELEM u = e;
        int  pts = e.verb == V_CUBIC ? 3 : e.verb == V_CLOSE ? 0 : 1;

        for( int i = 0; i < pts; i++ )
            apply( inv, e.p[i * 2], e.p[i * 2 + 1], u.p[i * 2], u.p[i * 2 + 1] );

        p->elems.push_back( u );
    }

    return p;
}


void ogc_append_path( ogc_t* c, const ogc_path_t* p )
{
    if( !p )
        return;

    for( const ELEM& e : p->elems )
    {
        switch( e.verb )
        {
        case V_MOVE:  ogc_move_to( c, e.p[0], e.p[1] ); break;
        case V_LINE:  ogc_line_to( c, e.p[0], e.p[1] ); break;
        case V_CUBIC: ogc_curve_to( c, e.p[0], e.p[1], e.p[2], e.p[3], e.p[4], e.p[5] ); break;
        case V_CLOSE: ogc_close_path( c ); break;
        }
    }
}


void ogc_path_destroy( ogc_path_t* p )
{
    delete p;
}


// ---- drawing ---------------------------------------------------------------

void ogc_fill_preserve( ogc_t* c )
{
    drawPath( c, c->path, false );
}


void ogc_fill( ogc_t* c )
{
    ogc_fill_preserve( c );
    ogc_new_path( c );
}


void ogc_stroke_preserve( ogc_t* c )
{
    drawPath( c, c->path, true );
}


void ogc_stroke( ogc_t* c )
{
    ogc_stroke_preserve( c );
    ogc_new_path( c );
}


void ogc_paint_with_alpha( ogc_t* c, double alpha )
{
    ogc_surface_t* t = c->target;

    if( !t )
        return;

    const STATE&        st = c->st;
    const ogc_matrix_t& m = st.sourceMatrix;

    // a surface at a whole-pixel offset: one COMPOSITE_PD
    if( st.source && m.xx == 1.0 && m.yy == 1.0 && m.xy == 0.0 && m.yx == 0.0
        && m.x0 == std::floor( m.x0 ) && m.y0 == std::floor( m.y0 )
        && ( st.op == OGC_OPERATOR_OVER || st.op == OGC_OPERATOR_ADD
             || st.op == OGC_OPERATOR_DIFFERENCE || st.op == OGC_OPERATOR_SOURCE ) )
    {
        if( g_dev.nslots >= OGPU_MAX_SLOTS - 1 )
            submit();

        if( !begin( t, 8 ) )
            return;

        int src = slotFor( st.source );
        st.source->batch = g_dev.batch;

        int x = (int) m.x0, y = (int) m.y0;
        int sx = std::max( 0, -x ), sy = std::max( 0, -y );
        int w = std::min( st.source->width - sx, t->width - std::max( x, 0 ) );
        int h = std::min( st.source->height - sy, t->height - std::max( y, 0 ) );

        if( w <= 0 || h <= 0 )
            return;

        ULONG op = st.op == OGC_OPERATOR_ADD          ? OGPU_PD_ADD
                   : st.op == OGC_OPERATOR_DIFFERENCE ? OGPU_BLEND_DIFFERENCE
                   : st.op == OGC_OPERATOR_SOURCE     ? OGPU_PD_SRC
                                                      : OGPU_PD_OVER;
        unsigned a = (unsigned) std::lround( std::clamp( alpha, 0.0, 1.0 ) * 255.0 );
        word( OGPU_HDR( OGPU_OP_COMPOSITE_PD, 8 ) );
        word( op | ( a < 255 ? OGPU_PD_MASK_SOLID : 0 ) );
        word( (ULONG) src );
        word( OGPU_XY( sx, sy ) );
        word( a < 255 ? a : 0 );
        word( OGPU_XY( 0, 0 ) );
        word( OGPU_XY( std::max( x, 0 ), std::max( y, 0 ) ) );
        word( OGPU_XY( w, h ) );
        return;
    }

    // otherwise the area to paint as a path: the surface's rectangle under its
    // matrix, or the whole target for a colour
    std::vector<ELEM> area;
    auto corner = [&]( unsigned char aVerb, double aX, double aY )
    {
        ELEM e;
        e.verb = aVerb;

        if( st.source )
            apply( m, aX, aY, e.p[0], e.p[1] );
        else
            e.p[0] = aX, e.p[1] = aY;

        area.push_back( e );
    };

    double w = st.source ? st.source->width : t->width;
    double h = st.source ? st.source->height : t->height;
    corner( V_MOVE, 0, 0 );
    corner( V_LINE, w, 0 );
    corner( V_LINE, w, h );
    corner( V_LINE, 0, h );
    ELEM close;
    close.verb = V_CLOSE;
    area.push_back( close );

    drawPath( c, area, false, alpha );
}


void ogc_paint( ogc_t* c )
{
    ogc_paint_with_alpha( c, 1.0 );
}


void ogc_sync()
{
    if( g_dev.ok )
        submit();
}


bool ogc_available()
{
    return device();
}
