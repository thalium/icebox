/* $Id: VBoxClient.h $ */
/** @file
 *
 * VirtualBox additions user session daemon.
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

#ifndef ___vboxclient_vboxclient_h
# define ___vboxclient_vboxclient_h

#include <VBox/log.h>
#include <iprt/cpp/utils.h>
#include <iprt/string.h>

/** Exit with a fatal error. */
#define VBClFatalError(format) \
do { \
    char *pszMessage = RTStrAPrintf2 format; \
    LogRel(format); \
    vbclFatalError(pszMessage); \
} while(0)

/** Exit with a fatal error. */
extern DECLNORETURN(void) vbclFatalError(char *pszMessage);

/** Call clean-up for the current service and exit. */
extern void VBClCleanUp(bool fExit = true);

/** A simple interface describing a service.  VBoxClient will run exactly one
 * service per invocation. */
struct VBCLSERVICE
{
    /** Get the services default path to pidfile, relative to $HOME */
    /** @todo Should this also have a component relative to the X server number?
     */
    const char *(*getPidFilePath)(void);
    /** Special initialisation, if needed.  @a pause and @a resume are
     * guaranteed not to be called until after this returns. */
    int (*init)(struct VBCLSERVICE **ppInterface);
    /** Run the service main loop */
    int (*run)(struct VBCLSERVICE **ppInterface, bool fDaemonised);
    /** Clean up any global resources before we shut down hard.  The last calls
     * to @a pause and @a resume are guaranteed to finish before this is called.
     */
    void (*cleanup)(struct VBCLSERVICE **ppInterface);
};

/** Default handler for various struct VBCLSERVICE member functions. */
DECLINLINE(int) VBClServiceDefaultHandler(struct VBCLSERVICE **pSelf)
{
    RT_NOREF1(pSelf);
    return VINF_SUCCESS;
}

/** Default handler for the struct VBCLSERVICE clean-up member function.
 * Usually used because the service is cleaned up automatically when the user
 * process/X11 exits. */
DECLINLINE(void) VBClServiceDefaultCleanup(struct VBCLSERVICE **ppInterface)
{
    NOREF(ppInterface);
}

extern struct VBCLSERVICE **VBClGetClipboardService();
extern struct VBCLSERVICE **VBClGetSeamlessService();
extern struct VBCLSERVICE **VBClGetDisplayService();
extern struct VBCLSERVICE **VBClGetHostVersionService();
#ifdef VBOX_WITH_DRAG_AND_DROP
extern struct VBCLSERVICE **VBClGetDragAndDropService();
#endif /* VBOX_WITH_DRAG_AND_DROP */
extern struct VBCLSERVICE **VBClCheck3DService();
extern struct VBCLSERVICE **VBClDisplaySVGAService();

#endif /* !___vboxclient_vboxclient_h */
