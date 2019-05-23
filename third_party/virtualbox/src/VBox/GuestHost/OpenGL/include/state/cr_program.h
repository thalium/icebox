/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_STATE_PROGRAM_H
#define CR_STATE_PROGRAM_H

#include "cr_hash.h"
#include "state/cr_statetypes.h"
#include "state/cr_limits.h"

#include <iprt/cdefs.h>

/*
 * Dirty bits for per-context program state.  Per-program dirty bits
 * are in the CRProgram structure.
 */
typedef struct {
    CRbitvalue dirty[CR_MAX_BITARRAY];
    CRbitvalue vpEnable[CR_MAX_BITARRAY];
    CRbitvalue fpEnable[CR_MAX_BITARRAY];
    CRbitvalue vpBinding[CR_MAX_BITARRAY];
    CRbitvalue fpBinding[CR_MAX_BITARRAY];
    CRbitvalue vertexAttribArrayEnable[CR_MAX_VERTEX_ATTRIBS][CR_MAX_BITARRAY];
    CRbitvalue map1AttribArrayEnable[CR_MAX_VERTEX_ATTRIBS][CR_MAX_BITARRAY];
    CRbitvalue map2AttribArrayEnable[CR_MAX_VERTEX_ATTRIBS][CR_MAX_BITARRAY];
    /* per-param flags: */
    CRbitvalue vertexEnvParameter[CR_MAX_VERTEX_PROGRAM_ENV_PARAMS][CR_MAX_BITARRAY];
    CRbitvalue fragmentEnvParameter[CR_MAX_FRAGMENT_PROGRAM_ENV_PARAMS][CR_MAX_BITARRAY];
    /* any param flags: */
    CRbitvalue vertexEnvParameters[CR_MAX_BITARRAY];
    CRbitvalue fragmentEnvParameters[CR_MAX_BITARRAY];
    CRbitvalue trackMatrix[CR_MAX_VERTEX_PROGRAM_ENV_PARAMS / 4][CR_MAX_BITARRAY];
} CRProgramBits;


/*
 * Fragment programs have named symbols which are defined/declared
 * within the fragment program that can also be set with the
 * glProgramNamedParameter4*NV() functions.
 * We keep a linked list of these CRProgramSymbol structures to implement
 * a symbol table.  A simple linked list is sufficient since a fragment
 * program typically has just a few symbols.
 */
typedef struct CRProgramSymbol {
    const char *name;
    GLuint     cbName;
    GLfloat value[4];
    CRbitvalue dirty[CR_MAX_BITARRAY];
    struct CRProgramSymbol *next;
} CRProgramSymbol;


/*
 * A vertex or fragment program.
 */
typedef struct {
    GLenum target;
    GLuint id;
    GLboolean isARBprogram;  /* to distinguish between NV and ARB programs */
    const GLubyte *string;
    GLsizei length;
    GLboolean resident;
    GLenum format;

    /* Set with ProgramNamedParameterNV */
    struct CRProgramSymbol *symbolTable;

    /* Set with ProgramLocalParameterARB: */
    GLfloat parameters[CR_MAX_PROGRAM_LOCAL_PARAMS][4];

    /* ARB info (this could be impossible to implement without parsing */
    GLint numInstructions;
    GLint numTemporaries;
    GLint numParameters;
    GLint numAttributes;
    GLint numAddressRegs;
    GLint numAluInstructions;
    GLint numTexInstructions;
    GLint numTexIndirections;

    CRbitvalue dirtyNamedParams[CR_MAX_BITARRAY];
    CRbitvalue dirtyParam[CR_MAX_PROGRAM_LOCAL_PARAMS][CR_MAX_BITARRAY];
    CRbitvalue dirtyParams[CR_MAX_BITARRAY];
    CRbitvalue dirtyProgram[CR_MAX_BITARRAY];
} CRProgram;



typedef struct {
    CRProgram *currentVertexProgram;
    CRProgram *currentFragmentProgram;
    GLint errorPos;
    const GLubyte *errorString;
    GLboolean loadedProgram;    /* XXX temporary */

    CRProgram *defaultVertexProgram;
    CRProgram *defaultFragmentProgram;

    /* tracking matrices for vertex programs */
#ifdef VBOX /* see state_program.c */
    GLenum TrackMatrix[CR_MAX_VERTEX_PROGRAM_ENV_PARAMS / 4];
    GLenum TrackMatrixTransform[CR_MAX_VERTEX_PROGRAM_ENV_PARAMS / 4];
#else
    GLenum TrackMatrix[CR_MAX_VERTEX_PROGRAM_LOCAL_PARAMS / 4];
    GLenum TrackMatrixTransform[CR_MAX_VERTEX_PROGRAM_LOCAL_PARAMS / 4];
#endif

    /* global/env params shared by all programs */
    GLfloat fragmentParameters[CR_MAX_FRAGMENT_PROGRAM_ENV_PARAMS][4];
    GLfloat vertexParameters[CR_MAX_VERTEX_PROGRAM_ENV_PARAMS][4];

    CRHashTable *programHash;  /* XXX belongs in shared state, actually */

    GLuint vpProgramBinding;
    GLuint fpProgramBinding;
    GLboolean vpEnabled;    /* GL_VERTEX_PROGRAM_NV / ARB*/
    GLboolean fpEnabled;    /* GL_FRAGMENT_PROGRAM_NV */
    GLboolean fpEnabledARB; /* GL_FRAGMENT_PROGRAM_ARB */
    GLboolean vpPointSize;  /* GL_VERTEX_PROGRAM_NV */
    GLboolean vpTwoSide;    /* GL_VERTEX_PROGRAM_NV */

    /* Indicates that we have to resend program data to GPU on first glMakeCurrent call with owning context */
    GLboolean   bResyncNeeded;

} CRProgramState;



extern DECLEXPORT(void) crStateProgramInit(CRContext *ctx);
extern DECLEXPORT(void) crStateProgramDestroy(CRContext *ctx);

extern DECLEXPORT(void) crStateProgramDiff(CRProgramBits *b, CRbitvalue *bitID,
                                           CRContext *fromCtx, CRContext *toCtx);
extern DECLEXPORT(void) crStateProgramSwitch(CRProgramBits *b, CRbitvalue *bitID,
                                             CRContext *fromCtx, CRContext *toCtx);

DECLEXPORT(void) crStateDiffAllPrograms(CRContext *g, CRbitvalue *bitID, GLboolean bForceUpdate);

#endif /* CR_STATE_PROGRAM_H */
