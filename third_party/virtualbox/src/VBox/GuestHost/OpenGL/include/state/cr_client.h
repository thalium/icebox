/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_CLIENT_H
#define CR_STATE_CLIENT_H

#include "state/cr_statetypes.h"
#include "state/cr_limits.h"
#include "state/cr_bufferobject.h"
#include "cr_bits.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    CRbitvalue  dirty[CR_MAX_BITARRAY];
    /* pixel pack/unpack */
    CRbitvalue  pack[CR_MAX_BITARRAY];
    CRbitvalue  unpack[CR_MAX_BITARRAY];
    /* vertex array */
    CRbitvalue  enableClientState[CR_MAX_BITARRAY];
    CRbitvalue  clientPointer[CR_MAX_BITARRAY];
    CRbitvalue  *v; /* vertex */
    CRbitvalue  *n; /* normal */
    CRbitvalue  *c; /* color */
    CRbitvalue  *i; /* index */
    CRbitvalue  *t[CR_MAX_TEXTURE_UNITS]; /* texcoord */
    CRbitvalue  *e; /* edgeflag */
    CRbitvalue  *s; /* secondary color */
    CRbitvalue  *f; /* fog coord */
#ifdef CR_NV_vertex_program
    CRbitvalue  *a[CR_MAX_VERTEX_ATTRIBS]; /* NV_vertex_program */
#endif
} CRClientBits;

/*
 * NOTE!!!! If you change this structure, search through the code for
 * occurrences of 'defaultPacking' and fix the static initializations!!!!
 */
typedef struct {
    GLint       rowLength;
    GLint       skipRows;
    GLint       skipPixels;
    GLint       alignment;
    GLint       imageHeight;
    GLint       skipImages;
    GLboolean   swapBytes;
    GLboolean   psLSBFirst; /* don't conflict with crap from Xlib.h */
} CRPixelPackState;

typedef struct {
    unsigned char *p;
    GLint size;
    GLint type;
    GLint stride;
    GLboolean enabled;
    GLboolean normalized; /* Added with GL_ARB_vertex_program */
    int bytesPerIndex;
#ifdef CR_ARB_vertex_buffer_object
    CRBufferObject *buffer;
#endif
#ifdef CR_EXT_compiled_vertex_array
    GLboolean locked;
    unsigned char *prevPtr;
    GLint   prevStride;
#endif
} CRClientPointer;

typedef struct {
    CRClientPointer v;                         /* vertex */
    CRClientPointer n;                         /* normal */
    CRClientPointer c;                         /* color */
    CRClientPointer i;                         /* color index */
    CRClientPointer t[CR_MAX_TEXTURE_UNITS];   /* texcoords */
    CRClientPointer e;                         /* edge flags */
    CRClientPointer s;                         /* secondary color */
    CRClientPointer f;                         /* fog coord */
#ifdef CR_NV_vertex_program
    CRClientPointer a[CR_MAX_VERTEX_ATTRIBS];  /* vertex attribs */
#endif
#ifdef CR_NV_vertex_array_range
    GLboolean arrayRange;
    GLboolean arrayRangeValid;
    void *arrayRangePointer;
    GLuint arrayRangeLength;
#endif
#ifdef CR_EXT_compiled_vertex_array
    GLint lockFirst;
    GLint lockCount;
    GLboolean locked;
# ifdef IN_GUEST
    GLboolean synced;
# endif
#endif
} CRVertexArrays;

#define CRSTATECLIENT_MAX_VERTEXARRAYS (7+CR_MAX_TEXTURE_UNITS+CR_MAX_VERTEX_ATTRIBS)

typedef struct {
    /* pixel pack/unpack */
    CRPixelPackState pack;
    CRPixelPackState unpack;

    CRVertexArrays array;

    GLint curClientTextureUnit;

    /* state stacks (glPush/PopClientState) */
    GLint attribStackDepth;
    CRbitvalue pushMaskStack[CR_MAX_CLIENT_ATTRIB_STACK_DEPTH];

    GLint pixelStoreStackDepth;
    CRPixelPackState pixelPackStoreStack[CR_MAX_CLIENT_ATTRIB_STACK_DEPTH];
    CRPixelPackState pixelUnpackStoreStack[CR_MAX_CLIENT_ATTRIB_STACK_DEPTH];

    GLint vertexArrayStackDepth;
    CRVertexArrays vertexArrayStack[CR_MAX_CLIENT_ATTRIB_STACK_DEPTH];
} CRClientState;

extern const CRPixelPackState crStateNativePixelPacking;

struct CRContext;

DECLEXPORT(void) crStateClientInitBits(CRClientBits *c);
DECLEXPORT(void) crStateClientDestroyBits(CRClientBits *c);
DECLEXPORT(void) crStateClientInit(struct CRContext *g);
DECLEXPORT(void) crStateClientDestroy(struct CRContext *g);

DECLEXPORT(GLboolean) crStateUseServerArrays(void);
DECLEXPORT(GLboolean) crStateUseServerArrayElements(void);
DECLEXPORT(CRClientPointer*) crStateGetClientPointerByIndex(int index, CRVertexArrays *array);

#ifdef __cplusplus
}
#endif

#endif /* CR_CLIENT_H */
