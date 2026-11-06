/*
 * The GLU calls of glstub/GL/glu.h. Nothing reaches them until KiCad can
 * make a GL context, so they do no work.
 * Copyright (c) 2026 AmiCradle Team
 * Designed, tested and delivered by The AmigaChrome Team
 */
#include <GL/glu.h>

struct GLUquadric { int unused; };
struct GLUtesselator { int unused; };

extern "C" {
GLUquadric* gluNewQuadric( void ) { return new GLUquadric(); }
void gluDeleteQuadric( GLUquadric* q ) { delete q; }
void gluQuadricDrawStyle( GLUquadric*, GLenum ) {}
void gluQuadricNormals( GLUquadric*, GLenum ) {}
void gluCylinder( GLUquadric*, GLdouble, GLdouble, GLdouble, GLint, GLint ) {}
void gluDisk( GLUquadric*, GLdouble, GLdouble, GLint, GLint ) {}
void gluSphere( GLUquadric*, GLdouble, GLint, GLint ) {}
void gluPerspective( GLdouble, GLdouble, GLdouble, GLdouble ) {}
const GLubyte* gluErrorString( GLenum ) { return (const GLubyte*) "no OpenGL on this system yet"; }

GLUtesselator* gluNewTess( void ) { return new GLUtesselator(); }
void gluDeleteTess( GLUtesselator* t ) { delete t; }
void gluTessCallback( GLUtesselator*, GLenum, _GLUfuncptr ) {}
void gluTessProperty( GLUtesselator*, GLenum, GLdouble ) {}
void gluTessNormal( GLUtesselator*, GLdouble, GLdouble, GLdouble ) {}
void gluTessBeginPolygon( GLUtesselator*, GLvoid* ) {}
void gluTessBeginContour( GLUtesselator* ) {}
void gluTessVertex( GLUtesselator*, GLdouble*, GLvoid* ) {}
void gluTessEndContour( GLUtesselator* ) {}
void gluTessEndPolygon( GLUtesselator* ) {}
}
