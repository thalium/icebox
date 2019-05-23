/* $Id: VBoxDD2.h $ */
/** @file
 * Built-in drivers & devices part 2 header.
 *
 * These drivers and devices are in separate modules because of LGPL.
 */

/*
 * Copyright (C) 2006-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___build_VBoxDD2_h
#define ___build_VBoxDD2_h

#include <VBox/vmm/pdm.h>

RT_C_DECLS_BEGIN

#ifdef IN_VBOXDD2
extern DECLEXPORT(const unsigned char)  g_abPcBiosBinary386[];
extern DECLEXPORT(const unsigned)       g_cbPcBiosBinary386;
extern DECLEXPORT(const unsigned char)  g_abPcBiosBinary286[];
extern DECLEXPORT(const unsigned)       g_cbPcBiosBinary286;
extern DECLEXPORT(const unsigned char)  g_abPcBiosBinary8086[];
extern DECLEXPORT(const unsigned)       g_cbPcBiosBinary8086;
extern DECLEXPORT(const unsigned char)  g_abVgaBiosBinary386[];
extern DECLEXPORT(const unsigned)       g_cbVgaBiosBinary386;
extern DECLEXPORT(const unsigned char)  g_abVgaBiosBinary286[];
extern DECLEXPORT(const unsigned)       g_cbVgaBiosBinary286;
extern DECLEXPORT(const unsigned char)  g_abVgaBiosBinary8086[];
extern DECLEXPORT(const unsigned)       g_cbVgaBiosBinary8086;
# ifdef VBOX_WITH_PXE_ROM
extern DECLEXPORT(const unsigned char)  g_abNetBiosBinary[];
extern DECLEXPORT(const unsigned)       g_cbNetBiosBinary;
# endif
#else  /* !IN_VBOXDD2 */
extern DECLIMPORT(const unsigned char)  g_abPcBiosBinary386[];
extern DECLIMPORT(const unsigned)       g_cbPcBiosBinary386;
extern DECLIMPORT(const unsigned char)  g_abPcBiosBinary286[];
extern DECLIMPORT(const unsigned)       g_cbPcBiosBinary286;
extern DECLIMPORT(const unsigned char)  g_abPcBiosBinary8086[];
extern DECLIMPORT(const unsigned)       g_cbPcBiosBinary8086;
extern DECLIMPORT(const unsigned char)  g_abVgaBiosBinary386[];
extern DECLIMPORT(const unsigned)       g_cbVgaBiosBinary386;
extern DECLIMPORT(const unsigned char)  g_abVgaBiosBinary286[];
extern DECLIMPORT(const unsigned)       g_cbVgaBiosBinary286;
extern DECLIMPORT(const unsigned char)  g_abVgaBiosBinary8086[];
extern DECLIMPORT(const unsigned)       g_cbVgaBiosBinary8086;
# ifdef VBOX_WITH_PXE_ROM
extern DECLIMPORT(const unsigned char)  g_abNetBiosBinary[];
extern DECLIMPORT(const unsigned)       g_cbNetBiosBinary;
# endif
#endif /* !IN_VBOXDD2 */

extern const PDMDEVREG g_DeviceLPC;

RT_C_DECLS_END

#endif

