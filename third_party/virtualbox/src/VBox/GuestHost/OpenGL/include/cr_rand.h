/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_RAND_H
#define CR_RAND_H

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLEXPORT(void) crRandSeed( unsigned long seed );
DECLEXPORT(void) crRandAutoSeed(void);
DECLEXPORT(float) crRandFloat( float min, float max );
DECLEXPORT(int) crRandInt( int min, int max );

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* CR_RAND_H */
