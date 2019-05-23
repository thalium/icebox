/* $Id: test-gccplugin.c $ */
/** @file
 * Compiler plugin testcase \#2.
 */

/*
 * Copyright (C) 2010-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

extern void MyIprtPrintf(const char *pszFormat, ...) __attribute__((__iprt_format__(1,2)));
extern void foo(void);

void foo(void)
{
    MyIprtPrintf(0);
    MyIprtPrintf("%RX32 %d %s\n", 10, 42, "string");
}

