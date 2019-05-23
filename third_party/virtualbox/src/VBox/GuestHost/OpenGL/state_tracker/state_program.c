/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "state.h"
#include "state_internals.h"
#include "cr_mem.h"
#include "cr_string.h"


/*
 * General notes:
 *
 * Vertex programs can change vertices so bounding boxes may not be
 * practical for tilesort.  Tilesort may have to broadcast geometry
 * when vertex programs are in effect.  We could semi-parse vertex
 * programs to determine if they write to the o[HPOS] register.
 */


/*
 * Lookup the named program and return a pointer to it.
 * If the program doesn't exist, create it and reserve its Id and put
 * it into the hash table.
 */
static CRProgram *
GetProgram(CRProgramState *p, GLenum target, GLuint id)
{
    CRProgram *prog;

    prog = crHashtableSearch(p->programHash, id);
    if (!prog) {
        prog = (CRProgram *) crCalloc(sizeof(CRProgram));
        if (!prog)
            return NULL;
        prog->target = target;
        prog->id = id;
        prog->format = GL_PROGRAM_FORMAT_ASCII_ARB;
        prog->resident = GL_TRUE;
        prog->symbolTable = NULL;
    
        if (id > 0)
            crHashtableAdd(p->programHash, id, (void *) prog);
    }
    return prog;
}


/*
 * Delete a CRProgram object and all attached data.
 */
static void
DeleteProgram(CRProgram *prog)
{
    CRProgramSymbol *symbol, *next;

    if (prog->string)
        crFree((void *) prog->string);

    for (symbol = prog->symbolTable; symbol; symbol = next) {
        next = symbol->next;
        crFree((void *) symbol->name);
        crFree(symbol);
    }
    crFree(prog);
}


/*
 * Set the named symbol to the value (x, y, z, w).
 * NOTE: Symbols should only really be added during parsing of the program.
 * However, the state tracker does not parse the programs (yet).  So, when
 * someone calls glProgramNamedParameter4fNV() we always enter the symbol
 * since we don't know if it's really valid or not.
 */
static void
SetProgramSymbol(CRProgram *prog, const char *name, GLsizei len,
                                 GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    CRProgramSymbol *symbol;

    for (symbol = prog->symbolTable; symbol; symbol = symbol->next) {
        /* NOTE: <name> may not be null-terminated! */
        if (crStrncmp(symbol->name, name, len) == 0 && symbol->name[len] == 0) {
            /* found it */
            symbol->value[0] = x;
            symbol->value[1] = y;
            symbol->value[2] = z;
            symbol->value[3] = w;
            FILLDIRTY(symbol->dirty);
            return;
        }
    }
    /* add new symbol table entry */
    symbol = (CRProgramSymbol *) crAlloc(sizeof(CRProgramSymbol));
    if (symbol) {
       symbol->name = crStrndup(name, len);
       symbol->cbName = len;
       symbol->value[0] = x;
       symbol->value[1] = y;
       symbol->value[2] = z;
       symbol->value[3] = w;
       symbol->next = prog->symbolTable;
       prog->symbolTable = symbol;
       FILLDIRTY(symbol->dirty);
    }
}


/*
 * Return a pointer to the values for the given symbol.  Return NULL if
 * the name doesn't exist in the symbol table.
 */
static const GLfloat *
GetProgramSymbol(const CRProgram *prog, const char *name, GLsizei len)
{
    CRProgramSymbol *symbol = prog->symbolTable;
    for (symbol = prog->symbolTable; symbol; symbol = symbol->next) {
        /* NOTE: <name> may not be null-terminated! */
        if (crStrncmp(symbol->name, name, len) == 0 && symbol->name[len] == 0) {
            return symbol->value;
        }
    }
    return NULL;
}


/*
 * Used by both glBindProgramNV and glBindProgramARB
 */
static CRProgram *
BindProgram(GLenum target, GLuint id,
                        GLenum vertexTarget, GLenum fragmentTarget)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    CRStateBits *sb = GetCurrentBits();
    CRProgramBits *pb = &(sb->program);
    CRProgram *prog;

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glBindProgram called in Begin/End");
        return NULL;
    }

    if (id == 0) {
        if (target == vertexTarget) {
            prog = p->defaultVertexProgram;
        }
        else if (target == fragmentTarget) {
            prog = p->defaultFragmentProgram;
        }
        else {
            crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                     "glBindProgram(bad target)");
            return NULL;
        }
    }
    else {
        prog = GetProgram(p, target, id );
    }

    if (!prog) {
        crStateError(__LINE__, __FILE__, GL_OUT_OF_MEMORY, "glBindProgram");
        return NULL;
    }
    else if (prog->target != target) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glBindProgram target mismatch");
        return NULL;
    }

    if (target == vertexTarget) {
        p->currentVertexProgram = prog;
        p->vpProgramBinding = id;
        DIRTY(pb->dirty, g->neg_bitid);
        DIRTY(pb->vpBinding, g->neg_bitid);
    }
    else if (target == fragmentTarget) {
        p->currentFragmentProgram = prog;
        p->fpProgramBinding = id;
        DIRTY(pb->dirty, g->neg_bitid);
        DIRTY(pb->fpBinding, g->neg_bitid);
    }
    return prog;
}


void STATE_APIENTRY crStateBindProgramNV(GLenum target, GLuint id)
{
    CRProgram *prog = BindProgram(target, id, GL_VERTEX_PROGRAM_NV,
                                  GL_FRAGMENT_PROGRAM_NV);
    if (prog) {
        prog->isARBprogram = GL_FALSE;
    }
}


void STATE_APIENTRY crStateBindProgramARB(GLenum target, GLuint id)
{
    CRProgram *prog = BindProgram(target, id, GL_VERTEX_PROGRAM_ARB,
                                  GL_FRAGMENT_PROGRAM_ARB);
    if (prog) {
        prog->isARBprogram = GL_TRUE;
    }
}


void STATE_APIENTRY crStateDeleteProgramsARB(GLsizei n, const GLuint *ids)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    CRStateBits *sb = GetCurrentBits();
    CRProgramBits *pb = &(sb->program);
    GLint i;

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glDeleteProgramsNV called in Begin/End");
        return;
    }

    if (n < 0)
    {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glDeleteProgramsNV(n)");
        return;
    }

    for (i = 0; i < n; i++) {
        CRProgram *prog;
        if (ids[i] > 0) {
            prog = (CRProgram *) crHashtableSearch(p->programHash, ids[i]);
            if (prog == p->currentVertexProgram) {
                 p->currentVertexProgram = p->defaultVertexProgram;
                 DIRTY(pb->dirty, g->neg_bitid);
                 DIRTY(pb->vpBinding, g->neg_bitid);
            }
            else if (prog == p->currentFragmentProgram) {
                 p->currentFragmentProgram = p->defaultFragmentProgram;
                 DIRTY(pb->dirty, g->neg_bitid);
                 DIRTY(pb->fpBinding, g->neg_bitid);
            }
            if (prog) {
                DeleteProgram(prog);
            }
            crHashtableDelete(p->programHash, ids[i], GL_FALSE);
        }
    }
}


void STATE_APIENTRY crStateExecuteProgramNV(GLenum target, GLuint id, const GLfloat *params)
{
    /* Hmmm, this is really hard to do if we don't actually execute
     * the program in a software simulation.
     */
    (void)params;
    (void)target;
    (void)id;
}


void STATE_APIENTRY crStateGenProgramsNV(GLsizei n, GLuint *ids)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);

    crStateGenNames(g, p->programHash, n, ids);
}

void STATE_APIENTRY crStateGenProgramsARB(GLsizei n, GLuint *ids)
{
    crStateGenProgramsNV(n, ids);
}


GLboolean STATE_APIENTRY crStateIsProgramARB(GLuint id)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    CRProgram *prog;

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glIsProgram called in Begin/End");
        return GL_FALSE;
    }

    if (id == 0) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glIsProgram(id==0)");
        return GL_FALSE;
    }

    prog = (CRProgram *) crHashtableSearch(p->programHash, id);
    if (prog)
        return GL_TRUE;
    else
        return GL_FALSE;
}


GLboolean STATE_APIENTRY crStateAreProgramsResidentNV(GLsizei n, const GLuint *ids, GLboolean *residences)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    int i;
    GLboolean retVal = GL_TRUE;

    if (n < 0) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "glAreProgramsResidentNV(n)");
        return GL_FALSE;
    }

    for (i = 0; i < n; i++) {
        CRProgram *prog;

        if (ids[i] == 0) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glAreProgramsResidentNV(id)");
            return GL_FALSE;
        }

        prog = (CRProgram *) crHashtableSearch(p->programHash, ids[i]);
        if (!prog) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glAreProgramsResidentNV(id)");
            return GL_FALSE;
        }

        if (!prog->resident) {
             retVal = GL_FALSE;
             break;
        }
    }

    if (retVal == GL_FALSE) {
        for (i = 0; i < n; i++) {
            CRProgram *prog = (CRProgram *)
                crHashtableSearch(p->programHash, ids[i]);
            residences[i] = prog->resident;
        }
    }

    return retVal;
}


void STATE_APIENTRY crStateRequestResidentProgramsNV(GLsizei n, const GLuint *ids)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    GLint i;

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glRequestResidentProgramsNV called in Begin/End");
        return;
    }

    if (n < 0) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glRequestResidentProgramsNV(n<0)");
        return;
    }

    for (i = 0; i < n ; i++) {
         CRProgram *prog = (CRProgram *) crHashtableSearch(p->programHash, ids[i]);
         if (prog)
                prog->resident = GL_TRUE;
    }
}


