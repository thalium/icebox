/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "chromium.h"
#include "cr_error.h"
#include "cr_mem.h"
#include "server_dispatch.h"
#include "server.h"

void SERVER_DISPATCH_APIENTRY
crServerDispatchGenQueriesARB(GLsizei n, GLuint *queries)
{
    GLuint *local_queries;
    (void) queries;

    if (n >= INT32_MAX / sizeof(GLuint))
    {
        crError("crServerDispatchGenQueriesARB: parameter 'n' is out of range");
        return;
    }

    local_queries = (GLuint *)crCalloc(n * sizeof(*local_queries));

    if (!local_queries)
    {
        crError("crServerDispatchGenQueriesARB: out of memory");
        return;
    }

    cr_server.head_spu->dispatch_table.GenQueriesARB( n, local_queries );

    crServerReturnValue( local_queries, n * sizeof(*local_queries) );
    crFree( local_queries );
}
