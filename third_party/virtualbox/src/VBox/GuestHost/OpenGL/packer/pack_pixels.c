/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "packer.h"
#include "cr_pixeldata.h"
#include "cr_error.h"
#include "cr_mem.h"
#include "cr_version.h"


void PACK_APIENTRY crPackDrawPixels(GLsizei width, GLsizei height, 
                                    GLenum format, GLenum type,
                                    const GLvoid *pixels,
                                    const CRPixelPackState *unpackstate )
{
    unsigned char *data_ptr;
    int packet_length, imagesize;
    int noimagedata = (pixels == NULL) || crStateIsBufferBound(g_pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB);

    packet_length = 
        sizeof( width ) +
        sizeof( height ) +
        sizeof( format ) +
        sizeof( type ) + sizeof(int) + sizeof(GLint);

    if (!noimagedata)
    {
        imagesize = crImageSize( format, type, width, height );

        if (imagesize<=0)
        {
            crDebug("crPackDrawPixels: 0 image size, ignoring");
            return;
        }
        packet_length += imagesize;
    }

    data_ptr = (unsigned char *) crPackAlloc( packet_length );
    WRITE_DATA( 0, GLsizei, width );
    WRITE_DATA( 4, GLsizei, height );
    WRITE_DATA( 8, GLenum, format );
    WRITE_DATA( 12, GLenum, type );
    WRITE_DATA( 16, GLint, noimagedata );
    WRITE_DATA( 20, GLint, (GLint) (uintptr_t) pixels );

    if (!noimagedata)
    {
        crPixelCopy2D(width, height,
                      (void *) (data_ptr + 24), format, type, NULL, /* dst */
                      pixels, format, type, unpackstate);  /* src */
    }

    crHugePacket( CR_DRAWPIXELS_OPCODE, data_ptr );
    crPackFree( data_ptr );
}

void PACK_APIENTRY crPackReadPixels(GLint x, GLint y, GLsizei width, 
                                    GLsizei height, GLenum format,
                                    GLenum type, GLvoid *pixels,
                                    const CRPixelPackState *packstate,
                                    int *writeback)
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    GLint stride = 0;
    GLint bytes_per_row;
    int bytes_per_pixel;
    *writeback = 0;

    bytes_per_pixel = crPixelSize(format, type);
    if (bytes_per_pixel <= 0) {
        char string[80];
        sprintf(string, "crPackReadPixels(format 0x%x or type 0x%x)", format, type);
        __PackError(__LINE__, __FILE__, GL_INVALID_ENUM, string);
        return;
    }

    /* default bytes_per_row so crserver can allocate memory */
    bytes_per_row = width * bytes_per_pixel;

    stride = bytes_per_row;
    if (packstate->alignment != 1) {
         GLint remainder = bytes_per_row % packstate->alignment;
         if (remainder)
                stride = bytes_per_row + (packstate->alignment - remainder);
    }

    CR_GET_BUFFERED_POINTER(pc, 48 + sizeof(CRNetworkPointer) );
    WRITE_DATA( 0,  GLint,  x );
    WRITE_DATA( 4,  GLint,  y );
    WRITE_DATA( 8,  GLsizei,  width );
    WRITE_DATA( 12, GLsizei,  height );
    WRITE_DATA( 16, GLenum, format );
    WRITE_DATA( 20, GLenum, type );
    WRITE_DATA( 24, GLint,  stride ); /* XXX not really used! */
    WRITE_DATA( 28, GLint, packstate->alignment );
    WRITE_DATA( 32, GLint, packstate->skipRows );
    WRITE_DATA( 36, GLint, packstate->skipPixels );
    WRITE_DATA( 40, GLint, bytes_per_row );
    WRITE_DATA( 44, GLint, packstate->rowLength );
    WRITE_NETWORK_POINTER( 48, (char *) pixels );
    WRITE_OPCODE( pc, CR_READPIXELS_OPCODE );
    CR_CMDBLOCK_CHECK_FLUSH(pc);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}

/* Round N up to the next multiple of 8 */
#define CEIL8(N)  (((N) + 7) & ~0x7)

