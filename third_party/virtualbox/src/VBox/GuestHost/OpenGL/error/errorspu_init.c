/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include <stdio.h>
#include "cr_spu.h"

extern SPUNamedFunctionTable _cr_error_table[];

static SPUFunctions error_functions = {
        NULL, /* CHILD COPY */
        NULL, /* DATA */
        _cr_error_table /* THE ACTUAL FUNCTIONS */
};

static SPUFunctions *errorSPUInit( int id, SPU *child, SPU *self,
                unsigned int context_id,
                unsigned int num_contexts )
{
        (void) id;
        (void) context_id;
        (void) num_contexts;
        (void) child;
        (void) self;
        return &error_functions;
}

static void errorSPUSelfDispatch(SPUDispatchTable *parent)
{
        (void)parent;
}

static int errorSPUCleanup(void)
{
        return 1;
}

static SPUOptions errorSPUOptions[] = {
   { NULL, CR_BOOL, 0, NULL, NULL, NULL, NULL, NULL },
};


int SPULoad( char **name, char **super, SPUInitFuncPtr *init,
             SPUSelfDispatchFuncPtr *self, SPUCleanupFuncPtr *cleanup,
             SPUOptionsPtr *options, int *flags )
{
#ifdef IN_GUEST
        *name = "error";
#else
        *name = "hosterror";
#endif
        *super = NULL;
        *init = errorSPUInit;
        *self = errorSPUSelfDispatch;
        *cleanup = errorSPUCleanup;
        *options = errorSPUOptions;
        *flags = (SPU_NO_PACKER|SPU_NOT_TERMINAL|SPU_MAX_SERVERS_ZERO);
        
        return 1;
}