void STATE_APIENTRY crStateLoadProgramNV(GLenum target, GLuint id, GLsizei len,
                                                                                 const GLubyte *program)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    CRStateBits *sb = GetCurrentBits();
    CRProgramBits *pb = &(sb->program);
    CRProgram *prog;
    GLubyte *progCopy;

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glLoadProgramNV called in Begin/End");
        return;
    }

    if (id == 0) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "glLoadProgramNV(id==0)");
        return;
    }

    prog = GetProgram(p, target, id);

    if (!prog) {
        crStateError(__LINE__, __FILE__, GL_OUT_OF_MEMORY, "glLoadProgramNV");
        return;
    }
    else if (prog && prog->target != target) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glLoadProgramNV(target)");
        return;
    }

    progCopy = crAlloc(len);
    if (!progCopy) {
            crStateError(__LINE__, __FILE__, GL_OUT_OF_MEMORY, "glLoadProgramNV");
            return;
    }
    if (crStrncmp((const char *) program,"!!FP1.0", 7) != 0
     && crStrncmp((const char *) program,"!!FCP1.0", 8) != 0
     && crStrncmp((const char *) program,"!!VP1.0", 7) != 0
     && crStrncmp((const char *) program,"!!VP1.1", 7) != 0
     && crStrncmp((const char *) program,"!!VP2.0", 7) != 0
     && crStrncmp((const char *) program,"!!VSP1.0", 8) != 0) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glLoadProgramNV");
            crDebug("program = (%s)\n",program);
            return;
    }
    crMemcpy(progCopy, program, len);
    if (prog->string)
        crFree((void *) prog->string);

    prog->string = progCopy;
    prog->length = len;
    prog->isARBprogram = GL_FALSE;

    DIRTY(prog->dirtyProgram, g->neg_bitid);
    DIRTY(pb->dirty, g->neg_bitid);
}


void STATE_APIENTRY crStateProgramStringARB(GLenum target, GLenum format,
                                                                                        GLsizei len, const GLvoid *string)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    CRStateBits *sb = GetCurrentBits();
    CRProgramBits *pb = &(sb->program);
    CRProgram *prog;
    GLubyte *progCopy;

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glProgramStringARB called in Begin/End");
        return;
    }

    if (format != GL_PROGRAM_FORMAT_ASCII_ARB) {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glProgramStringARB(format)");
        return;
    }

    if (target == GL_FRAGMENT_PROGRAM_ARB
            && g->extensions.ARB_fragment_program) {
        prog = p->currentFragmentProgram;
    }
    else if (target == GL_VERTEX_PROGRAM_ARB
                     && g->extensions.ARB_vertex_program) {
        prog = p->currentVertexProgram;
    }
    else {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glProgramStringARB(target)");
        return;
    }

    CRASSERT(prog);


    progCopy = crAlloc(len);
    if (!progCopy) {
        crStateError(__LINE__, __FILE__, GL_OUT_OF_MEMORY, "glProgramStringARB");
        return;
    }
    if (crStrncmp(string,"!!ARBvp1.0", 10) !=  0 
     && crStrncmp(string,"!!ARBfp1.0", 10) != 0) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE, "glProgramStringARB");
            return;
    }
    crMemcpy(progCopy, string, len);
    if (prog->string)
        crFree((void *) prog->string);

    prog->string = progCopy;
    prog->length = len;
    prog->format = format;
    prog->isARBprogram = GL_TRUE;

    DIRTY(prog->dirtyProgram, g->neg_bitid);
    DIRTY(pb->dirty, g->neg_bitid);
}


void STATE_APIENTRY crStateGetProgramivNV(GLuint id, GLenum pname, GLint *params)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    CRProgram *prog;

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramivNV called in Begin/End");
        return;
    }

    if (id == 0) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramivNV(bad id)");
        return;
    }
        
    prog = (CRProgram *) crHashtableSearch(p->programHash, id);
    if (!prog) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramivNV(bad id)");
        return;
    }

    switch (pname) {
    case GL_PROGRAM_TARGET_NV:
        *params = prog->target;
        return;
    case GL_PROGRAM_LENGTH_NV:
        *params = prog->length;
        return;
    case GL_PROGRAM_RESIDENT_NV:
        *params = prog->resident;
        return;
    default:
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glGetProgramivNV(pname)");
        return;
    }
}


void STATE_APIENTRY crStateGetProgramStringNV(GLuint id, GLenum pname, GLubyte *program)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    CRProgram *prog;

    if (pname != GL_PROGRAM_STRING_NV) {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glGetProgramStringNV(pname)");
        return;
    }

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramStringNV called in Begin/End");
        return;
    }

    if (id == 0) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramStringNV(bad id)");
        return;
    }
        
    prog = (CRProgram *) crHashtableSearch(p->programHash, id);
    if (!prog) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramStringNV(bad id)");
        return;
    }

    crMemcpy(program, prog->string, prog->length);
}


void STATE_APIENTRY crStateGetProgramStringARB(GLenum target, GLenum pname, GLvoid *string)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    CRProgram *prog;

    if (target == GL_VERTEX_PROGRAM_ARB) {
        prog = p->currentVertexProgram;
    }
    else if (target == GL_FRAGMENT_PROGRAM_ARB) {
        prog = p->currentFragmentProgram;
    }
    else {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glGetProgramStringNV(target)");
        return;
    }

    if (pname != GL_PROGRAM_STRING_NV) {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glGetProgramStringNV(pname)");
        return;
    }

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramStringNV called in Begin/End");
        return;
    }

    crMemcpy(string, prog->string, prog->length);
}


void STATE_APIENTRY crStateProgramParameter4dNV(GLenum target, GLuint index,
                                GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    crStateProgramParameter4fNV(target, index, (GLfloat) x, (GLfloat) y,
                                                            (GLfloat) z, (GLfloat) w);
}


void STATE_APIENTRY crStateProgramParameter4dvNV(GLenum target, GLuint index,
                                 const GLdouble *params)
{
    crStateProgramParameter4fNV(target, index,
                                                            (GLfloat) params[0], (GLfloat) params[1],
                                                            (GLfloat) params[2], (GLfloat) params[3]);
}


void STATE_APIENTRY crStateProgramParameter4fNV(GLenum target, GLuint index,
                                                                 GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    CRStateBits *sb = GetCurrentBits();
    CRProgramBits *pb = &(sb->program);

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glProgramParameterNV called in Begin/End");
        return;
    }

    if (target == GL_VERTEX_PROGRAM_NV) {
        if (index < g->limits.maxVertexProgramEnvParams) {
            p->vertexParameters[index][0] = x;
            p->vertexParameters[index][1] = y;
            p->vertexParameters[index][2] = z;
            p->vertexParameters[index][3] = w;
            DIRTY(pb->dirty, g->neg_bitid);
            DIRTY(pb->vertexEnvParameter[index], g->neg_bitid);
            DIRTY(pb->vertexEnvParameters, g->neg_bitid);
        }
        else {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glProgramParameterNV(index=%d)", index);
            return;
        }
    }
    else {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glProgramParameterNV(target)");
        return;
    }
}


void STATE_APIENTRY crStateProgramParameter4fvNV(GLenum target, GLuint index,
                                 const GLfloat *params)
{
    crStateProgramParameter4fNV(target, index,
                                                            params[0], params[1], params[2], params[3]);
}


void STATE_APIENTRY crStateProgramParameters4dvNV(GLenum target, GLuint index,
                                  GLuint num, const GLdouble *params)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    CRStateBits *sb = GetCurrentBits();
    CRProgramBits *pb = &(sb->program);

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glProgramParameters4dvNV called in Begin/End");
        return;
    }

    if (target == GL_VERTEX_PROGRAM_NV) {
        if (index >= UINT32_MAX - num) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                "glProgramParameters4dvNV(index+num) integer overflow");
            return;
        }

        if (index + num < g->limits.maxVertexProgramEnvParams) {
            GLuint i;
            for (i = 0; i < num; i++) {
                p->vertexParameters[index+i][0] = (GLfloat) params[i*4+0];
                p->vertexParameters[index+i][1] = (GLfloat) params[i*4+1];
                p->vertexParameters[index+i][2] = (GLfloat) params[i*4+2];
                p->vertexParameters[index+i][3] = (GLfloat) params[i*4+3];
            }
            DIRTY(pb->dirty, g->neg_bitid);
            DIRTY(pb->vertexEnvParameters, g->neg_bitid);
        }
        else {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glProgramParameters4dvNV(index+num)");
            return;
        }
    }
    else {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glProgramParameterNV(target)");
        return;
    }
}


void STATE_APIENTRY crStateProgramParameters4fvNV(GLenum target, GLuint index,
                                  GLuint num, const GLfloat *params)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    CRStateBits *sb = GetCurrentBits();
    CRProgramBits *pb = &(sb->program);

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glProgramParameters4dvNV called in Begin/End");
        return;
    }

    if (target == GL_VERTEX_PROGRAM_NV) {
        if (index >= UINT32_MAX - num) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                "glProgramParameters4dvNV(index+num) integer overflow");
            return;
        }

        if (index + num < g->limits.maxVertexProgramEnvParams) {
            GLuint i;
            for (i = 0; i < num; i++) {
                p->vertexParameters[index+i][0] = params[i*4+0];
                p->vertexParameters[index+i][1] = params[i*4+1];
                p->vertexParameters[index+i][2] = params[i*4+2];
                p->vertexParameters[index+i][3] = params[i*4+3];
            }
            DIRTY(pb->dirty, g->neg_bitid);
            DIRTY(pb->vertexEnvParameters, g->neg_bitid);
        }
        else {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glProgramParameters4dvNV(index+num)");
            return;
        }
    }
    else {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glProgramParameterNV(target)");
        return;
    }
}


