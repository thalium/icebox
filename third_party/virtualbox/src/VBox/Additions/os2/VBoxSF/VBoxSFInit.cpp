/* $Id: VBoxSFInit.cpp $ */
/** @file
 * VBoxSF - OS/2 Shared Folders, Initialization.
 */

/*
 * Copyright (c) 2007 knut st. osmundsen <bird-src-spam@anduin.net>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_DEFAULT
#include "VBoxSFInternal.h"

#include <VBox/VBoxGuestLib.h>
#include <VBox/log.h>
#include <iprt/assert.h>
#include <iprt/initterm.h>


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
RT_C_DECLS_BEGIN
/* from VBoxSFA.asm */
extern RTFAR16 g_fpfnDevHlp;
extern VBGLOS2ATTACHDD g_VBoxGuestIDC;
extern uint32_t g_u32Info;
/* from sys0.asm and the linker/end.lib. */
extern char _text, _etext, _data, _end;
RT_C_DECLS_END



/**
 * 32-bit Ring-0 init routine.
 *
 * This is called the first time somebody tries to use the IFS.
 * It will initialize IPRT, Vbgl and whatever else is required.
 *
 * The caller will do the necessary AttachDD and calling of the 16 bit
 * IDC to initialize the g_VBoxGuestIDC global. Perhaps we should move
 * this bit to VbglR0InitClient? It's just that it's so much simpler to do it
 * while we're on the way here...
 *
 */
DECLASM(void) VBoxSFR0Init(void)
{
    Log(("VBoxSFR0Init: g_fpfnDevHlp=%lx u32Version=%RX32 u32Session=%RX32 pfnServiceEP=%p g_u32Info=%u (%#x)\n",
         g_fpfnDevHlp, g_VBoxGuestIDC.u32Version, g_VBoxGuestIDC.u32Session, g_VBoxGuestIDC.pfnServiceEP, g_u32Info, g_u32Info));

    /*
     * Start by initializing IPRT.
     */
    if (    g_VBoxGuestIDC.u32Version == VBGL_IOC_VERSION
        &&  VALID_PTR(g_VBoxGuestIDC.u32Session)
        &&  VALID_PTR(g_VBoxGuestIDC.pfnServiceEP))
    {
        int rc = RTR0Init(0);
        if (RT_SUCCESS(rc))
        {
            rc = VbglR0InitClient();
            if (RT_SUCCESS(rc))
            {
#ifndef DONT_LOCK_SEGMENTS
                /*
                 * Lock the 32-bit segments in memory.
                 */
                static KernVMLock_t s_Text32, s_Data32;
                rc = KernVMLock(VMDHL_LONG,
                                &_text, (uintptr_t)&_etext - (uintptr_t)&_text,
                                &s_Text32, (KernPageList_t *)-1, NULL);
                AssertMsg(rc == NO_ERROR, ("locking text32 failed, rc=%d\n"));
                rc = KernVMLock(VMDHL_LONG | VMDHL_WRITE,
                                &_data, (uintptr_t)&_end - (uintptr_t)&_data,
                                &s_Data32, (KernPageList_t *)-1, NULL);
                AssertMsg(rc == NO_ERROR, ("locking text32 failed, rc=%d\n"));
#endif

                Log(("VBoxSFR0Init: completed successfully\n"));
                return;
            }
        }

        LogRel(("VBoxSF: RTR0Init failed, rc=%Rrc\n", rc));
    }
    else
        LogRel(("VBoxSF: Failed to connect to VBoxGuest.sys.\n"));
}

