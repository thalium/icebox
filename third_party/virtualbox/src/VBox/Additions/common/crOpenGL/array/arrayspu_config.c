/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "arrayspu.h"

#include "cr_string.h"

#include <stdio.h>

static void __setDefaults( void )
{
}

/* No SPU options yet.
 */
SPUOptions arraySPUOptions[] = {
   { NULL, CR_BOOL, 0, NULL, NULL, NULL, NULL, NULL },
};


void arrayspuSetVBoxConfiguration( void )
{
    __setDefaults();
}