void STATE_APIENTRY crStateGetProgramParameterfvNV(GLenum target, GLuint index,
                                                                        GLenum pname, GLfloat *params)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramParameterfvNV called in Begin/End");
        return;
    }

    if (target == GL_VERTEX_PROGRAM_NV) {
        if (pname == GL_PROGRAM_PARAMETER_NV) {
            if (index < g->limits.maxVertexProgramEnvParams) {
                params[0] = p->vertexParameters[index][0];
                params[1] = p->vertexParameters[index][1];
                params[2] = p->vertexParameters[index][2];
                params[3] = p->vertexParameters[index][3];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                         "glGetProgramParameterfvNV(index)");
                return;
            }
        }
        else {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "glGetProgramParameterfvNV(pname)");
            return;
        }
    }
    else {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glGetProgramParameterfvNV(target)");
        return;
    }
}


void STATE_APIENTRY crStateGetProgramParameterdvNV(GLenum target, GLuint index,
                                   GLenum pname, GLdouble *params)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramParameterdvNV called in Begin/End");
        return;
    }

    if (target == GL_VERTEX_PROGRAM_NV) {
        if (pname == GL_PROGRAM_PARAMETER_NV) {
            if (index < g->limits.maxVertexProgramEnvParams) {
                params[0] = p->vertexParameters[index][0];
                params[1] = p->vertexParameters[index][1];
                params[2] = p->vertexParameters[index][2];
                params[3] = p->vertexParameters[index][3];
            }
            else {
                crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                         "glGetProgramParameterdvNV(index)");
                return;
            }
        }
        else {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "glGetProgramParameterdvNV(pname)");
            return;
        }
    }
    else {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glGetProgramParameterdvNV(target)");
        return;
    }
}


void STATE_APIENTRY crStateTrackMatrixNV(GLenum target, GLuint address,
                         GLenum matrix, GLenum transform)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    CRStateBits *sb = GetCurrentBits();
    CRProgramBits *pb = &(sb->program);

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetTrackMatrixivNV called in Begin/End");
        return;
    }

    if (target == GL_VERTEX_PROGRAM_NV) {
        if (address & 0x3 || address >= g->limits.maxVertexProgramEnvParams) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glTrackMatrixNV(address)");
            return;
        }

        switch (matrix) {
        case GL_NONE:
        case GL_MODELVIEW:
        case GL_PROJECTION:
        case GL_TEXTURE:
        case GL_COLOR:
        case GL_MODELVIEW_PROJECTION_NV:
        case GL_MATRIX0_NV:
        case GL_MATRIX1_NV:
        case GL_MATRIX2_NV:
        case GL_MATRIX3_NV:
        case GL_MATRIX4_NV:
        case GL_MATRIX5_NV:
        case GL_MATRIX6_NV:
        case GL_MATRIX7_NV:
        case GL_TEXTURE0_ARB:
        case GL_TEXTURE1_ARB:
        case GL_TEXTURE2_ARB:
        case GL_TEXTURE3_ARB:
        case GL_TEXTURE4_ARB:
        case GL_TEXTURE5_ARB:
        case GL_TEXTURE6_ARB:
        case GL_TEXTURE7_ARB:
            /* OK, fallthrough */
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "glTrackMatrixNV(matrix = %x)",matrix);
            return;
        }

        switch (transform) {
        case GL_IDENTITY_NV:
        case GL_INVERSE_NV:
        case GL_TRANSPOSE_NV:
        case GL_INVERSE_TRANSPOSE_NV:
            /* OK, fallthrough */
            break;
        default:
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "glTrackMatrixNV(transform = %x)",transform);
            return;
        }

        p->TrackMatrix[address / 4] = matrix;
        p->TrackMatrixTransform[address / 4] = transform;
        DIRTY(pb->trackMatrix[address/4], g->neg_bitid);
        DIRTY(pb->dirty, g->neg_bitid);
    }
    else {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glTrackMatrixNV(target = %x)",target);
    }
}


void STATE_APIENTRY crStateGetTrackMatrixivNV(GLenum target, GLuint address,
                                                             GLenum pname, GLint *params)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetTrackMatrixivNV called in Begin/End");
        return;
    }

    if (target == GL_VERTEX_PROGRAM_NV) {
        if ((address & 0x3) || address >= g->limits.maxVertexProgramEnvParams) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glGetTrackMatrixivNV(address)");
            return;
        }
        if (pname == GL_TRACK_MATRIX_NV) {
            params[0] = (GLint) p->TrackMatrix[address / 4];
        }
        else if (pname == GL_TRACK_MATRIX_TRANSFORM_NV) {
            params[0] = (GLint) p->TrackMatrixTransform[address / 4];
        }
        else {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "glGetTrackMatrixivNV(pname)");
            return;
        }
    }
    else {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glGetTrackMatrixivNV(target)");
        return;
    }
}


void STATE_APIENTRY crStateGetVertexAttribdvNV(GLuint index, GLenum pname, GLdouble *params)
{
     /* init vars to prevent compiler warnings/errors */
     GLfloat floatParams[4] = { 0.0, 0.0, 0.0, 0.0 };
     crStateGetVertexAttribfvNV(index, pname, floatParams);
     params[0] = floatParams[0];
     if (pname == GL_CURRENT_ATTRIB_NV) {
            params[1] = floatParams[1];
            params[2] = floatParams[2];
            params[3] = floatParams[3];
     }
}


void STATE_APIENTRY crStateGetVertexAttribfvNV(GLuint index, GLenum pname, GLfloat *params)
{
    CRContext *g = GetCurrentContext();

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetVertexAttribfvNV called in Begin/End");
        return;
    }

    if (index >= CR_MAX_VERTEX_ATTRIBS) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "glGetVertexAttribfvNV(index)");
        return;
    }

    switch (pname) {
    case GL_ATTRIB_ARRAY_SIZE_NV:
        params[0] = (GLfloat) g->client.array.a[index].size;
        break;
    case GL_ATTRIB_ARRAY_STRIDE_NV:
        params[0] = (GLfloat) g->client.array.a[index].stride;
        break;
    case GL_ATTRIB_ARRAY_TYPE_NV:
        params[0] = (GLfloat) g->client.array.a[index].type;
        break;
    case GL_CURRENT_ATTRIB_NV:
        crStateCurrentRecover();
        COPY_4V(params , g->current.vertexAttrib[index]);
        break;
    default:
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetVertexAttribfvNV");
        return;
    }
}


void STATE_APIENTRY crStateGetVertexAttribivNV(GLuint index, GLenum pname, GLint *params)
{
     /* init vars to prevent compiler warnings/errors */
     GLfloat floatParams[4] = { 0.0, 0.0, 0.0, 0.0 };
     crStateGetVertexAttribfvNV(index, pname, floatParams);
     params[0] = (GLint) floatParams[0];
     if (pname == GL_CURRENT_ATTRIB_NV) {
            params[1] = (GLint) floatParams[1];
            params[2] = (GLint) floatParams[2];
            params[3] = (GLint) floatParams[3];
     }
}



void STATE_APIENTRY crStateGetVertexAttribfvARB(GLuint index, GLenum pname, GLfloat *params)
{
    CRContext *g = GetCurrentContext();

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetVertexAttribfvARB called in Begin/End");
        return;
    }

    if (index >= CR_MAX_VERTEX_ATTRIBS) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "glGetVertexAttribfvARB(index)");
        return;
    }

    switch (pname) {
    case GL_VERTEX_ATTRIB_ARRAY_ENABLED_ARB:
        params[0] = (GLfloat) g->client.array.a[index].enabled;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_SIZE_ARB:
        params[0] = (GLfloat) g->client.array.a[index].size;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_STRIDE_ARB:
        params[0] = (GLfloat) g->client.array.a[index].stride;
        break;
    case GL_VERTEX_ATTRIB_ARRAY_TYPE_ARB:
        params[0] = (GLfloat) g->client.array.a[index].type;
        break;
  case GL_VERTEX_ATTRIB_ARRAY_NORMALIZED_ARB:
        params[0] = (GLfloat) g->client.array.a[index].normalized;
        break;
    case GL_CURRENT_VERTEX_ATTRIB_ARB:
        crStateCurrentRecover();
        COPY_4V(params , g->current.vertexAttrib[index]);
        break;
    default:
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM, "glGetVertexAttribfvARB");
        return;
    }
}


void STATE_APIENTRY crStateGetVertexAttribivARB(GLuint index, GLenum pname, GLint *params)
{
     /* init vars to prevent compiler warnings/errors */
     GLfloat floatParams[4] = { 0.0, 0.0, 0.0, 0.0 };
     crStateGetVertexAttribfvARB(index, pname, floatParams);
     params[0] = (GLint) floatParams[0];
     if (pname == GL_CURRENT_VERTEX_ATTRIB_ARB) {
            params[1] = (GLint) floatParams[1];
            params[2] = (GLint) floatParams[2];
            params[3] = (GLint) floatParams[3];
     }
}


void STATE_APIENTRY crStateGetVertexAttribdvARB(GLuint index, GLenum pname, GLdouble *params)
{
     /* init vars to prevent compiler warnings/errors */
     GLfloat floatParams[4] = { 0.0, 0.0, 0.0, 0.0 };
     crStateGetVertexAttribfvARB(index, pname, floatParams);
     params[0] = floatParams[0];
     if (pname == GL_CURRENT_VERTEX_ATTRIB_ARB) {
            params[1] = floatParams[1];
            params[2] = floatParams[2];
            params[3] = floatParams[3];
     }
}


