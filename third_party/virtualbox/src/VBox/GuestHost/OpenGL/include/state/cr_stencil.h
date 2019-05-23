/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_STENCIL_H
#define CR_STATE_STENCIL_H

#include "cr_glstate.h"
#include "state/cr_statetypes.h"

#include <iprt/cdefs.h>
#include <iprt/assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CRSTATE_STENCIL_BUFFER_ID_FRONT         0
#define CRSTATE_STENCIL_BUFFER_ID_BACK          1
#define CRSTATE_STENCIL_BUFFER_ID_TWO_SIDE_BACK 2
#define CRSTATE_STENCIL_BUFFER_COUNT            3

/* stencil buffer settings were accessed with StencilXxx with ActiveStencilFaceEXT == GL_FRONT or StencilXxxSeparate(GL_FRONT_AND_BACK) */
#define CRSTATE_STENCIL_BUFFER_REF_ID_FRONT_AND_BACK     0
/* stencil buffer settings were accessed with StencilXxxSeparate(GL_FRONT_FRONT) */
#define CRSTATE_STENCIL_BUFFER_REF_ID_FRONT              1
/* stencil buffer settings were accessed with StencilXxxSeparate(GL_FRONT_BACK) */
#define CRSTATE_STENCIL_BUFFER_REF_ID_BACK               2
/* stencil buffer settings were accessed with StencilXxx with ActiveStencilFaceEXT == GL_BACK */
#define CRSTATE_STENCIL_BUFFER_REF_ID_TWO_SIDE_BACK      3
#define CRSTATE_STENCIL_BUFFER_REF_COUNT                 4

typedef struct {
    CRbitvalue dirty[CR_MAX_BITARRAY];
    CRbitvalue enable[CR_MAX_BITARRAY];
    CRbitvalue func[CR_MAX_BITARRAY];
    CRbitvalue op[CR_MAX_BITARRAY];
    CRbitvalue clearValue[CR_MAX_BITARRAY];
    CRbitvalue writeMask[CR_MAX_BITARRAY];
} CRStencilBits_v_33;

typedef struct {
    CRbitvalue func[CR_MAX_BITARRAY];
    CRbitvalue op[CR_MAX_BITARRAY];
} CRStencilBufferRefBits;

typedef struct {
    CRbitvalue dirty[CR_MAX_BITARRAY];
    CRbitvalue enable[CR_MAX_BITARRAY];
    CRbitvalue enableTwoSideEXT[CR_MAX_BITARRAY];
    CRbitvalue activeStencilFace[CR_MAX_BITARRAY];
    CRbitvalue clearValue[CR_MAX_BITARRAY];
    CRbitvalue writeMask[CR_MAX_BITARRAY];
    /* note: here we use _BUFFER_REF_ rather than _REF_ because we track the way buffers are accessed here,
     * to ensure the correct function is called on hw->chromium state restoration,
     * i.e. we want to avoid always calling StencilXxxSeparate, but call StencilXxx when it was actually called */
    CRStencilBufferRefBits bufferRefs[CRSTATE_STENCIL_BUFFER_REF_COUNT];
} CRStencilBits;

typedef struct {
    GLboolean   stencilTest;
    GLenum      func;
    GLint       mask;
    GLint       ref;
    GLenum      fail;
    GLenum      passDepthFail;
    GLenum      passDepthPass;
    GLint       clearValue;
    GLint       writeMask;
} CRStencilState_v_33;

typedef struct {
    GLenum      func;
    GLint       mask;
    GLint       ref;
    GLenum      fail;
    GLenum      passDepthFail;
    GLenum      passDepthPass;
} CRStencilBufferState;

typedef struct {
    /* true if stencil test is enabled */
	GLboolean	stencilTest;
	/* true if GL_EXT_stencil_two_side is enabled (glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT)) */
	GLboolean   stencilTwoSideEXT;
	/* GL_FRONT or GL_BACK */
    GLenum      activeStencilFace;
    GLint       clearValue;
    GLint       writeMask;
	CRStencilBufferState buffers[CRSTATE_STENCIL_BUFFER_COUNT];
} CRStencilState;

DECLEXPORT(void) crStateStencilInit(CRContext *ctx);
DECLEXPORT(void) crStateStencilBufferInit(CRStencilBufferState *s);

DECLEXPORT(void) crStateStencilDiff(CRStencilBits *bb, CRbitvalue *bitID,
                                    CRContext *fromCtx, CRContext *toCtx);
DECLEXPORT(void) crStateStencilSwitch(CRStencilBits *bb, CRbitvalue *bitID, 
                                      CRContext *fromCtx, CRContext *toCtx);

#ifdef __cplusplus
}
#endif

#endif /* CR_STATE_STENCIL_H */
