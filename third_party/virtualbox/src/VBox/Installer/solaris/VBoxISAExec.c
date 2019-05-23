/* $Id: VBoxISAExec.c $ */
/** @file
 * VBoxISAExec, ISA exec wrapper, Solaris hosts.
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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[], char *envv[])
{
    int rc = 0;
    const char *pszExec = getexecname();

    if (!pszExec)
    {
        fprintf(stderr, "Failed to get executable name.\n");
        return -1;
    }

    rc = isaexec(pszExec, argv, envv);
    if (rc == -1)
        fprintf(stderr, "Failed to find/execute ISA specific executable for %s\n", pszExec);

    return rc;
}

