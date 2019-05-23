/* $Id: tstDarwinKeyboard.cpp $ */
/** @file
 * VBox Qt GUI Testcase - Common GUI Library - Darwin Keyboard routines.
 *
 * @todo Move this up somewhere so that the two SDL GUIs can use parts of this code too (-HID crap).
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

# include <iprt/initterm.h>
# include <iprt/stream.h>
# include <iprt/string.h>
# include <iprt/time.h>
# include <iprt/assert.h>

# include "DarwinKeyboard.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */


int main(int argc, char **argv)
{
    int rc = RTR3InitExe(argc, &argv, 0);
    AssertReleaseRCReturn(rc, 1);

    /*
     * Warmup tests.
     */
    RTPrintf("tstDarwinKeyboard: Warmup...\n");

    RTTimeNanoTS();
    DarwinGrabKeyboard(true);
    DarwinReleaseKeyboard();

    RTTimeNanoTS();
    DarwinGrabKeyboard(true);
    DarwinReleaseKeyboard();

/* Test these too:
unsigned DarwinKeycodeToSet1Scancode(unsigned uKeyCode);
UInt32   DarwinAdjustModifierMask(UInt32 fModifiers);
unsigned DarwinModifierMaskToSet1Scancode(UInt32 fModifiers);
unsigned DarwinModifierMaskToDarwinKeycode(UInt32 fModifiers);
UInt32   DarwinKeyCodeToDarwinModifierMask(unsigned uKeyCode);
unsigned DarwinEventToSet1Scancode(EventRef Event, UInt32 *pfCurKeyModifiers);
void     DarwinDisableGlobalHotKeys(bool fDisable);
*/

    /*
     * Grab and release the keyboard a lot of times and time it.
     * We're looking both at performance and for memory and reference leaks here.
     */
    RTPrintf("tstDarwinKeyboard: Profiling Grab and Release");
    RTStrmFlush(g_pStdOut);
    const uint64_t u64Start = RTTimeNanoTS();
    uint64_t u64Grab = 0;
    uint64_t u64Release = 0;
    unsigned i;
    for (i = 0; i < 20; i++)
    {
        uint64_t u64 = RTTimeNanoTS();
        DarwinGrabKeyboard(argc != 1);
        u64Grab += RTTimeNanoTS() - u64;

        u64 = RTTimeNanoTS();
        DarwinReleaseKeyboard();
        u64Release += RTTimeNanoTS() - u64;

        if ((i % 10) == 0)
        {
            RTPrintf(".");
            RTStrmFlush(g_pStdOut);
        }
    }
    const uint64_t u64Elapsed = RTTimeNanoTS() - u64Start;
    RTPrintf("\n"
             "tstDarwinKeyboard: %u times in %RU64 ms - %RU64 ms per call\n",
             i, u64Elapsed / 1000000, (u64Elapsed / i) / 1000000);
    RTPrintf("tstDarwinKeyboard: DarwinGrabKeyboard: %RU64 ms total - %RU64 ms per call\n",
             u64Grab / 1000000, (u64Grab / i) / 1000000);
    RTPrintf("tstDarwinKeyboard: DarwinReleaseKeyboard: %RU64 ms total - %RU64 ms per call\n",
             u64Release / 1000000, (u64Release / i) / 1000000);

    return 0;
}

