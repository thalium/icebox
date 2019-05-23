/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_spu.h"
#include "chromium.h"
#include "cr_mem.h"
#include "cr_net.h"
#include "server_dispatch.h"
#include "server.h"

void SERVER_DISPATCH_APIENTRY crServerDispatchGenTextures( GLsizei n, GLuint *textures )
{
    GLuint *local_textures = (GLuint *) crAlloc(n*sizeof(*local_textures));
    (void) textures;

    crStateGenTextures(n, local_textures);

    crServerReturnValue(local_textures, n*sizeof(*local_textures));
    crFree( local_textures );
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGenProgramsNV( GLsizei n, GLuint * ids )
{
    GLuint *local_progs = (GLuint *) crAlloc( n*sizeof( *local_progs) );
    (void) ids;
    cr_server.head_spu->dispatch_table.GenProgramsNV( n, local_progs );
    crServerReturnValue( local_progs, n*sizeof( *local_progs ) );
    crFree( local_progs );
}


void SERVER_DISPATCH_APIENTRY crServerDispatchGenFencesNV( GLsizei n, GLuint * ids )
{
    GLuint *local_fences = (GLuint *) crAlloc( n*sizeof( *local_fences) );
    (void) ids;
    cr_server.head_spu->dispatch_table.GenFencesNV( n, local_fences );
    crServerReturnValue( local_fences, n*sizeof( *local_fences ) );
    crFree( local_fences );
}

void SERVER_DISPATCH_APIENTRY crServerDispatchGenProgramsARB( GLsizei n, GLuint * ids )
{
    GLuint *local_progs = (GLuint *) crAlloc( n*sizeof( *local_progs) );
    GLsizei i;
    (void) ids;
    cr_server.head_spu->dispatch_table.GenProgramsARB( n, local_progs );

    /* see comments in crServerDispatchGenTextures */
    for (i=0; i<n; ++i)
    {
        GLuint tID = crServerTranslateProgramID(local_progs[i]);
        while (crStateIsProgramARB(tID))
        {
            cr_server.head_spu->dispatch_table.GenProgramsARB(1, &tID);
            local_progs[i] = tID;
            tID = crServerTranslateProgramID(tID);
        }
    }

    crServerReturnValue( local_progs, n*sizeof( *local_progs ) );
    crFree( local_progs );
}

void SERVER_DISPATCH_APIENTRY
crServerDispatchCopyTexImage2D(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    GLsizei tw, th;

    cr_server.head_spu->dispatch_table.GetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &tw);
    cr_server.head_spu->dispatch_table.GetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &th);

    /* Workaround for a wine or ati bug. Host drivers crash unless we first provide texture bounds. */
    if (((tw!=width) || (th!=height)) && (internalFormat==GL_DEPTH_COMPONENT24))
    {
        crServerDispatchTexImage2D(target, level, internalFormat, width, height, border, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, NULL);
    }

    crStateCopyTexImage2D(target, level, internalFormat, x, y, width, height, border);
    cr_server.head_spu->dispatch_table.CopyTexImage2D(target, level, internalFormat, x, y, width, height, border);
}
