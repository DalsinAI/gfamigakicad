/* GL/glu.h for KiCad's Amiga build until the toolkit has an OpenGL canvas:
   the GLU calls KiCad makes, answered by amiga/glu_none.cpp. They are only
   reached through a GL context, which no canvas can make yet.
   Copyright (c) 2026 AmiCradle Team */
#ifndef GLU_H_
#define GLU_H_
#include <GL/gl.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GLU_FILL        100012
#define GLU_LINE        100011
#define GLU_POINT       100010
#define GLU_SILHOUETTE  100013
#define GLU_SMOOTH      100000
#define GLU_FLAT        100001
#define GLU_NONE        100002

#define GLU_TESS_BEGIN          100100
#define GLU_TESS_VERTEX         100101
#define GLU_TESS_END            100102
#define GLU_TESS_ERROR          100103
#define GLU_TESS_EDGE_FLAG      100104
#define GLU_TESS_COMBINE        100105
#define GLU_TESS_BEGIN_DATA     100106
#define GLU_TESS_VERTEX_DATA    100107
#define GLU_TESS_END_DATA       100108
#define GLU_TESS_ERROR_DATA     100109
#define GLU_TESS_EDGE_FLAG_DATA 100110
#define GLU_TESS_COMBINE_DATA   100111
#define GLU_TESS_WINDING_RULE   100140
#define GLU_TESS_BOUNDARY_ONLY  100141
#define GLU_TESS_TOLERANCE      100142
#define GLU_TESS_WINDING_ODD          100130
#define GLU_TESS_WINDING_NONZERO      100131
#define GLU_TESS_WINDING_POSITIVE     100132
#define GLU_TESS_WINDING_NEGATIVE     100133
#define GLU_TESS_WINDING_ABS_GEQ_TWO  100134

typedef struct GLUquadric GLUquadric;
typedef struct GLUtesselator GLUtesselator;
typedef GLUquadric GLUquadricObj;
typedef GLUtesselator GLUtesselatorObj;
typedef GLUtesselator GLUtriangulatorObj;
typedef void ( *_GLUfuncptr )( void );

GLUquadric* gluNewQuadric( void );
void gluDeleteQuadric( GLUquadric* q );
void gluQuadricDrawStyle( GLUquadric* q, GLenum draw );
void gluQuadricNormals( GLUquadric* q, GLenum normal );
void gluCylinder( GLUquadric* q, GLdouble base, GLdouble top, GLdouble height, GLint slices,
                  GLint stacks );
void gluDisk( GLUquadric* q, GLdouble inner, GLdouble outer, GLint slices, GLint loops );
void gluSphere( GLUquadric* q, GLdouble radius, GLint slices, GLint stacks );
void gluPerspective( GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar );
const GLubyte* gluErrorString( GLenum error );

GLUtesselator* gluNewTess( void );
void gluDeleteTess( GLUtesselator* tess );
void gluTessCallback( GLUtesselator* tess, GLenum which, _GLUfuncptr fn );
void gluTessProperty( GLUtesselator* tess, GLenum which, GLdouble data );
void gluTessNormal( GLUtesselator* tess, GLdouble x, GLdouble y, GLdouble z );
void gluTessBeginPolygon( GLUtesselator* tess, GLvoid* data );
void gluTessBeginContour( GLUtesselator* tess );
void gluTessVertex( GLUtesselator* tess, GLdouble* location, GLvoid* data );
void gluTessEndContour( GLUtesselator* tess );
void gluTessEndPolygon( GLUtesselator* tess );

#ifdef __cplusplus
}
#endif
#endif
