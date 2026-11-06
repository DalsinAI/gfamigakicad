/* KiCad's canvas on gfopengpu.library (stream v1.6 PATH_FILL, PATH_STROKE,
 * COMPOSITE_PD): the drawing state the Cairo GAL keeps (a path, a matrix,
 * a source, a line, an operator), written as stream words. No cairo, no
 * pixman: every pixel is gfopengpu.library's.
 *
 * Paths are kept in device pixels as they are built (the matrix in force
 * when a point is added, as a canvas does), and sent as 24.8 points under
 * the identity matrix. Strokes are as wide as the matrix makes them.
 * Drawing is batched: one stream for every surface, sent when it fills or
 * when the CPU needs pixels (ogc_sync, ogc_surface_flush, freeing a
 * surface that a waiting batch draws into).
 *
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, as KiCad. */
#ifndef KICAD_AMIGA_OGPU_CANVAS_H
#define KICAD_AMIGA_OGPU_CANVAS_H

enum ogc_status_t
{
    OGC_STATUS_SUCCESS = 0,
    OGC_STATUS_NO_MEMORY
};

enum ogc_format_t
{
    OGC_FORMAT_ARGB32 = 0 /* A, R, G, B in memory: a big-endian ULONG, premultiplied */
};

enum ogc_operator_t
{
    OGC_OPERATOR_CLEAR,
    OGC_OPERATOR_SOURCE,
    OGC_OPERATOR_OVER,
    OGC_OPERATOR_ADD,
    OGC_OPERATOR_DIFFERENCE
};

enum ogc_antialias_t
{
    OGC_ANTIALIAS_DEFAULT,
    OGC_ANTIALIAS_NONE,
    OGC_ANTIALIAS_FAST,
    OGC_ANTIALIAS_GOOD
};

enum ogc_line_cap_t
{
    OGC_LINE_CAP_BUTT,
    OGC_LINE_CAP_ROUND,
    OGC_LINE_CAP_SQUARE
};

enum ogc_line_join_t
{
    OGC_LINE_JOIN_MITER,
    OGC_LINE_JOIN_ROUND,
    OGC_LINE_JOIN_BEVEL
};

enum ogc_fill_rule_t
{
    OGC_FILL_RULE_WINDING,
    OGC_FILL_RULE_EVEN_ODD
};

/* x' = xx x + xy y + x0, y' = yx x + yy y + y0 */
struct ogc_matrix_t
{
    double xx, yx, xy, yy, x0, y0;
};

struct ogc_surface_t;
struct ogc_t;
struct ogc_path_t;

/* matrices */
void ogc_matrix_init( ogc_matrix_t* m, double xx, double yx, double xy, double yy, double x0,
                      double y0 );
void ogc_matrix_init_identity( ogc_matrix_t* m );
/* r = a, then b */
void ogc_matrix_multiply( ogc_matrix_t* r, const ogc_matrix_t* a, const ogc_matrix_t* b );
void ogc_matrix_translate( ogc_matrix_t* m, double tx, double ty );
void ogc_matrix_scale( ogc_matrix_t* m, double sx, double sy );
void ogc_matrix_rotate( ogc_matrix_t* m, double radians );

/* surfaces: ARGB32, rows of width * 4 bytes */
int            ogc_format_stride_for_width( ogc_format_t f, int width );
ogc_surface_t* ogc_image_surface_create( ogc_format_t f, int width, int height );
ogc_surface_t* ogc_image_surface_create_for_data( unsigned char* data, ogc_format_t f, int width,
                                                  int height, int stride );
unsigned char* ogc_image_surface_get_data( ogc_surface_t* s );
int            ogc_image_surface_get_width( ogc_surface_t* s );
int            ogc_image_surface_get_height( ogc_surface_t* s );
int            ogc_image_surface_get_stride( ogc_surface_t* s );
ogc_status_t   ogc_surface_status( ogc_surface_t* s );
/* the drawing sent so far is done: the CPU may read or write the pixels */
void           ogc_surface_flush( ogc_surface_t* s );
void           ogc_surface_mark_dirty( ogc_surface_t* s );
ogc_surface_t* ogc_surface_reference( ogc_surface_t* s );
void           ogc_surface_destroy( ogc_surface_t* s );
/* every pixel 0, in the stream (no CPU) */
void           ogc_surface_clear( ogc_surface_t* s );

/* contexts */
ogc_t*         ogc_create( ogc_surface_t* target );
ogc_t*         ogc_reference( ogc_t* c );
void           ogc_destroy( ogc_t* c );
ogc_status_t   ogc_status( ogc_t* c );
ogc_surface_t* ogc_get_target( ogc_t* c );
void           ogc_save( ogc_t* c );
void           ogc_restore( ogc_t* c );

void ogc_get_matrix( ogc_t* c, ogc_matrix_t* m );
void ogc_set_matrix( ogc_t* c, const ogc_matrix_t* m );
void ogc_identity_matrix( ogc_t* c );
void ogc_translate( ogc_t* c, double tx, double ty );
void ogc_scale( ogc_t* c, double sx, double sy );
void ogc_rotate( ogc_t* c, double radians );
void ogc_transform( ogc_t* c, const ogc_matrix_t* m );
void ogc_device_to_user_distance( ogc_t* c, double* dx, double* dy );

void            ogc_set_source_rgba( ogc_t* c, double r, double g, double b, double a );
void            ogc_set_source_surface( ogc_t* c, ogc_surface_t* s, double x, double y );
void            ogc_set_operator( ogc_t* c, ogc_operator_t op );
ogc_operator_t  ogc_get_operator( ogc_t* c );
void            ogc_set_antialias( ogc_t* c, ogc_antialias_t aa );
void            ogc_set_fill_rule( ogc_t* c, ogc_fill_rule_t rule );
void            ogc_set_line_width( ogc_t* c, double width );
void            ogc_set_line_cap( ogc_t* c, ogc_line_cap_t cap );
void            ogc_set_line_join( ogc_t* c, ogc_line_join_t join );

void ogc_new_path( ogc_t* c );
void ogc_new_sub_path( ogc_t* c );
void ogc_move_to( ogc_t* c, double x, double y );
void ogc_line_to( ogc_t* c, double x, double y );
void ogc_curve_to( ogc_t* c, double x1, double y1, double x2, double y2, double x, double y );
void ogc_arc( ogc_t* c, double xc, double yc, double r, double a1, double a2 );
void ogc_arc_negative( ogc_t* c, double xc, double yc, double r, double a1, double a2 );
void ogc_rectangle( ogc_t* c, double x, double y, double w, double h );
void ogc_close_path( ogc_t* c );

/* the path in user space (the matrix in force now), to append later */
ogc_path_t* ogc_copy_path( ogc_t* c );
void        ogc_append_path( ogc_t* c, const ogc_path_t* p );
void        ogc_path_destroy( ogc_path_t* p );

void ogc_fill( ogc_t* c );
void ogc_fill_preserve( ogc_t* c );
void ogc_stroke( ogc_t* c );
void ogc_stroke_preserve( ogc_t* c );
void ogc_paint( ogc_t* c );
void ogc_paint_with_alpha( ogc_t* c, double alpha );

/* send what is batched and wait for it */
void ogc_sync();
/* false when gfopengpu.library (0.18 or later, stream v1.6) can't be had:
 * the canvas then stays empty */
bool ogc_available();

#endif
