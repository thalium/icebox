/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "server_dispatch.h"
#include "server.h"
#include "cr_error.h"
#include "state/cr_statetypes.h"
#include "cr_mem.h"
#include "cr_string.h"

/*
 * This file provides implementations of the basic OpenGL matrix functions.
 * We often need to twiddle with their operation in order to make tilesorting
 * and non-planar projections work.
 */



/*
 * Determine which view and projection matrices to use when in stereo mode.
 * Return 0 = left eye, 1 = right eye.
 */
int crServerGetCurrentEye(void)
{
    if (cr_server.currentEye != -1) {
        /* current eye was specified by tilesort SPU */
        return cr_server.currentEye;
    }
    else {
        /* we have a quad-buffered window and we're watching glDrawBuffer */
        GLenum drawBuffer = cr_server.curClient->currentCtxInfo->pContext->buffer.drawBuffer;
        int eye = drawBuffer == GL_BACK_RIGHT || drawBuffer == GL_FRONT_RIGHT
            || drawBuffer == GL_RIGHT;
        return eye;
    }
}


void SERVER_DISPATCH_APIENTRY crServerDispatchLoadMatrixf( const GLfloat *m )
{
    const GLenum matMode = cr_server.curClient->currentCtxInfo->pContext->transform.matrixMode;
    const CRMuralInfo *mural = cr_server.curClient->currentMural;

    crStateLoadMatrixf( m );

    if (matMode == GL_MODELVIEW && cr_server.viewOverride) {
        int eye = crServerGetCurrentEye();
        crServerApplyViewMatrix(&cr_server.viewMatrix[eye]);
    }
    else {
        cr_server.head_spu->dispatch_table.LoadMatrixf( m );
    }
}


void SERVER_DISPATCH_APIENTRY crServerDispatchLoadMatrixd( const GLdouble *m )
{
    const GLenum matMode = cr_server.curClient->currentCtxInfo->pContext->transform.matrixMode;
    const CRMuralInfo *mural = cr_server.curClient->currentMural;

    crStateLoadMatrixd( m );

    if (matMode == GL_MODELVIEW && cr_server.viewOverride) {
        int eye = crServerGetCurrentEye();
        crServerApplyViewMatrix(&cr_server.viewMatrix[eye]);
    }
    else {
        cr_server.head_spu->dispatch_table.LoadMatrixd( m );
    }
}


void SERVER_DISPATCH_APIENTRY crServerDispatchMultMatrixf( const GLfloat *m )
{
    const GLenum matMode = cr_server.curClient->currentCtxInfo->pContext->transform.matrixMode;

    if (matMode == GL_PROJECTION && cr_server.projectionOverride) {
        /* load the overriding projection matrix */
        int eye = crServerGetCurrentEye();
        crStateLoadMatrix( &cr_server.projectionMatrix[eye] );
    }
    else {
        /* the usual case */
        crStateMultMatrixf( m );
        cr_server.head_spu->dispatch_table.MultMatrixf( m );
    }
}


void SERVER_DISPATCH_APIENTRY crServerDispatchMultMatrixd( const GLdouble *m )
{
    const GLenum matMode = cr_server.curClient->currentCtxInfo->pContext->transform.matrixMode;

    if (matMode == GL_PROJECTION && cr_server.projectionOverride) {
        /* load the overriding projection matrix */
        int eye = crServerGetCurrentEye();
        crStateLoadMatrix( &cr_server.projectionMatrix[eye] );
    }
    else {
        /* the usual case */
        crStateMultMatrixd( m );
        cr_server.head_spu->dispatch_table.MultMatrixd( m );
    }
}



void SERVER_DISPATCH_APIENTRY crServerDispatchLoadIdentity( void )
{
    const GLenum matMode = cr_server.curClient->currentCtxInfo->pContext->transform.matrixMode;
    const CRMuralInfo *mural = cr_server.curClient->currentMural;

    crStateLoadIdentity();

    if (matMode == GL_MODELVIEW && cr_server.viewOverride) {
        int eye = crServerGetCurrentEye();
        crServerApplyViewMatrix(&cr_server.viewMatrix[eye]);
    }
    else {
        cr_server.head_spu->dispatch_table.LoadIdentity( );
    }
}



