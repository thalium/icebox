/* Copyright (c) 2001, Stanford University
 * All rights reserved
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#include "cr_spu.h"
#include "cr_error.h"
#include "cr_string.h"
#include <stdio.h>

/**
 * \mainpage spu_loader 
 *
 * \section Spu_loaderIntroduction Introduction
 *
 * Chromium consists of all the top-level files in the cr
 * directory.  The spu_loader module basically takes care of API dispatch,
 * and OpenGL state management.
 *
 */
void crSPUInitDispatchTable( SPUDispatchTable *table )
{
    table->copyList = NULL;
    table->copy_of = NULL;
    table->mark = 0;
    table->server = NULL;
}

