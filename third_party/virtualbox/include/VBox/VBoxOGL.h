/* $Id: VBoxOGL.h $ */
/** @file
 * VBox 3D Support API
 */
/*
 * Copyright (C) 2012-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualBox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */
#ifndef ___VBoxOGL_h
#define ___VBoxOGL_h

#include <iprt/cdefs.h>
#include <iprt/types.h>

RT_C_DECLS_BEGIN

/* GUI and VBox OpenGL code require scaling factor value to be stored in container
 * of type of 'double'. Communication between them is done via Main. In the same time,
 * currently, Main does not like type of 'double' to be used for an interface method parameter.
 * An integer type should be used instead. This value is used in order to specify scaling factor in type
 * of 'integer' units. It is assumed that GUI feeds Main with its internal scaling factor value
 * (which is originally of type of 'double') multiplied by this constant and converted resulting
 * value to type of 'uint32_t'. Then Main provides this data to OpenGL HGCM thread. Finally, VBox OpenGL
 * code divides received scalar by this constant and converts result to type of 'double'.
 * This constant can be increased (multiplied by 10^n) in order to get better precision
 * for scaling factor manipulations. */
#define VBOX_OGL_SCALE_FACTOR_MULTIPLIER    10000.0

/* 3D content scale factor range bounds. */
#define VBOX_OGL_SCALE_FACTOR_MIN           0.01
#define VBOX_OGL_SCALE_FACTOR_MAX           10.0

bool RTCALL VBoxOglIsOfflineRenderingAppropriate(void);
bool RTCALL VBoxOglIs3DAccelerationSupported(void);

DECLEXPORT(int) VBoxOglSetScaleFactor(uint32_t idScreen, double dScaleFactorW, double dScaleFactorH);

RT_C_DECLS_END

#endif /* !___VBoxOGL_h */
