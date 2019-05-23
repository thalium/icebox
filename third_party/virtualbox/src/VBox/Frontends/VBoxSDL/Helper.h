/* $Id: Helper.h $ */
/** @file
 *
 * VBox frontends: VBoxSDL (simple frontend based on SDL):
 * Miscellaneous helpers header
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

#ifndef __H_HELPER
#define __H_HELPER

#if defined(VBOX_WITH_XPCOM) && !defined(RT_OS_DARWIN) && !defined(RT_OS_OS2)

/** Indicates that the XPCOM queue thread is needed for this platform. */
# define USE_XPCOM_QUEUE_THREAD 1

/**
 * Creates the XPCOM event thread
 *
 * @returns VBOX status code
 * @param   eqFD XPCOM event queue file descriptor
 */
int startXPCOMEventQueueThread(int eqFD);

/*
 * Notify the XPCOM thread that we consumed an XPCOM event
 */
void consumedXPCOMUserEvent(void);

/**
 * Signal to the XPCOM even queue thread that it should select for more events.
 */
void signalXPCOMEventQueueThread(void);

/**
 * Indicates to the XPCOM thread that it should terminate now.
 */
void terminateXPCOMQueueThread(void);

#endif /* defined(VBOX_WITH_XPCOM) && !defined(RT_OS_DARWIN) && !defined(RT_OS_OS2) */

#endif // __H_HELPER

