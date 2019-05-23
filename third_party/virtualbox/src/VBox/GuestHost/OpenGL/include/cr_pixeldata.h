/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_PIXELDATA_H
#define CR_PIXELDATA_H

#include "chromium.h"
#include "state/cr_client.h"

#include <iprt/cdefs.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLEXPORT(int) crPixelSize( GLenum format, GLenum type );

DECLEXPORT(unsigned int) crImageSize( GLenum format, GLenum type,
													GLsizei width, GLsizei height );

DECLEXPORT(unsigned int) crTextureSize( GLenum format, GLenum type, GLsizei width, GLsizei height, GLsizei depth );

DECLEXPORT(void) crPixelCopy1D( GLvoid *dstPtr, GLenum dstFormat, GLenum dstType,
										const GLvoid *srcPtr, GLenum srcFormat, GLenum srcType,
										GLsizei width, const CRPixelPackState *srcPacking );

DECLEXPORT(void) crPixelCopy2D( GLsizei width, GLsizei height,
										GLvoid *dstPtr, GLenum dstFormat, GLenum dstType,
										const CRPixelPackState *dstPacking,
										const GLvoid *srcPtr, GLenum srcFormat, GLenum srcType,
										const CRPixelPackState *srcPacking );

DECLEXPORT(void) crPixelCopy3D( GLsizei width, GLsizei height, GLsizei depth,
										GLvoid *dstPtr, GLenum dstFormat, GLenum dstType,
										const CRPixelPackState *dstPacking, const GLvoid *srcPtr,
										GLenum srcFormat, GLenum srcType,
										const CRPixelPackState *srcPacking );

DECLEXPORT(void) crBitmapCopy( GLsizei width, GLsizei height, GLubyte *dstPtr,
									 const GLubyte *srcPtr, const CRPixelPackState *srcPacking );

DECLEXPORT(void) crDumpNamedTGA(const char *fname, GLint w, GLint h, GLvoid *data);
DECLEXPORT(void) crDumpNamedTGAV(GLint w, GLint h, GLvoid *data, const char* fname, va_list va);
DECLEXPORT(void) crDumpNamedTGAF(GLint w, GLint h, GLvoid *data, const char* fname, ...);
DECLEXPORT(void) crDumpTGA(GLint w, GLint h, GLvoid *data);
#ifdef __cplusplus
}
#endif

#endif /* CR_PIXELDATA_H */
