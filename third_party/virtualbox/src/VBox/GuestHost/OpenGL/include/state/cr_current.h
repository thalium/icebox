/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_CURRENT_H
#define CR_STATE_CURRENT_H

#include "state/cr_currentpointers.h"

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif


#define VERT_ATTRIB_POS      0
#define VERT_ATTRIB_WEIGHT   1
#define VERT_ATTRIB_NORMAL   2
#define VERT_ATTRIB_COLOR0   3
#define VERT_ATTRIB_COLOR1   4
#define VERT_ATTRIB_FOG      5
#define VERT_ATTRIB_SIX      6
#define VERT_ATTRIB_SEVEN    7
#define VERT_ATTRIB_TEX0     8
#define VERT_ATTRIB_TEX1     9
#define VERT_ATTRIB_TEX2     10
#define VERT_ATTRIB_TEX3     11
#define VERT_ATTRIB_TEX4     12
#define VERT_ATTRIB_TEX5     13
#define VERT_ATTRIB_TEX6     14
#define VERT_ATTRIB_TEX7     15
#define VERT_ATTRIB_MAX      16


typedef struct {
	CRbitvalue  dirty[CR_MAX_BITARRAY];
	/* Regardless of NV_vertex_program, we use this array */
	CRbitvalue  vertexAttrib[CR_MAX_VERTEX_ATTRIBS][CR_MAX_BITARRAY];
	CRbitvalue  edgeFlag[CR_MAX_BITARRAY];
	CRbitvalue  colorIndex[CR_MAX_BITARRAY];
	CRbitvalue  rasterPos[CR_MAX_BITARRAY];
} CRCurrentBits;


typedef struct {
	/* Pre-transform values */
	/* Regardless of NV_vertex_program, we use this array */
	GLfloat   attrib[CR_MAX_VERTEX_ATTRIBS][4];
	GLboolean edgeFlag;
	GLfloat   colorIndex;
	/* Post-transform values */
	GLvectorf	eyePos;
	GLvectorf	clipPos;
	GLvectorf	winPos;
} CRVertex;


typedef struct {
	/* Regardless of NV_vertex_program, we use this array */
	GLfloat  vertexAttrib[CR_MAX_VERTEX_ATTRIBS][4];
	GLfloat  vertexAttribPre[CR_MAX_VERTEX_ATTRIBS][4];

	CRCurrentStatePointers   *current;

	GLboolean    rasterValid;
	GLfloat      rasterAttrib[CR_MAX_VERTEX_ATTRIBS][4];
	GLfloat      rasterAttribPre[CR_MAX_VERTEX_ATTRIBS][4];

	GLdouble     rasterIndex;
	GLboolean    edgeFlag;
	GLboolean    edgeFlagPre;
	GLfloat      colorIndex;
	GLfloat      colorIndexPre;

	/* XXX this isn't really "current" state - move someday */
	GLuint       attribsUsedMask;  /* for ARB_vertex_program */
	GLboolean    inBeginEnd;
	GLenum       mode;
	GLuint       beginEndMax;
	GLuint       beginEndNum;
	GLuint       flushOnEnd;

} CRCurrentState;

DECLEXPORT(void) crStateCurrentInit( CRContext *ctx );

DECLEXPORT(void) crStateCurrentRecover( void );

DECLEXPORT(void) crStateCurrentRecoverNew(CRContext *g, CRCurrentStatePointers  *current);

DECLEXPORT(void) crStateCurrentDiff(CRCurrentBits *bb, CRbitvalue *bitID,
                                    CRContext *fromCtx, CRContext *toCtx);
DECLEXPORT(void) crStateCurrentSwitch(CRCurrentBits *bb, CRbitvalue *bitID,
                                    CRContext *fromCtx, CRContext *toCtx);

DECLEXPORT(void) crStateRasterPosUpdate(GLfloat x, GLfloat y, GLfloat z, GLfloat w);

DECLEXPORT(GLuint) crStateNeedDummyZeroVertexArray(CRContext *g, CRCurrentStatePointers  *current, GLfloat *pZva);

#ifdef __cplusplus
}
#endif

#endif /* CR_STATE_CURRENT_H */
