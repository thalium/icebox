/* $Id: expandospu.h $ */
/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef EXPANDO_SPU_H
#define EXPANDO_SPU_H

#ifdef WINDOWS
#define EXPANDOSPU_APIENTRY __stdcall
#else
#define EXPANDOSPU_APIENTRY
#endif

#include "cr_glstate.h"
#include "cr_spu.h"
#include "cr_server.h"
#include "cr_dlm.h"

typedef struct {
	int id;
	int has_child;
	SPUDispatchTable self, child, super;
	CRServer *server;

	/* Expando-specific variables */
	CRHashTable *contextTable;
} ExpandoSPU;

typedef struct {
	/* Local copy of state, needed by DLM to compile client-side stuff.
	 * We only collect client-side state; we ignore all server-side
	 * state (we just don't need it).
	 */
	CRContext *State; 

	/* The DLM, and the per-context state for a DLM.  Right now, every
	 * context will have its own DLM; it's possible in OpenGL to share
	 * DLMs, but the Chromium interface doesn't allow it yet.
	 */
	CRDLM *dlm;
	CRDLMContextState *dlmContext;
} ExpandoContextState;

extern ExpandoSPU expando_spu;

extern SPUNamedFunctionTable _cr_expando_table[];

extern SPUOptions expandoSPUOptions[];

extern void expandospuGatherConfiguration( void );

extern void expando_free_context_state(void *data);

extern GLint EXPANDOSPU_APIENTRY expandoCreateContext(const char *displayName, GLint visBits, GLint shareCtx);
extern void EXPANDOSPU_APIENTRY expandoDestroyContext(GLint contextId);
extern void EXPANDOSPU_APIENTRY expandoMakeCurrent(GLint crWindow, GLint nativeWindow, GLint contextId);
extern void EXPANDOSPU_APIENTRY expandoNewList(GLuint list, GLenum mode);
extern void EXPANDOSPU_APIENTRY expandoEndList(void);
extern void EXPANDOSPU_APIENTRY expandoDeleteLists(GLuint first, GLsizei range);
extern GLuint EXPANDOSPU_APIENTRY expandoGenLists(GLsizei range);
extern void EXPANDOSPU_APIENTRY expandoListBase(GLuint base);
extern GLboolean EXPANDOSPU_APIENTRY expandoIsList(GLuint list);
extern  void EXPANDOSPU_APIENTRY expandoCallList(GLuint list);
extern void EXPANDOSPU_APIENTRY expandoCallLists(GLsizei n, GLenum type, const GLvoid *lists);

#endif /* EXPANDO_SPU_H */