/*
 * The following code is used to deal with vertex programs.
 * Basically, vertex programs might not directly use the usual
 * OpenGL projection matrix to project vertices from eye coords to
 * clip coords.
 *
 * If you're using Cg then the vertex programs it generates will have
 * some comments that we can scan to figure out which program parameters
 * contain the projection matrix.
 * In this case, look at the Cg program code for a string like
 * "ModelViewProj".  Then set the crserver's 'vertprog_projection_param'
 * config option to this name.
 *
 * If you're not using Cg, you may have to tell Chromium which program
 * parameters contain the projection matrix.
 * In this case, look at the OpenGL application's vertex program code to
 * determine which program parameters contain the projection matrix.
 * Then set the crserver's 'vertprog_projection_param' config option to
 * the number of the parameter which holds the first row of the matrix.
 *
 * Yup, this is complicated.
 *
 */


static void matmul(GLfloat r[16], const GLfloat p[16], const GLfloat q[16])
{
    GLfloat tmp[16];
    int i, j, k;
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            GLfloat dot = 0.0;
            for (k = 0; k < 4; k++) {
                dot += p[i+4*k] * q[k+4*j];
            }
            tmp[i+4*j] = dot;
        }
    }
    for (i = 0; i < 16; i++)
        r[i] = tmp[i];
}


static CRServerProgram *
LookupProgram(GLuint id)
{
    CRServerProgram *prog = crHashtableSearch(cr_server.programTable, id);
    if (!prog) {
        prog = (CRServerProgram *) crAlloc(sizeof(CRServerProgram));
        if (!prog)
            return NULL;
        prog->id = id;
        prog->projParamStart = cr_server.vpProjectionMatrixParameter;
        crHashtableAdd(cr_server.programTable, id, prog);
    }
    return prog;
}


void SERVER_DISPATCH_APIENTRY
crServerDispatchProgramLocalParameter4fARB(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
#if 0
    if (target == GL_VERTEX_PROGRAM_ARB) {
        CRServerProgram *prog = LookupProgram(cr_server.currentProgram);

        if (prog && prog->projParamStart != -1) {
            if (index >= (GLuint) prog->projParamStart && index <= (GLuint) prog->projParamStart + 3) {
                /* save the parameters as rows in the matrix */
                const int i = index - prog->projParamStart;
                prog->projMat[4*0+i] = x;
                prog->projMat[4*1+i] = y;
                prog->projMat[4*2+i] = z;
                prog->projMat[4*3+i] = w;
            }

            /* When we get the 4th row (row==3) of the projection matrix we can
             * then pre-multiply it by the base matrix and update the program
             * parameters with the new matrix.
             */
            if (index == (GLuint) (prog->projParamStart + 3)) {
                const CRMuralInfo *mural = cr_server.curClient->currentMural;
                const GLfloat *baseMat = (const GLfloat *) &(mural->extents[mural->curExtent].baseProjection);
                int i;
                GLfloat mat[16];

                /* pre-mult the projection matrix by the base projection */
                matmul(mat, baseMat, prog->projMat);
                /* update the program parameters with the new matrix */
                for (i = 0; i < 4; i++) {
                    cr_server.head_spu->dispatch_table.ProgramLocalParameter4fARB(target, index + i - 3, mat[4*0+i], mat[4*1+i], mat[4*2+i], mat[4*3+i]);
                }
                return;  /* done */
            }
        }
    }
#endif

    /* if we get here, pass the call through unchanged */
    cr_server.head_spu->dispatch_table.ProgramLocalParameter4fARB(target, index, x, y, z, w);
}