/**********************************************************************/

/*
 * Added by GL_NV_fragment_program
 */

void STATE_APIENTRY crStateProgramNamedParameter4fNV(GLuint id, GLsizei len, const GLubyte *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    CRProgram *prog;
    CRStateBits *sb = GetCurrentBits();
    CRProgramBits *pb = &(sb->program);

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glProgramNamedParameterfNV called in Begin/End");
        return;
    }

    prog = (CRProgram *) crHashtableSearch(p->programHash, id);
    if (!prog) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glProgramNamedParameterNV(bad id %d)", id);
        return;
    }

    if (prog->target != GL_FRAGMENT_PROGRAM_NV) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glProgramNamedParameterNV(target)");
        return;
    }

    SetProgramSymbol(prog, (const char *)name, len, x, y, z, w);
    DIRTY(prog->dirtyNamedParams, g->neg_bitid);
    DIRTY(pb->dirty, g->neg_bitid);
}


void STATE_APIENTRY crStateProgramNamedParameter4dNV(GLuint id, GLsizei len, const GLubyte *name, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    crStateProgramNamedParameter4fNV(id, len, name, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}


void STATE_APIENTRY crStateProgramNamedParameter4fvNV(GLuint id, GLsizei len, const GLubyte *name, const GLfloat v[])
{
    crStateProgramNamedParameter4fNV(id, len, name, v[0], v[1], v[2], v[3]);
}


void STATE_APIENTRY crStateProgramNamedParameter4dvNV(GLuint id, GLsizei len, const GLubyte *name, const GLdouble v[])
{
    crStateProgramNamedParameter4fNV(id, len, name, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}


void STATE_APIENTRY crStateGetProgramNamedParameterfvNV(GLuint id, GLsizei len, const GLubyte *name, GLfloat *params)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    const CRProgram *prog;
    const GLfloat *value;

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramNamedParameterfNV called in Begin/End");
        return;
    }

    prog = (const CRProgram *) crHashtableSearch(p->programHash, id);
    if (!prog) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramNamedParameterNV(bad id)");
        return;
    }

    if (prog->target != GL_FRAGMENT_PROGRAM_NV) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramNamedParameterNV(target)");
        return;
    }

    value = GetProgramSymbol(prog, (const char *)name, len);
    if (!value) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "glGetProgramNamedParameterNV(name)");
        return;
    }

    params[0] = value[0];
    params[1] = value[1];
    params[2] = value[2];
    params[3] = value[3];
}


void STATE_APIENTRY crStateGetProgramNamedParameterdvNV(GLuint id, GLsizei len, const GLubyte *name, GLdouble *params)
{
    GLfloat floatParams[4];
    crStateGetProgramNamedParameterfvNV(id, len, name, floatParams);
    params[0] = floatParams[0];
    params[1] = floatParams[1];
    params[2] = floatParams[2];
    params[3] = floatParams[3];
}


void STATE_APIENTRY crStateProgramLocalParameter4dARB(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    crStateProgramLocalParameter4fARB(target, index, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}


void STATE_APIENTRY crStateProgramLocalParameter4dvARB(GLenum target, GLuint index, const GLdouble *params)
{
    crStateProgramLocalParameter4fARB(target, index, (GLfloat) params[0], (GLfloat) params[1],
                                                                        (GLfloat) params[2], (GLfloat) params[3]);
}


void STATE_APIENTRY crStateProgramLocalParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    CRProgram *prog;
    CRStateBits *sb = GetCurrentBits();
    CRProgramBits *pb = &(sb->program);

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glProgramLocalParameterARB called in Begin/End");
        return;
    }

    if (target == GL_FRAGMENT_PROGRAM_ARB || target == GL_FRAGMENT_PROGRAM_NV) {
        if (index >= CR_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMS) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glProgramLocalParameterARB(index)");
            return;
        }
        prog = p->currentFragmentProgram;
    }
    else if (target == GL_VERTEX_PROGRAM_ARB) {
        if (index >= CR_MAX_VERTEX_PROGRAM_LOCAL_PARAMS) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glProgramLocalParameterARB(index)");
            return;
        }
        prog = p->currentVertexProgram;
    }
    else {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glProgramLocalParameterARB(target)");
        return;
    }

    CRASSERT(prog);

    prog->parameters[index][0] = x;
    prog->parameters[index][1] = y;
    prog->parameters[index][2] = z;
    prog->parameters[index][3] = w;
    DIRTY(prog->dirtyParam[index], g->neg_bitid);
    DIRTY(prog->dirtyParams, g->neg_bitid);
    DIRTY(pb->dirty, g->neg_bitid);
}


void STATE_APIENTRY crStateProgramLocalParameter4fvARB(GLenum target, GLuint index, const GLfloat *params)
{
    crStateProgramLocalParameter4fARB(target, index, params[0], params[1], params[2], params[3]);
}


void STATE_APIENTRY crStateGetProgramLocalParameterfvARB(GLenum target, GLuint index, GLfloat *params)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    const CRProgram *prog = NULL;

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramLocalParameterARB called in Begin/End");
        return;
    }

    if (target == GL_FRAGMENT_PROGRAM_ARB || target == GL_FRAGMENT_PROGRAM_NV) {
        prog = p->currentFragmentProgram;
        if (index >= g->limits.maxFragmentProgramLocalParams) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glGetProgramLocalParameterARB(index)");
            return;
        }
    }
    else if (   target == GL_VERTEX_PROGRAM_ARB
#if GL_VERTEX_PROGRAM_ARB != GL_VERTEX_PROGRAM_NV
             || target == GL_VERTEX_PROGRAM_NV
#endif
            ) {
        prog = p->currentVertexProgram;
        if (index >= g->limits.maxVertexProgramLocalParams) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glGetProgramLocalParameterARB(index)");
            return;
        }
    }
    else {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glGetProgramLocalParameterARB(target)");
        return;
    }
    if (!prog) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramLocalParameterARB(no program)");
        return;
    }

    if (!prog) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramLocalParameterARB(no program)");
        return;
    }

    CRASSERT(prog);
    CRASSERT(index < CR_MAX_PROGRAM_LOCAL_PARAMS);
    params[0] = prog->parameters[index][0];
    params[1] = prog->parameters[index][1];
    params[2] = prog->parameters[index][2];
    params[3] = prog->parameters[index][3];
}


void STATE_APIENTRY crStateGetProgramLocalParameterdvARB(GLenum target, GLuint index, GLdouble *params)
{
    GLfloat floatParams[4];
    crStateGetProgramLocalParameterfvARB(target, index, floatParams);
    params[0] = floatParams[0];
    params[1] = floatParams[1];
    params[2] = floatParams[2];
    params[3] = floatParams[3];
}