void PACK_APIENTRY crPackBitmap(GLsizei width, GLsizei height, 
                                GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove,
                                const GLubyte *bitmap, const CRPixelPackState *unpack )
{
    const int noimagedata = (bitmap == NULL) || crStateIsBufferBound(g_pStateTracker, GL_PIXEL_UNPACK_BUFFER_ARB);
    unsigned char *data_ptr;
    int data_length = 0;
    /*GLubyte *destBitmap = NULL; - unused */
    int packet_length = 
        sizeof( width ) + 
        sizeof( height ) +
        sizeof( xorig ) + 
        sizeof( yorig ) + 
        sizeof( xmove ) + 
        sizeof( ymove ) +
        sizeof( GLuint ) + sizeof(GLint);

    if (!noimagedata)
    {
        data_length = CEIL8(width) * height / 8;
        packet_length += data_length;
    }

    data_ptr = (unsigned char *) crPackAlloc( packet_length );

    WRITE_DATA( 0, GLsizei, width );
    WRITE_DATA( 4, GLsizei, height );
    WRITE_DATA( 8, GLfloat, xorig );
    WRITE_DATA( 12, GLfloat, yorig );
    WRITE_DATA( 16, GLfloat, xmove );
    WRITE_DATA( 20, GLfloat, ymove );
    WRITE_DATA( 24, GLuint, noimagedata );
    WRITE_DATA( 28, GLint, (GLint) (uintptr_t) bitmap);

    if (!noimagedata)
        crBitmapCopy(width, height, (GLubyte *)(data_ptr + 32), bitmap, unpack);

    crHugePacket( CR_BITMAP_OPCODE, data_ptr );
    crPackFree( data_ptr );
}
/*
       ZPix - compressed DrawPixels
*/
void PACK_APIENTRY crPackZPixCR( GLsizei width, GLsizei height, 
        GLenum format, GLenum type,
        GLenum ztype, GLint zparm, GLint length,
        const GLvoid *pixels,
        const CRPixelPackState *unpackstate )
{
    unsigned char *data_ptr;
    int packet_length;
    (void)unpackstate;

    if (pixels == NULL)
    {
        return;
    }

    packet_length = 
        sizeof( int ) +  /* packet size */
        sizeof( GLenum ) +  /* extended opcode */
        sizeof( width ) +
        sizeof( height ) +
        sizeof( format ) +
        sizeof( type ) +
        sizeof( ztype ) +
        sizeof( zparm ) +
        sizeof( length );

    packet_length += length;

/* XXX JAG
  crDebug("PackZPixCR: fb %d x %d, state %d, zlen = %d, plen = %d",
                               width, height, ztype, length, packet_length);
*/
    data_ptr = (unsigned char *) crPackAlloc( packet_length );
    WRITE_DATA(  0, GLenum , CR_ZPIXCR_EXTEND_OPCODE );
    WRITE_DATA(  4, GLsizei, width );
    WRITE_DATA(  8, GLsizei, height );
    WRITE_DATA( 12, GLenum,  format );
    WRITE_DATA( 16, GLenum,  type );
    WRITE_DATA( 20, GLenum,  ztype );
    WRITE_DATA( 24, GLint,   zparm );
    WRITE_DATA( 28, GLint,   length );

    crMemcpy((void *) (data_ptr+32), pixels, length);

    crHugePacket( CR_EXTEND_OPCODE, data_ptr );
    crPackFree( data_ptr );
}


void PACK_APIENTRY
crPackGetTexImage( GLenum target, GLint level, GLenum format, GLenum type,
                                     GLvoid * pixels, const CRPixelPackState * packstate,
                                     int * writeback )
{
    CR_GET_PACKER_CONTEXT(pc);
    unsigned char *data_ptr;
    (void) pc; (void) packstate;
    CR_GET_BUFFERED_POINTER( pc, 40 );
    WRITE_DATA( 0, GLint, 40 );
    WRITE_DATA( 4, GLenum, CR_GETTEXIMAGE_EXTEND_OPCODE );
    WRITE_DATA( 8, GLenum, target );
    WRITE_DATA( 12, GLint, level );
    WRITE_DATA( 16, GLenum, format );
    WRITE_DATA( 20, GLenum, type );
    WRITE_NETWORK_POINTER( 24, (void *) pixels );
    WRITE_NETWORK_POINTER( 32, (void *) writeback );
    WRITE_OPCODE( pc, CR_EXTEND_OPCODE );
    CR_CMDBLOCK_CHECK_FLUSH(pc);
    CR_UNLOCK_PACKER_CONTEXT(pc);
}
