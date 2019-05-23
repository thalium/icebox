/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_error.h"

void PACK_APIENTRY crPackChromiumParametervCR(CR_PACKER_CONTEXT_ARGDECL GLenum target, GLenum type, GLsizei count, const GLvoid *values)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned int header_length = 2 * sizeof(int) + sizeof(target) + sizeof(type) + sizeof(count);
    unsigned int packet_length;
    unsigned int params_length = 0;
    unsigned char *data_ptr;
    int i, pos;

    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        params_length = sizeof(GLbyte) * count;
        break;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
        params_length = sizeof(GLshort) * count;
        break;
    case GL_INT:
    case GL_UNSIGNED_INT:
        params_length = sizeof(GLint) * count;
        break;
#ifndef IN_RING0
    case GL_FLOAT:
        params_length = sizeof(GLfloat) * count;
        break;
#endif
#if 0
    case GL_DOUBLE:
        params_length = sizeof(GLdouble) * count;
        break;
#endif
    default:
        __PackError( __LINE__, __FILE__, GL_INVALID_ENUM,
                                 "crPackChromiumParametervCR(bad type)" );
        return;
    }

    packet_length = header_length + params_length;

    CR_GET_BUFFERED_POINTER(pc, packet_length );
    WRITE_DATA( 0, GLint, packet_length );
    WRITE_DATA( 4, GLenum, CR_CHROMIUMPARAMETERVCR_EXTEND_OPCODE );
    WRITE_DATA( 8, GLenum, target );
    WRITE_DATA( 12, GLenum, type );
    WRITE_DATA( 16, GLsizei, count );
    WRITE_OPCODE( pc, CR_EXTEND_OPCODE );

    pos = header_length;

    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        for (i = 0; i < count; i++, pos += sizeof(GLbyte)) {
            WRITE_DATA( pos, GLbyte, ((GLbyte *) values)[i]);
        }
        break;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
        for (i = 0; i < count; i++, pos += sizeof(GLshort)) {
            WRITE_DATA( pos, GLshort, ((GLshort *) values)[i]);
        }
        break;
    case GL_INT:
    case GL_UNSIGNED_INT:
        for (i = 0; i < count; i++, pos += sizeof(GLint)) {
            WRITE_DATA( pos, GLint, ((GLint *) values)[i]);
        }
        break;
#ifndef IN_RING0
    case GL_FLOAT:
        for (i = 0; i < count; i++, pos += sizeof(GLfloat)) {
            WRITE_DATA( pos, GLfloat, ((GLfloat *) values)[i]);
        }
        break;
#endif
#if 0
    case GL_DOUBLE:
        for (i = 0; i < count; i++) {
            WRITE_foo_DATA( sizeof(int) + 12, GLdouble, ((GLdouble *) values)[i]);
        }
        break;
#endif
    default:
        __PackError( __LINE__, __FILE__, GL_INVALID_ENUM,
                                 "crPackChromiumParametervCR(bad type)" );
        CR_UNLOCK_PACKER_CONTEXT(pc);
        return;
    }
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

#ifndef IN_RING0
void PACK_APIENTRY crPackDeleteQueriesARB(CR_PACKER_CONTEXT_ARGDECL GLsizei n, const GLuint * ids)
{
    unsigned char *data_ptr;
    int packet_length = sizeof(GLenum)+sizeof(n)+n*sizeof(*ids);
    if (!ids) return;
    data_ptr = (unsigned char *) crPackAlloc(packet_length);
    WRITE_DATA(0, GLenum, CR_DELETEQUERIESARB_EXTEND_OPCODE);
    WRITE_DATA(4, GLsizei, n);
    crMemcpy(data_ptr + 8, ids, n*sizeof(*ids));
    crHugePacket(CR_EXTEND_OPCODE, data_ptr);
    crPackFree(data_ptr);
}
#endif

void PACK_APIENTRY crPackVBoxTexPresent( CR_PACKER_CONTEXT_ARGDECL GLuint texture, GLuint cfg, GLint xPos, GLint yPos, GLint cRects, const GLint * pRects )
{
    GLint i, size, cnt;

    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    (void) pc;
    size = 28 + cRects * 4 * sizeof(GLint);
    CR_GET_BUFFERED_POINTER( pc, size );
    WRITE_DATA( 0, GLint, size );
    WRITE_DATA( 4, GLenum, CR_VBOXTEXPRESENT_EXTEND_OPCODE );
    WRITE_DATA( 8, GLuint, texture );
    WRITE_DATA( 12, GLuint, cfg );
    WRITE_DATA( 16, GLint, xPos );
    WRITE_DATA( 20, GLint, yPos );
    WRITE_DATA( 24, GLint, cRects );

    cnt = 28;
    for (i=0; i<cRects; ++i)
    {
        WRITE_DATA(cnt, GLint, (GLint) pRects[4*i+0]);
        WRITE_DATA(cnt+4, GLint, (GLint) pRects[4*i+1]);
        WRITE_DATA(cnt+8, GLint, (GLint) pRects[4*i+2]);
        WRITE_DATA(cnt+12, GLint, (GLint) pRects[4*i+3]);
        cnt += 16;
    }
    WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackWindowPosition( CR_PACKER_CONTEXT_ARGDECL GLint window, GLint x, GLint y )
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    (void) pc;
    CR_GET_BUFFERED_POINTER( pc, 20 );
    WRITE_DATA( 0, GLint, 20 );
    WRITE_DATA( 4, GLenum, CR_WINDOWPOSITION_EXTEND_OPCODE );
    WRITE_DATA( 8, GLint, window );
    WRITE_DATA( 12, GLint, x );
    WRITE_DATA( 16, GLint, y );
    WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackWindowShow( CR_PACKER_CONTEXT_ARGDECL GLint window, GLint flag )
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    (void) pc;
    CR_GET_BUFFERED_POINTER( pc, 16 );
    WRITE_DATA( 0, GLint, 16 );
    WRITE_DATA( 4, GLenum, CR_WINDOWSHOW_EXTEND_OPCODE );
    WRITE_DATA( 8, GLint, window );
    WRITE_DATA( 12, GLint, flag );
    WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackWindowSize( CR_PACKER_CONTEXT_ARGDECL GLint window, GLint w, GLint h )
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    (void) pc;
    CR_GET_BUFFERED_POINTER( pc, 20 );
    WRITE_DATA( 0, GLint, 20 );
    WRITE_DATA( 4, GLenum, CR_WINDOWSIZE_EXTEND_OPCODE );
    WRITE_DATA( 8, GLint, window );
    WRITE_DATA( 12, GLint, w );
    WRITE_DATA( 16, GLint, h );
    WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackBeginQueryARB( CR_PACKER_CONTEXT_ARGDECL GLenum target, GLuint id )
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    (void) pc;
    CR_GET_BUFFERED_POINTER( pc, 16 );
    WRITE_DATA( 0, GLint, 16 );
    WRITE_DATA( 4, GLenum, CR_BEGINQUERYARB_EXTEND_OPCODE );
    WRITE_DATA( 8, GLenum, target );
    WRITE_DATA( 12, GLuint, id );
    WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

void PACK_APIENTRY crPackEndQueryARB( CR_PACKER_CONTEXT_ARGDECL GLenum target )
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    (void) pc;
    CR_GET_BUFFERED_POINTER( pc, 12 );
    WRITE_DATA( 0, GLint, 12 );
    WRITE_DATA( 4, GLenum, CR_ENDQUERYARB_EXTEND_OPCODE );
    WRITE_DATA( 8, GLenum, target );
    WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_UNLOCK_PACKER_CONTEXT(pc);
}