void STATE_APIENTRY crStateGetProgramivARB(GLenum target, GLenum pname, GLint *params)
{
    CRProgram *prog;
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramivARB called in Begin/End");
        return;
    }

    if (target == GL_VERTEX_PROGRAM_ARB) {
        prog = p->currentVertexProgram;
    }
    else if (target == GL_FRAGMENT_PROGRAM_ARB) {
        prog = p->currentFragmentProgram;
    }
    else {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glGetProgramivARB(target)");
        return;
    }

    CRASSERT(prog);

    switch (pname) {
    case GL_PROGRAM_LENGTH_ARB:
        *params = prog->length;
        break;
    case GL_PROGRAM_FORMAT_ARB:
        *params = prog->format;
        break;
    case GL_PROGRAM_BINDING_ARB:
        *params = prog->id;
        break;
    case GL_PROGRAM_INSTRUCTIONS_ARB:
        *params = prog->numInstructions;
        break;
    case GL_MAX_PROGRAM_INSTRUCTIONS_ARB:
        if (target == GL_VERTEX_PROGRAM_ARB)
            *params = g->limits.maxVertexProgramInstructions;
        else
            *params = g->limits.maxFragmentProgramInstructions;
        break;
    case GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB:
        *params = prog->numInstructions;
        break;
    case GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB:
        if (target == GL_VERTEX_PROGRAM_ARB)
            *params = g->limits.maxVertexProgramInstructions;
        else
            *params = g->limits.maxFragmentProgramInstructions;
        break;
    case GL_PROGRAM_TEMPORARIES_ARB:
        *params = prog->numTemporaries;
        break;
    case GL_MAX_PROGRAM_TEMPORARIES_ARB:
        if (target == GL_VERTEX_PROGRAM_ARB)
            *params = g->limits.maxVertexProgramTemps;
        else
            *params = g->limits.maxFragmentProgramTemps;
        break;
    case GL_PROGRAM_NATIVE_TEMPORARIES_ARB:
        /* XXX same as GL_PROGRAM_TEMPORARIES_ARB? */
        *params = prog->numTemporaries;
        break;
    case GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB:
        /* XXX same as GL_MAX_PROGRAM_TEMPORARIES_ARB? */
        if (target == GL_VERTEX_PROGRAM_ARB)
            *params = g->limits.maxVertexProgramTemps;
        else
            *params = g->limits.maxFragmentProgramTemps;
        break;
    case GL_PROGRAM_PARAMETERS_ARB:
        *params = prog->numParameters;
        break;
    case GL_MAX_PROGRAM_PARAMETERS_ARB:
        if (target == GL_VERTEX_PROGRAM_ARB)
            *params = g->limits.maxVertexProgramLocalParams;
        else
            *params = g->limits.maxFragmentProgramLocalParams;
        break;
    case GL_PROGRAM_NATIVE_PARAMETERS_ARB:
        /* XXX same as GL_MAX_PROGRAM_PARAMETERS_ARB? */
        *params = prog->numParameters;
        break;
    case GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB:
        /* XXX same as GL_MAX_PROGRAM_PARAMETERS_ARB? */
        if (target == GL_VERTEX_PROGRAM_ARB)
            *params = g->limits.maxVertexProgramLocalParams;
        else
            *params = g->limits.maxFragmentProgramLocalParams;
        break;
    case GL_PROGRAM_ATTRIBS_ARB:
        *params = prog->numAttributes;
        break;
    case GL_MAX_PROGRAM_ATTRIBS_ARB:
        if (target == GL_VERTEX_PROGRAM_ARB)
            *params = g->limits.maxVertexProgramAttribs;
        else
            *params = g->limits.maxFragmentProgramAttribs;
        break;
    case GL_PROGRAM_NATIVE_ATTRIBS_ARB:
        /* XXX same as GL_PROGRAM_ATTRIBS_ARB? */
        *params = prog->numAttributes;
        break;
    case GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB:
        /* XXX same as GL_MAX_PROGRAM_ATTRIBS_ARB? */
        if (target == GL_VERTEX_PROGRAM_ARB)
            *params = g->limits.maxVertexProgramAttribs;
        else
            *params = g->limits.maxFragmentProgramAttribs;
        break;
    case GL_PROGRAM_ADDRESS_REGISTERS_ARB:
        *params = prog->numAddressRegs;
        break;
    case GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB:
        if (target == GL_VERTEX_PROGRAM_ARB)
            *params = g->limits.maxVertexProgramAddressRegs;
        else
            *params = g->limits.maxFragmentProgramAddressRegs;
        break;
    case GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB:
        /* XXX same as GL_PROGRAM_ADDRESS_REGISTERS_ARB? */
        *params = prog->numAddressRegs;
        break;
    case GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB:
        /* XXX same as GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB? */
        if (target == GL_VERTEX_PROGRAM_ARB)
            *params = g->limits.maxVertexProgramAddressRegs;
        else
            *params = g->limits.maxFragmentProgramAddressRegs;
        break;
    case GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB:
        if (target == GL_VERTEX_PROGRAM_ARB)
            *params = g->limits.maxVertexProgramLocalParams;
        else
            *params = g->limits.maxFragmentProgramLocalParams;
        break;
    case GL_MAX_PROGRAM_ENV_PARAMETERS_ARB:
        if (target == GL_VERTEX_PROGRAM_ARB)
            *params = g->limits.maxVertexProgramEnvParams;
        else
            *params = g->limits.maxFragmentProgramEnvParams;
        break;
    case GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB:
        /* XXX ok? */
        *params = GL_TRUE;
        break;

    /*
     * These are for fragment programs only
     */
    case GL_PROGRAM_ALU_INSTRUCTIONS_ARB:
        if (target != GL_FRAGMENT_PROGRAM_ARB || !g->extensions.ARB_fragment_program) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "crStateGetProgramivARB(target or pname)");
            return;
        }
        *params = prog->numAluInstructions;
        break;
    case GL_PROGRAM_TEX_INSTRUCTIONS_ARB:
        if (target != GL_FRAGMENT_PROGRAM_ARB || !g->extensions.ARB_fragment_program) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "crStateGetProgramivARB(target or pname)");
            return;
        }
        *params = prog->numTexInstructions;
        break;
    case GL_PROGRAM_TEX_INDIRECTIONS_ARB:
        if (target != GL_FRAGMENT_PROGRAM_ARB || !g->extensions.ARB_fragment_program) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "crStateGetProgramivARB(target or pname)");
            return;
        }
        *params = prog->numTexIndirections;
        break;
    case GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB:
        /* XXX same as GL_PROGRAM_ALU_INSTRUCTIONS_ARB? */
        if (target != GL_FRAGMENT_PROGRAM_ARB || !g->extensions.ARB_fragment_program) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "crStateGetProgramivARB(target or pname)");
            return;
        }
        *params = prog->numAluInstructions;
        break;
    case GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB:
        /* XXX same as GL_PROGRAM_ALU_INSTRUCTIONS_ARB? */
        if (target != GL_FRAGMENT_PROGRAM_ARB || !g->extensions.ARB_fragment_program) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "crStateGetProgramivARB(target or pname)");
            return;
        }
        *params = prog->numTexInstructions;
        break;
    case GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB:
        if (target != GL_FRAGMENT_PROGRAM_ARB || !g->extensions.ARB_fragment_program) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "crStateGetProgramivARB(target or pname)");
            return;
        }
        *params = prog->numTexIndirections;
        break;
    case GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB:
    case GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB:
        if (target != GL_FRAGMENT_PROGRAM_ARB || !g->extensions.ARB_fragment_program) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "crStateGetProgramivARB(target or pname)");
            return;
        }
        *params = g->limits.maxFragmentProgramAluInstructions;
        break;
    case GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB:
    case GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB:
        if (target != GL_FRAGMENT_PROGRAM_ARB || !g->extensions.ARB_fragment_program) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "crStateGetProgramivARB(target or pname)");
            return;
        }
        *params = g->limits.maxFragmentProgramTexInstructions;
        break;
    case GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB:
    case GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB:
        if (target != GL_FRAGMENT_PROGRAM_ARB || !g->extensions.ARB_fragment_program) {
            crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                     "crStateGetProgramivARB(target or pname)");
            return;
        }
        *params = g->limits.maxFragmentProgramTexIndirections;
        break;
    default:
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "crStateGetProgramivARB(pname)");
        return;
    }
}


/* XXX maybe move these two functions into state_client.c? */
void STATE_APIENTRY crStateDisableVertexAttribArrayARB(GLuint index)
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    CRStateBits *sb = GetCurrentBits();
    CRClientBits *cb = &(sb->client);

    if (index >= g->limits.maxVertexProgramAttribs) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "glDisableVertexAttribArrayARB(index)");
        return;
    }
    c->array.a[index].enabled = GL_FALSE;
    DIRTY(cb->dirty, g->neg_bitid);
    DIRTY(cb->enableClientState, g->neg_bitid);
}


void STATE_APIENTRY crStateEnableVertexAttribArrayARB(GLuint index)
{
    CRContext *g = GetCurrentContext();
    CRClientState *c = &(g->client);
    CRStateBits *sb = GetCurrentBits();
    CRClientBits *cb = &(sb->client);

    if (index >= g->limits.maxVertexProgramAttribs) {
        crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                 "glEnableVertexAttribArrayARB(index)");
        return;
    }
    c->array.a[index].enabled = GL_TRUE;
    DIRTY(cb->dirty, g->neg_bitid);
    DIRTY(cb->enableClientState, g->neg_bitid);
}


void STATE_APIENTRY crStateGetProgramEnvParameterdvARB(GLenum target, GLuint index, GLdouble *params)
{
     GLfloat fparams[4];
     crStateGetProgramEnvParameterfvARB(target, index, fparams);
     params[0] = fparams[0];
     params[1] = fparams[1];
     params[2] = fparams[2];
     params[3] = fparams[3];
}

void STATE_APIENTRY crStateGetProgramEnvParameterfvARB(GLenum target, GLuint index, GLfloat *params)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glGetProgramEnvParameterARB called in Begin/End");
        return;
    }

    if (target == GL_FRAGMENT_PROGRAM_ARB || target == GL_FRAGMENT_PROGRAM_NV) {
        if (index >= g->limits.maxFragmentProgramEnvParams) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glGetProgramEnvParameterARB(index)");
            return;
        }
        params[0] = p->fragmentParameters[index][0];
        params[1] = p->fragmentParameters[index][1];
        params[2] = p->fragmentParameters[index][2];
        params[3] = p->fragmentParameters[index][3];
    }
    else if (   target == GL_VERTEX_PROGRAM_ARB
#if GL_VERTEX_PROGRAM_ARB != GL_VERTEX_PROGRAM_NV
             || target == GL_VERTEX_PROGRAM_NV
#endif
             ) {
        if (index >= g->limits.maxVertexProgramEnvParams) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glGetProgramEnvParameterARB(index)");
            return;
        }
        params[0] = p->vertexParameters[index][0];
        params[1] = p->vertexParameters[index][1];
        params[2] = p->vertexParameters[index][2];
        params[3] = p->vertexParameters[index][3];
    }
    else {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glGetProgramEnvParameterARB(target)");
        return;
    }
}