void SERVER_DISPATCH_APIENTRY
crServerDispatchProgramLocalParameter4dARB(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    crServerDispatchProgramLocalParameter4fARB(target, index, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}


void SERVER_DISPATCH_APIENTRY
crServerDispatchProgramParameter4fNV(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
#if 0
    if (target == GL_VERTEX_PROGRAM_NV) {
        CRServerProgram *prog = LookupProgram(cr_server.currentProgram);

        if (prog && prog->projParamStart != -1) {
            if (index >= (GLuint) prog->projParamStart && index <= (GLuint) prog->projParamStart + 3) {
                /* save the parameters as rows in the matrix */
                const int i = index - prog->projParamStart;
                prog->projMat[4*0+i] = x;
                prog->projMat[4*1+i] = y;
                prog->projMat[4*2+i] = z;
                prog->projMat[4*3+i] = w;
            }

            /* When we get the 4th row (row==3) of the projection matrix we can
             * then pre-multiply it by the base matrix and update the program
             * parameters with the new matrix.
             */
            if (index == (GLuint) (prog->projParamStart + 3)) {
                const CRMuralInfo *mural = cr_server.curClient->currentMural;
                const GLfloat *baseMat = (const GLfloat *) &(mural->extents[mural->curExtent].baseProjection);
                int i;
                GLfloat mat[16];

                /* pre-mult the projection matrix by the base projection */
                matmul(mat, baseMat, prog->projMat);
                /* update the program parameters with the new matrix */
                for (i = 0; i < 4; i++) {
                    cr_server.head_spu->dispatch_table.ProgramParameter4fNV(target, index + i - 3, mat[4*0+i], mat[4*1+i], mat[4*2+i], mat[4*3+i]);
                }
                return; /* done */
            }
        }
    }
#endif

    /* if we get here, pass the call through unchanged */
    cr_server.head_spu->dispatch_table.ProgramParameter4fNV(target, index, x, y, z, w);
}


void SERVER_DISPATCH_APIENTRY
crServerDispatchProgramParameter4dNV(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    crServerDispatchProgramParameter4fNV(target, index, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}


void SERVER_DISPATCH_APIENTRY
crServerDispatchProgramStringARB(GLenum target, GLenum format, GLsizei len, const GLvoid *string)
{
    if (target == GL_VERTEX_PROGRAM_ARB &&
            cr_server.vpProjectionMatrixVariable != NULL) {
        /* scan the program string looking for 'vertprog_projection'
         * If the program was generated by Cg, the info we want will look
         * something like this:
         * #var float4x4 ModelViewProj :  : c[0], 4 : 1 : 1
         */
        CRServerProgram *prog = LookupProgram(cr_server.currentProgram);
        CRASSERT(prog);
        if (prog) {
            const char *varPos, *paramPos;
            varPos = crStrstr((const char *) string, cr_server.vpProjectionMatrixVariable);
            if (varPos) {
                 paramPos = crStrstr(varPos, "c[");
                 if (paramPos) {
                        char number[10];
                        int i = 0;
                        paramPos += 2;  /* skip "c[" */
                        while (crIsDigit(paramPos[i])) {
                             number[i] = paramPos[i];
                             i++;
                        }
                        number[i] = 0;
                        prog->projParamStart = crStrToInt(number);
                 }
            }
            else {
                 crWarning("Didn't find %s parameter in vertex program string",
                                     cr_server.vpProjectionMatrixVariable);
            }
        }
    }

    /* pass through */
    crStateProgramStringARB(target, format, len, string);
    cr_server.head_spu->dispatch_table.ProgramStringARB(target, format, len, string);
}


void SERVER_DISPATCH_APIENTRY
crServerDispatchLoadProgramNV(GLenum target, GLuint id, GLsizei len, const GLubyte *string)
{
    if (target == GL_VERTEX_PROGRAM_NV &&
            cr_server.vpProjectionMatrixVariable != NULL) {
        /* scan the program string looking for 'vertprog_projection'
         * If the program was generated by Cg, the info we want will look
         * something like this:
         * #var float4x4 ModelViewProj :  : c[0], 4 : 1 : 1
         */
        CRServerProgram *prog = LookupProgram(id);
        CRASSERT(prog);
        if (prog) {
            const char *varPos, *paramPos;
            varPos = crStrstr((const char *) string, cr_server.vpProjectionMatrixVariable);
            if (varPos) {
                 paramPos = crStrstr(varPos, "c[");
                 if (paramPos) {
                        char number[10];
                        int i = 0;
                        paramPos += 2;  /* skip "c[" */
                        while (crIsDigit(paramPos[i])) {
                             number[i] = paramPos[i];
                             i++;
                        }
                        number[i] = 0;
                        prog->projParamStart = crStrToInt(number);
                 }
            }
            else {
                 crWarning("Didn't find %s parameter in vertex program string",
                                     cr_server.vpProjectionMatrixVariable);
            }
        }
    }

    /* pass through */
    crStateLoadProgramNV(target, id, len, string);
    cr_server.head_spu->dispatch_table.LoadProgramNV(target, id, len, string);
}


void SERVER_DISPATCH_APIENTRY
crServerDispatchBindProgramARB(GLenum target, GLuint id)
{
    id = crServerTranslateProgramID(id);

    if (target == GL_VERTEX_PROGRAM_ARB) {
        CRServerProgram *prog = LookupProgram(id);
        (void) prog;
        cr_server.currentProgram = id;
    }

    /* pass through */
    crStateBindProgramARB(target, id);
    cr_server.head_spu->dispatch_table.BindProgramARB(target, id);
}


void SERVER_DISPATCH_APIENTRY
crServerDispatchBindProgramNV(GLenum target, GLuint id)
{
    if (target == GL_VERTEX_PROGRAM_NV) {
        CRServerProgram *prog = LookupProgram(id);
        (void) prog;
        cr_server.currentProgram = id;
    }
    /* pass through */
    crStateBindProgramNV(target, id);
    cr_server.head_spu->dispatch_table.BindProgramNV(target, id);
}
