/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */


#include "cr_error.h"
#include "cr_environment.h"
#include "cr_pack.h"
#include "packer.h"


/*
 * Set the error handler callback
 */
void crPackErrorFunction( CRPackContext *pc, CRPackErrorHandlerFunc function )
{
    pc->Error = function;
}


/*
 * This function is called by the packer functions when it detects and
 * OpenGL error.
 */
void __PackError( int line, const char *file, GLenum error, const char *info)
{
    CR_GET_PACKER_CONTEXT(pc);

    if (pc->Error)
        pc->Error( line, file, error, info );

    if (crGetenv("CR_DEBUG"))
    {
        char *glerr;

        switch (error) {
        case GL_NO_ERROR:
            glerr = "GL_NO_ERROR";
            break;
        case GL_INVALID_VALUE:
            glerr = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_ENUM:
            glerr = "GL_INVALID_ENUM";
            break;
        case GL_INVALID_OPERATION:
            glerr = "GL_INVALID_OPERATION";
            break;
        case GL_STACK_OVERFLOW:
            glerr = "GL_STACK_OVERFLOW";
            break;
        case GL_STACK_UNDERFLOW:
            glerr = "GL_STACK_UNDERFLOW";
            break;
        case GL_OUT_OF_MEMORY:
            glerr = "GL_OUT_OF_MEMORY";
            break;
        case GL_TABLE_TOO_LARGE:
            glerr = "GL_TABLE_TOO_LARGE";
            break;
        default:
            glerr = "unknown";
            break;
        }

        crWarning( "GL error in packer: %s, line %d: %s: %s",
                         file, line, glerr, info );
    }
}