void STATE_APIENTRY crStateProgramEnvParameter4dARB(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
     crStateProgramEnvParameter4fARB(target, index, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

void STATE_APIENTRY crStateProgramEnvParameter4dvARB(GLenum target, GLuint index, const GLdouble *params)
{
     crStateProgramEnvParameter4fARB(target, index, (GLfloat) params[0], (GLfloat) params[1], (GLfloat) params[2], (GLfloat) params[3]);
}

void STATE_APIENTRY crStateProgramEnvParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    CRContext *g = GetCurrentContext();
    CRProgramState *p = &(g->program);
    CRStateBits *sb = GetCurrentBits();
    CRProgramBits *pb = &(sb->program);

    if (g->current.inBeginEnd) {
        crStateError(__LINE__, __FILE__, GL_INVALID_OPERATION,
                                 "glProgramEnvParameterARB called in Begin/End");
        return;
    }

    if (target == GL_FRAGMENT_PROGRAM_ARB || target == GL_FRAGMENT_PROGRAM_NV) {
        if (index >= g->limits.maxFragmentProgramEnvParams) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glProgramEnvParameterARB(index)");
            return;
        }
        p->fragmentParameters[index][0] = x;
        p->fragmentParameters[index][1] = y;
        p->fragmentParameters[index][2] = z;
        p->fragmentParameters[index][3] = w;
        DIRTY(pb->fragmentEnvParameter[index], g->neg_bitid);
        DIRTY(pb->fragmentEnvParameters, g->neg_bitid);
    }
    else if (   target == GL_VERTEX_PROGRAM_ARB
#if GL_VERTEX_PROGRAM_ARB != GL_VERTEX_PROGRAM_NV
             || target == GL_VERTEX_PROGRAM_NV
#endif
             ) {
        if (index >= g->limits.maxVertexProgramEnvParams) {
            crStateError(__LINE__, __FILE__, GL_INVALID_VALUE,
                                     "glProgramEnvParameterARB(index)");
            return;
        }
        p->vertexParameters[index][0] = x;
        p->vertexParameters[index][1] = y;
        p->vertexParameters[index][2] = z;
        p->vertexParameters[index][3] = w;
        DIRTY(pb->vertexEnvParameter[index], g->neg_bitid);
        DIRTY(pb->vertexEnvParameters, g->neg_bitid);
    }
    else {
        crStateError(__LINE__, __FILE__, GL_INVALID_ENUM,
                                 "glProgramEnvParameterARB(target)");
        return;
    }

    DIRTY(pb->dirty, g->neg_bitid);
}

void STATE_APIENTRY crStateProgramEnvParameter4fvARB(GLenum target, GLuint index, const GLfloat *params)
{
     crStateProgramEnvParameter4fARB(target, index, params[0], params[1], params[2], params[3]);
}


/**********************************************************************/


void crStateProgramInit( CRContext *ctx )
{
    CRProgramState *p = &(ctx->program);
    CRStateBits *sb = GetCurrentBits();
    CRProgramBits *pb = &(sb->program);
    GLuint i;

    CRASSERT(CR_MAX_PROGRAM_ENV_PARAMS >= CR_MAX_VERTEX_PROGRAM_ENV_PARAMS);
    CRASSERT(CR_MAX_PROGRAM_ENV_PARAMS >= CR_MAX_FRAGMENT_PROGRAM_ENV_PARAMS);

    CRASSERT(CR_MAX_PROGRAM_LOCAL_PARAMS >= CR_MAX_VERTEX_PROGRAM_LOCAL_PARAMS);
    CRASSERT(CR_MAX_PROGRAM_LOCAL_PARAMS >= CR_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMS);

    p->programHash = crAllocHashtable();

    /* ARB_vertex/fragment_program define default program objects */
    p->defaultVertexProgram = GetProgram(p, GL_VERTEX_PROGRAM_ARB, 0);
    p->defaultFragmentProgram = GetProgram(p, GL_FRAGMENT_PROGRAM_ARB, 0);

    p->currentVertexProgram = p->defaultVertexProgram;
    p->currentFragmentProgram = p->defaultFragmentProgram;
    p->errorPos = -1;
    p->errorString = NULL;

    for (i = 0; i < ctx->limits.maxVertexProgramEnvParams / 4; i++) {
        p->TrackMatrix[i] = GL_NONE;
        p->TrackMatrixTransform[i] = GL_IDENTITY_NV;
    }
    for (i = 0; i < ctx->limits.maxVertexProgramEnvParams; i++) {
        p->vertexParameters[i][0] = 0.0;
        p->vertexParameters[i][1] = 0.0;
        p->vertexParameters[i][2] = 0.0;
        p->vertexParameters[i][3] = 0.0;
    }
    for (i = 0; i < CR_MAX_FRAGMENT_PROGRAM_ENV_PARAMS; i++) {
        p->fragmentParameters[i][0] = 0.0;
        p->fragmentParameters[i][1] = 0.0;
        p->fragmentParameters[i][2] = 0.0;
        p->fragmentParameters[i][3] = 0.0;
    }

    p->vpEnabled = GL_FALSE;
    p->fpEnabled = GL_FALSE;
    p->fpEnabledARB = GL_FALSE;
    p->vpPointSize = GL_FALSE;
    p->vpTwoSide = GL_FALSE;
    RESET(pb->dirty, ctx->bitid);
}


static void DeleteProgramCallback( void *data )
{
    CRProgram *prog = (CRProgram *) data;
    DeleteProgram(prog);
}

void crStateProgramDestroy(CRContext *ctx)
{
    CRProgramState *p = &(ctx->program);
    crFreeHashtable(p->programHash, DeleteProgramCallback);
    DeleteProgram(p->defaultVertexProgram);
    DeleteProgram(p->defaultFragmentProgram);
}


/* XXX it would be nice to autogenerate this, but we can't for now.
 */
