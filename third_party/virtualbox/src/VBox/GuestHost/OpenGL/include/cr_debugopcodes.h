/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_DEBUG_OPCODES_H
#define CR_DEBUG_OPCODES_H

#include "cr_opcodes.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

void crDebugOpcodes( FILE *fp, unsigned char *ptr, unsigned int num_opcodes );

#ifdef __cplusplus
}
#endif

#endif /* CR_DEBUG_OPCODES_H */
