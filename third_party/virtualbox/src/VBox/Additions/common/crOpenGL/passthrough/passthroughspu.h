/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef PASSTHROUGH_SPU_H
#define PASSTHROUGH_SPU_H 1


#include "cr_spu.h"


extern SPUNamedFunctionTable _cr_passthrough_table[];
extern void BuildPassthroughTable( SPU *child );


#endif
