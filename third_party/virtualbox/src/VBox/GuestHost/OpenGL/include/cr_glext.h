/* $Id: cr_glext.h $ */

/** @file
 * GL chromium platform specifics
 */
/*
 * Copyright (C) 2014-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */
#ifndef ___cr_glext_h__
#define ___cr_glext_h__

#include <iprt/assert.h>

#ifndef __glext_h_
/* glext NOT included yet */
# include "GL/glext.h"
#else
/* glext IS included yet */
# ifndef RT_OS_DARWIN
typedef unsigned int VBoxGLhandleARB;   /* shader object handle */
AssertCompile(sizeof (VBoxGLhandleARB) == sizeof (GLhandleARB));
# else
#  error "patched glext is required for OSX!"
# endif
#endif
#endif /*___cr_glext_h__*/
