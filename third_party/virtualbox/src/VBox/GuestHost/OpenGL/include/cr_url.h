/* Copyright (c) 2001, Stanford University
 * All rights reserved.
 *
 * See the file LICENSE.txt for information on redistributing this software.
 */

#ifndef CR_URL_H
#define CR_URL_H

#include <iprt/cdefs.h>

#ifdef __cplusplus
extern "C" {
#endif

DECLEXPORT(int) crParseURL( const char *url, char *protocol, char *hostname,
				            unsigned short *port, unsigned short default_port );

#ifdef __cplusplus
}
#endif

#endif /* CR_URL_H */
