/* $Id: VBoxIChatTheaterWrapper.h $ */
/** @file
 * VBox Qt GUI - iChat Theater cocoa wrapper.
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___VBoxIChatTheaterWrapper_h
#define ___VBoxIChatTheaterWrapper_h

#if defined (VBOX_WS_MAC) && defined (VBOX_WITH_ICHAT_THEATER)

# include <ApplicationServices/ApplicationServices.h>

RT_C_DECLS_BEGIN

void initSharedAVManager();
void setImageRef (CGImageRef aImage);

RT_C_DECLS_END

#endif

#endif