void
crStateProgramDiff(CRProgramBits *b, CRbitvalue *bitID,
                                     CRContext *fromCtx, CRContext *toCtx)
{
    CRProgramState *from = &(fromCtx->program);
    CRProgramState *to = &(toCtx->program);
    unsigned int i, j;
    CRbitvalue nbitID[CR_MAX_BITARRAY];

    CRASSERT(from->currentVertexProgram);
    CRASSERT(to->currentVertexProgram);
    CRASSERT(from->currentFragmentProgram);
    CRASSERT(to->currentFragmentProgram);

    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];

    /* vertex program enable */
    if (CHECKDIRTY(b->vpEnable, bitID)) {
        glAble able[2];
        CRProgram *toProg = to->currentVertexProgram;

        able[0] = diff_api.Disable;
        able[1] = diff_api.Enable;
        if (from->vpEnabled != to->vpEnabled) {
            if (toProg->isARBprogram)
                able[to->vpEnabled](GL_VERTEX_PROGRAM_ARB);
            else
                able[to->vpEnabled](GL_VERTEX_PROGRAM_NV);
            from->vpEnabled = to->vpEnabled;
        }
        if (from->vpTwoSide != to->vpTwoSide) {
            able[to->vpTwoSide](GL_VERTEX_PROGRAM_TWO_SIDE_NV);
            from->vpTwoSide = to->vpTwoSide;
        }
        if (from->vpPointSize != to->vpPointSize) {
            able[to->vpPointSize](GL_VERTEX_PROGRAM_POINT_SIZE_NV);
            from->vpPointSize = to->vpPointSize;
        }
        CLEARDIRTY(b->vpEnable, nbitID);
    }

    /* fragment program enable */
    if (CHECKDIRTY(b->fpEnable, bitID)) {
        glAble able[2];
        able[0] = diff_api.Disable;
        able[1] = diff_api.Enable;
        if (from->fpEnabled != to->fpEnabled) {
            able[to->fpEnabled](GL_FRAGMENT_PROGRAM_NV);
            from->fpEnabled = to->fpEnabled;
        }
        if (from->fpEnabledARB != to->fpEnabledARB) {
            able[to->fpEnabledARB](GL_FRAGMENT_PROGRAM_ARB);
            from->fpEnabledARB = to->fpEnabledARB;
        }
        CLEARDIRTY(b->fpEnable, nbitID);
    }

    /* program/track matrices */
    if (to->vpEnabled) {
        for (i = 0; i < toCtx->limits.maxVertexProgramEnvParams / 4; i++) {
            if (CHECKDIRTY(b->trackMatrix[i], bitID))   {
                if (from->TrackMatrix[i] != to->TrackMatrix[i] ||
                        from->TrackMatrixTransform[i] != to->TrackMatrixTransform[i])   {
                    diff_api.TrackMatrixNV(GL_VERTEX_PROGRAM_NV, i * 4,
                                                                 to->TrackMatrix[i],
                                                                 to->TrackMatrixTransform[i]);
                    from->TrackMatrix[i] = to->TrackMatrix[i];
                    from->TrackMatrixTransform[i] = to->TrackMatrixTransform[i];
                }
                CLEARDIRTY(b->trackMatrix[i], nbitID);
            }
        }
    }

    if (to->vpEnabled) {
        /* vertex program binding */
        CRProgram *fromProg = from->currentVertexProgram;
        CRProgram *toProg = to->currentVertexProgram;

        if (CHECKDIRTY(b->vpBinding, bitID)) {
            if (fromProg->id != toProg->id) {
                if (toProg->isARBprogram)
                    diff_api.BindProgramARB(GL_VERTEX_PROGRAM_ARB, toProg->id);
                else
                    diff_api.BindProgramNV(GL_VERTEX_PROGRAM_NV, toProg->id);
                from->currentVertexProgram = toProg;
            }
            CLEARDIRTY(b->vpBinding, nbitID);
        }

        if (toProg) {
            /* vertex program text */
            if (CHECKDIRTY(toProg->dirtyProgram, bitID)) {
                if (toProg->isARBprogram) {
                    diff_api.ProgramStringARB( GL_VERTEX_PROGRAM_ARB, toProg->format, toProg->length, toProg->string );
                }
                else {
                    diff_api.LoadProgramNV( GL_VERTEX_PROGRAM_NV, toProg->id, toProg->length, toProg->string );
                }
                CLEARDIRTY(toProg->dirtyProgram, nbitID);
            }

            /* vertex program global/env parameters */
            if (CHECKDIRTY(b->vertexEnvParameters, bitID)) {
                for (i = 0; i < toCtx->limits.maxVertexProgramEnvParams; i++) {
                    if (CHECKDIRTY(b->vertexEnvParameter[i], bitID)) {
                        if (toProg->isARBprogram)
                            diff_api.ProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, i,
                                                                                                 to->vertexParameters[i]);
                        else
                            diff_api.ProgramParameter4fvNV(GL_VERTEX_PROGRAM_NV, i,
                                                                                         to->vertexParameters[i]);
                        if (fromProg) {
                            COPY_4V(from->vertexParameters[i],
                                            to->vertexParameters[i]);
                        }
                        CLEARDIRTY(b->vertexEnvParameter[i], nbitID);
                    }
                }
                CLEARDIRTY(b->vertexEnvParameters, nbitID);
            }

            /* vertex program local parameters */
            if (CHECKDIRTY(toProg->dirtyParams, bitID)) {
                for (i = 0; i < toCtx->limits.maxVertexProgramLocalParams; i++) {
                    if (CHECKDIRTY(toProg->dirtyParam[i], bitID)) {
                        if (toProg->isARBprogram)
                            diff_api.ProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, i, toProg->parameters[i]);
                        else
                            diff_api.ProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_NV, i, toProg->parameters[i]);
                        CLEARDIRTY(toProg->dirtyParam[i], nbitID);
                    }
                }
                CLEARDIRTY(toProg->dirtyParams, nbitID);
            }
        }
    }

    /*
     * Separate paths for NV vs ARB fragment program
     */
    if (to->fpEnabled) {
        /* NV fragment program binding */
        CRProgram *fromProg = from->currentFragmentProgram;
        CRProgram *toProg = to->currentFragmentProgram;
        if (CHECKDIRTY(b->fpBinding, bitID)) {
            if (fromProg->id != toProg->id) {
                diff_api.BindProgramNV(GL_FRAGMENT_PROGRAM_NV, toProg->id);
                from->currentFragmentProgram = toProg;
            }
            CLEARDIRTY(b->fpBinding, nbitID);
        }

        if (toProg) {
            /* fragment program text */
            if (CHECKDIRTY(toProg->dirtyProgram, bitID)) {
                diff_api.LoadProgramNV( GL_FRAGMENT_PROGRAM_NV, toProg->id,
                                                                toProg->length, toProg->string );
                CLEARDIRTY(toProg->dirtyProgram, nbitID);
            }

            /* fragment program global/env parameters */
            if (CHECKDIRTY(b->fragmentEnvParameters, bitID)) {
                for (i = 0; i < toCtx->limits.maxFragmentProgramEnvParams; i++) {
                    if (CHECKDIRTY(b->fragmentEnvParameter[i], bitID)) {
                        diff_api.ProgramParameter4fvNV(GL_FRAGMENT_PROGRAM_NV, i,
                                                                                     to->fragmentParameters[i]);
                        if (fromProg) {
                            COPY_4V(from->fragmentParameters[i],
                                            to->fragmentParameters[i]);
                        }
                        CLEARDIRTY(b->fragmentEnvParameter[i], nbitID);
                    }
                }
                CLEARDIRTY(b->fragmentEnvParameters, nbitID);
            }

            /* named local parameters */
            if (CHECKDIRTY(toProg->dirtyNamedParams, bitID)) {
                 CRProgramSymbol *symbol;
                for (symbol = toProg->symbolTable; symbol; symbol = symbol->next) {
                    if (CHECKDIRTY(symbol->dirty, bitID)) {
                        GLint len = crStrlen(symbol->name);
                        diff_api.ProgramNamedParameter4fvNV(toProg->id, len,
                                                            (const GLubyte *) symbol->name,
                                                            symbol->value);
                        if (fromProg) {
                             SetProgramSymbol(fromProg, symbol->name, len,
                                              symbol->value[0], symbol->value[1],
                                              symbol->value[2], symbol->value[3]);
                        }
                        CLEARDIRTY(symbol->dirty, nbitID);
                    }
                }
                CLEARDIRTY(toProg->dirtyNamedParams, nbitID);
            }

            /* numbered local parameters */
            if (CHECKDIRTY(toProg->dirtyParams, bitID)) {
                for (i = 0; i < CR_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMS; i++) {
                    if (CHECKDIRTY(toProg->dirtyParam[i], bitID)) {
                        diff_api.ProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_NV, i,
                                                                                                 toProg->parameters[i]);
                        if (fromProg) {
                            COPY_4V(fromProg->parameters[i], toProg->parameters[i]);
                        }
                        CLEARDIRTY(toProg->dirtyParam[i], nbitID);
                    }
                }
                CLEARDIRTY(toProg->dirtyParams, nbitID);
            }
        }
    }
    else if (to->fpEnabledARB) {
        /* ARB fragment program binding */
        CRProgram *fromProg = from->currentFragmentProgram;
        CRProgram *toProg = to->currentFragmentProgram;
        if (CHECKDIRTY(b->fpBinding, bitID)) {
            if (fromProg->id != toProg->id) {
                diff_api.BindProgramARB(GL_FRAGMENT_PROGRAM_ARB, toProg->id);
                from->currentFragmentProgram = toProg;
            }
            CLEARDIRTY(b->fpBinding, nbitID);
        }

        if (toProg) {
            /* fragment program text */
            if (CHECKDIRTY(toProg->dirtyProgram, bitID)) {
                diff_api.ProgramStringARB( GL_FRAGMENT_PROGRAM_ARB, toProg->format,
                                                                         toProg->length, toProg->string );
                CLEARDIRTY(toProg->dirtyProgram, nbitID);
            }

            /* fragment program global/env parameters */
            if (CHECKDIRTY(b->fragmentEnvParameters, bitID)) {
                for (i = 0; i < toCtx->limits.maxFragmentProgramEnvParams; i++) {
                    if (CHECKDIRTY(b->fragmentEnvParameter[i], bitID)) {
                        diff_api.ProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, i,
                                                                                             to->fragmentParameters[i]);
                        if (fromProg) {
                            COPY_4V(from->fragmentParameters[i],
                                            to->fragmentParameters[i]);
                        }
                        CLEARDIRTY(b->fragmentEnvParameter[i], nbitID);
                    }
                }
                CLEARDIRTY(b->fragmentEnvParameters, nbitID);
            }

            /* numbered local parameters */
            if (CHECKDIRTY(toProg->dirtyParams, bitID)) {
                for (i = 0; i < CR_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMS; i++) {
                    if (CHECKDIRTY(toProg->dirtyParam[i], bitID)) {
                        diff_api.ProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, i,
                                                                                                 toProg->parameters[i]);
                        if (fromProg) {
                            COPY_4V(fromProg->parameters[i], toProg->parameters[i]);
                        }
                        CLEARDIRTY(toProg->dirtyParam[i], nbitID);
                    }
                }
                CLEARDIRTY(toProg->dirtyParams, nbitID);
            }
        }
    }

    CLEARDIRTY(b->dirty, nbitID);
}


