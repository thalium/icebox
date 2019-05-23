/*
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "unpacker.h"

void crUnpackExtendChromiumParametervCR( void  )
{
    GLenum target = READ_DATA( 8, GLenum );
    GLenum type = READ_DATA( 12, GLenum );
    GLsizei count = READ_DATA( 16, GLsizei );
    GLvoid *values = DATA_POINTER( 20, GLvoid );

    cr_unpackDispatch.ChromiumParametervCR(target, type, count, values);


    /*
    INCR_VAR_PTR();
    */
}

void crUnpackExtendDeleteQueriesARB(void)
{
    GLsizei n = READ_DATA( 8, GLsizei );
    const GLuint *ids = DATA_POINTER(12, GLuint);
    cr_unpackDispatch.DeleteQueriesARB(n, ids);
}

void crUnpackExtendGetPolygonStipple(void)
{
    GLubyte *mask;

    SET_RETURN_PTR( 8 );
    SET_WRITEBACK_PTR( 16 );
    mask = DATA_POINTER(8, GLubyte);

    cr_unpackDispatch.GetPolygonStipple( mask );
}

void crUnpackExtendGetPixelMapfv(void)
{
    GLenum map = READ_DATA( 8, GLenum );
    GLfloat *values;

    SET_RETURN_PTR( 12 );
    SET_WRITEBACK_PTR( 20 );
    values = DATA_POINTER(12, GLfloat);

    cr_unpackDispatch.GetPixelMapfv( map, values );
}

void crUnpackExtendGetPixelMapuiv(void)
{
    GLenum map = READ_DATA( 8, GLenum );
    GLuint *values;

    SET_RETURN_PTR( 12 );
    SET_WRITEBACK_PTR( 20 );
    values = DATA_POINTER(12, GLuint);

    cr_unpackDispatch.GetPixelMapuiv( map, values );
}

void crUnpackExtendGetPixelMapusv(void)
{
    GLenum map = READ_DATA( 8, GLenum );
    GLushort *values;

    SET_RETURN_PTR( 12 );
    SET_WRITEBACK_PTR( 20 );
    values = DATA_POINTER(12, GLushort);

    cr_unpackDispatch.GetPixelMapusv( map, values );
}

void crUnpackExtendVBoxTexPresent(void)
{
    GLuint texture = READ_DATA( 8, GLuint );
    GLuint cfg = READ_DATA( 12, GLuint );
    GLint xPos = READ_DATA( 16, GLint );
    GLint yPos = READ_DATA( 20, GLint );
    GLint cRects = READ_DATA( 24, GLint );
    GLint *pRects = (GLint *)DATA_POINTER( 28, GLvoid );
    cr_unpackDispatch.VBoxTexPresent( texture, cfg, xPos, yPos, cRects, pRects );
}