void
crStateProgramSwitch(CRProgramBits *b, CRbitvalue *bitID,
                                         CRContext *fromCtx, CRContext *toCtx)
{
    CRProgramState *from = &(fromCtx->program);
    CRProgramState *to = &(toCtx->program);
    unsigned int i, j;
    CRbitvalue nbitID[CR_MAX_BITARRAY];
    GLenum whichVert = fromCtx->extensions.ARB_vertex_program && toCtx->extensions.ARB_vertex_program  ? GL_VERTEX_PROGRAM_ARB : GL_VERTEX_PROGRAM_NV;


    for (j=0;j<CR_MAX_BITARRAY;j++)
        nbitID[j] = ~bitID[j];

    /* vertex program enable */
    if (CHECKDIRTY(b->vpEnable, bitID)) {
        glAble able[2];
        able[0] = diff_api.Disable;
        able[1] = diff_api.Enable;
        if (from->vpEnabled != to->vpEnabled) {
            able[to->vpEnabled](whichVert);
        }
        if (from->vpTwoSide != to->vpTwoSide) {
            able[to->vpTwoSide](GL_VERTEX_PROGRAM_TWO_SIDE_NV);
        }
        if (from->vpPointSize != to->vpPointSize) {
            able[to->vpPointSize](GL_VERTEX_PROGRAM_POINT_SIZE_NV);
        }
        DIRTY(b->vpEnable, nbitID);
    }

    /* fragment program enable */
    if (CHECKDIRTY(b->fpEnable, bitID)) {
        glAble able[2];
        able[0] = diff_api.Disable;
        able[1] = diff_api.Enable;
        if (from->fpEnabled != to->fpEnabled) {
            able[to->fpEnabled](GL_FRAGMENT_PROGRAM_NV);
        }
        if (from->fpEnabledARB != to->fpEnabledARB) {
            able[to->fpEnabledARB](GL_FRAGMENT_PROGRAM_ARB);
        }
        DIRTY(b->fpEnable, nbitID);
    }

    /* program/track matrices */
    if (to->vpEnabled) {
        for (i = 0; i < toCtx->limits.maxVertexProgramEnvParams / 4; i++) {
            if (CHECKDIRTY(b->trackMatrix[i], bitID))   {
                if (from->TrackMatrix[i] != to->TrackMatrix[i]) {
                    diff_api.TrackMatrixNV(GL_VERTEX_PROGRAM_NV, i * 4,
                                                                 to->TrackMatrix[i],
                                                                 to->TrackMatrixTransform[i]);
                }
                DIRTY(b->trackMatrix[i], nbitID);
            }
        }
    }

    if (to->vpEnabled) {
        /* vertex program binding */
        CRProgram *fromProg = from->currentVertexProgram;
        CRProgram *toProg = to->currentVertexProgram;
        if (CHECKDIRTY(b->vpBinding, bitID)) {
            if (fromProg->id != toProg->id) {
                if (toProg->isARBprogram)
                    diff_api.BindProgramARB(GL_VERTEX_PROGRAM_ARB, toProg->id);
                else
                    diff_api.BindProgramNV(GL_VERTEX_PROGRAM_NV, toProg->id);
            }
            DIRTY(b->vpBinding, nbitID);
        }

        if (toProg) {
            /* vertex program text */
            if (CHECKDIRTY(toProg->dirtyProgram, bitID)) {
                if (toProg->isARBprogram)
                    diff_api.ProgramStringARB(GL_VERTEX_PROGRAM_ARB, toProg->format, toProg->length, toProg->string);
                else
                    diff_api.LoadProgramNV(GL_VERTEX_PROGRAM_NV, toProg->id, toProg->length, toProg->string);

                DIRTY(toProg->dirtyProgram, nbitID);
            }

            /* vertex program global/env parameters */
            if (CHECKDIRTY(b->vertexEnvParameters, bitID)) {
                for (i = 0; i < toCtx->limits.maxVertexProgramEnvParams; i++) {
                    if (CHECKDIRTY(b->vertexEnvParameter[i], bitID)) {
                        if (toProg->isARBprogram)
                            diff_api.ProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, i, to->vertexParameters[i]);
                        else
                            diff_api.ProgramParameter4fvNV(GL_VERTEX_PROGRAM_NV, i, to->vertexParameters[i]);

                        DIRTY(b->vertexEnvParameter[i], nbitID);
                    }
                }
                DIRTY(b->vertexEnvParameters, nbitID);
            }

            /* vertex program local parameters */
            if (CHECKDIRTY(toProg->dirtyParams, bitID)) {
                for (i = 0; i < toCtx->limits.maxVertexProgramLocalParams; i++) {
                    if (CHECKDIRTY(toProg->dirtyParam[i], bitID)) {


                        if (toProg->isARBprogram)
                            diff_api.ProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, i, toProg->parameters[i]);
                        else
                            diff_api.ProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_NV, i, toProg->parameters[i]);
                    }
                }
                DIRTY(toProg->dirtyParams, nbitID);
            }
        }
    }

    /*
     * Separate paths for NV vs ARB fragment program
     */
    if (to->fpEnabled) {
        /* NV fragment program binding */
        CRProgram *fromProg = from->currentFragmentProgram;
        CRProgram *toProg = to->currentFragmentProgram;
        if (CHECKDIRTY(b->fpBinding, bitID)) {
            if (fromProg->id != toProg->id) {
                diff_api.BindProgramNV(GL_FRAGMENT_PROGRAM_NV, toProg->id);
            }
            DIRTY(b->fpBinding, nbitID);
        }

        if (toProg) {
            /* fragment program text */
            if (CHECKDIRTY(toProg->dirtyProgram, bitID)) {
                diff_api.LoadProgramNV(GL_FRAGMENT_PROGRAM_NV, toProg->id, toProg->length, toProg->string);
                DIRTY(toProg->dirtyProgram, nbitID);
            }

            /* fragment program global/env parameters */
            if (CHECKDIRTY(b->fragmentEnvParameters, bitID)) {
                for (i = 0; i < toCtx->limits.maxFragmentProgramEnvParams; i++) {
                    if (CHECKDIRTY(b->fragmentEnvParameter[i], bitID)) {
                        diff_api.ProgramParameter4fvNV(GL_FRAGMENT_PROGRAM_NV, i,
                                                       to->fragmentParameters[i]);
                        DIRTY(b->fragmentEnvParameter[i], nbitID);
                    }
                }
                DIRTY(b->fragmentEnvParameters, nbitID);
            }

            /* named local parameters */
            if (CHECKDIRTY(toProg->dirtyNamedParams, bitID)) {
                 CRProgramSymbol *symbol;
                for (symbol = toProg->symbolTable; symbol; symbol = symbol->next) {
                    if (CHECKDIRTY(symbol->dirty, bitID)) {
                        GLint len = crStrlen(symbol->name);
                        diff_api.ProgramNamedParameter4fvNV(toProg->id, len,
                                                            (const GLubyte *) symbol->name,
                                                            symbol->value);
                        DIRTY(symbol->dirty, nbitID);
                    }
                }
                DIRTY(toProg->dirtyNamedParams, nbitID);
            }

            /* numbered local parameters */
            if (CHECKDIRTY(toProg->dirtyParams, bitID)) {
                for (i = 0; i < CR_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMS; i++) {
                    if (CHECKDIRTY(toProg->dirtyParam[i], bitID)) {
                        if (toProg->isARBprogram)
                            diff_api.ProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, i, toProg->parameters[i]);
                        else
                            diff_api.ProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_NV, i, toProg->parameters[i]);
                    }
                }
                DIRTY(toProg->dirtyParams, nbitID);
            }
        }
    }
    else if (to->fpEnabledARB) {
        /* ARB fragment program binding */
        CRProgram *fromProg = from->currentFragmentProgram;
        CRProgram *toProg = to->currentFragmentProgram;
        if (CHECKDIRTY(b->fpBinding, bitID)) {
            if (fromProg->id != toProg->id) {
                diff_api.BindProgramARB(GL_FRAGMENT_PROGRAM_ARB, toProg->id);
            }
            DIRTY(b->fpBinding, nbitID);
        }

        if (toProg) {
            /* fragment program text */
            if (CHECKDIRTY(toProg->dirtyProgram, bitID)) {
                diff_api.ProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, toProg->format, toProg->length, toProg->string);
                DIRTY(toProg->dirtyProgram, nbitID);
            }

            /* fragment program global/env parameters */
            if (CHECKDIRTY(b->fragmentEnvParameters, bitID)) {
                for (i = 0; i < toCtx->limits.maxFragmentProgramEnvParams; i++) {
                    if (CHECKDIRTY(b->fragmentEnvParameter[i], bitID)) {
                        diff_api.ProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, i, to->fragmentParameters[i]);
                        DIRTY(b->fragmentEnvParameter[i], nbitID);
                    }
                }
                DIRTY(b->fragmentEnvParameters, nbitID);
            }

            /* numbered local parameters */
            if (CHECKDIRTY(toProg->dirtyParams, bitID)) {
                for (i = 0; i < CR_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMS; i++) {
                    if (CHECKDIRTY(toProg->dirtyParam[i], bitID)) {
                        diff_api.ProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, i, toProg->parameters[i]);
                        DIRTY(toProg->dirtyParam[i], nbitID);
                    }
                }
                DIRTY(toProg->dirtyParams, nbitID);
            }
        }
    }

    DIRTY(b->dirty, nbitID);

    /* Resend program data */
    if (toCtx->program.bResyncNeeded)
    {
        toCtx->program.bResyncNeeded = GL_FALSE;

        crStateDiffAllPrograms(toCtx, bitID, GL_TRUE);
    }
}

/** @todo support NVprograms and add some data validity checks*/
static void
DiffProgramCallback(unsigned long key, void *pProg, void *pCtx)
{
    CRContext *pContext = (CRContext *) pCtx;
    CRProgram *pProgram = (CRProgram *) pProg;
    uint32_t i;
    (void)key;

    if (pProgram->isARBprogram)
    {
        diff_api.BindProgramARB(pProgram->target, pProgram->id);
        diff_api.ProgramStringARB(pProgram->target, pProgram->format, pProgram->length, pProgram->string);

        if (GL_VERTEX_PROGRAM_ARB == pProgram->target)
        {
            /* vertex program global/env parameters */
            for (i = 0; i < pContext->limits.maxVertexProgramEnvParams; i++) 
            {
                diff_api.ProgramEnvParameter4fvARB(GL_VERTEX_PROGRAM_ARB, i, pContext->program.vertexParameters[i]);
            }
            /* vertex program local parameters */
            for (i = 0; i < pContext->limits.maxVertexProgramLocalParams; i++) 
            {
                diff_api.ProgramLocalParameter4fvARB(GL_VERTEX_PROGRAM_ARB, i, pProgram->parameters[i]);
            }
        }
        else if (GL_FRAGMENT_PROGRAM_ARB == pProgram->target)
        {
            /* vertex program global/env parameters */
            for (i = 0; i < pContext->limits.maxFragmentProgramEnvParams; i++) 
            {
                diff_api.ProgramEnvParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, i, pContext->program.fragmentParameters[i]);
            }
            /* vertex program local parameters */
            for (i = 0; i < CR_MAX_FRAGMENT_PROGRAM_LOCAL_PARAMS; i++) 
            {
                diff_api.ProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, i, pProgram->parameters[i]);
            }
        }
        else
        {
            crError("Unexpected program target");
        }
    }
    else
    {
        diff_api.BindProgramNV(pProgram->target, pProgram->id);
    }
}

void crStateDiffAllPrograms(CRContext *g, CRbitvalue *bitID, GLboolean bForceUpdate)
{
    CRProgram *pOrigVP, *pOrigFP;

    (void) bForceUpdate; (void)bitID;

    /* save original bindings */
    pOrigVP = g->program.currentVertexProgram;
    pOrigFP = g->program.currentFragmentProgram;

    crHashtableWalk(g->program.programHash, DiffProgramCallback, g);

    /* restore original bindings */
    if (pOrigVP->isARBprogram)
        diff_api.BindProgramARB(pOrigVP->target, pOrigVP->id);
    else
        diff_api.BindProgramNV(pOrigVP->target, pOrigVP->id);

    if (pOrigFP->isARBprogram)
        diff_api.BindProgramARB(pOrigFP->target, pOrigFP->id);
    else
        diff_api.BindProgramNV(pOrigFP->target, pOrigFP->id);
}
