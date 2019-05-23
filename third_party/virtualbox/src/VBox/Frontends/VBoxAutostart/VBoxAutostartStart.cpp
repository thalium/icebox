/* $Id: VBoxAutostartStart.cpp $ */
/** @file
 * VBoxAutostart - VirtualBox Autostart service, start machines during system boot.
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
 */

#include <VBox/com/com.h>
#include <VBox/com/string.h>
#include <VBox/com/Guid.h>
#include <VBox/com/array.h>
#include <VBox/com/ErrorInfo.h>
#include <VBox/com/errorprint.h>

#include <iprt/message.h>
#include <iprt/thread.h>
#include <iprt/stream.h>
#include <iprt/log.h>

#include <algorithm>
#include <list>
#include <string>

#include "VBoxAutostart.h"

using namespace com;

/**
 * VM list entry.
 */
typedef struct AUTOSTARTVM
{
    /** ID of the VM to start. */
    Bstr  strId;
    /** Startup delay of the VM. */
    ULONG uStartupDelay;
} AUTOSTARTVM;

static DECLCALLBACK(bool) autostartVMCmp(const AUTOSTARTVM &vm1, const AUTOSTARTVM &vm2)
{
    return vm1.uStartupDelay <= vm2.uStartupDelay;
}

DECLHIDDEN(RTEXITCODE) autostartStartMain(PCFGAST pCfgAst)
{
    RTEXITCODE rcExit = RTEXITCODE_SUCCESS;
    int vrc = VINF_SUCCESS;
    std::list<AUTOSTARTVM> listVM;
    uint32_t uStartupDelay = 0;

    pCfgAst = autostartConfigAstGetByName(pCfgAst, "startup_delay");
    if (pCfgAst)
    {
        if (pCfgAst->enmType == CFGASTNODETYPE_KEYVALUE)
        {
            vrc = RTStrToUInt32Full(pCfgAst->u.KeyValue.aszValue, 10, &uStartupDelay);
            if (RT_FAILURE(vrc))
                return RTMsgErrorExit(RTEXITCODE_FAILURE, "'startup_delay' must be an unsigned number");
        }
    }

    if (uStartupDelay)
    {
        autostartSvcLogVerbose("Delay starting for %d seconds ...\n", uStartupDelay);
        vrc = RTThreadSleep(uStartupDelay * 1000);
    }

    if (vrc == VERR_INTERRUPTED)
        return RTEXITCODE_SUCCESS;

    /*
     * Build a list of all VMs we need to autostart first, apply the overrides
     * from the configuration and start the VMs afterwards.
     */
    com::SafeIfaceArray<IMachine> machines;
    HRESULT rc = g_pVirtualBox->COMGETTER(Machines)(ComSafeArrayAsOutParam(machines));
    if (SUCCEEDED(rc))
    {
        /*
         * Iterate through the collection
         */
        for (size_t i = 0; i < machines.size(); ++i)
        {
            if (machines[i])
            {
                BOOL fAccessible;
                CHECK_ERROR_BREAK(machines[i], COMGETTER(Accessible)(&fAccessible));
                if (!fAccessible)
                    continue;

                BOOL fAutostart;
                CHECK_ERROR_BREAK(machines[i], COMGETTER(AutostartEnabled)(&fAutostart));
                if (fAutostart)
                {
                    AUTOSTARTVM autostartVM;

                    CHECK_ERROR_BREAK(machines[i], COMGETTER(Id)(autostartVM.strId.asOutParam()));
                    CHECK_ERROR_BREAK(machines[i], COMGETTER(AutostartDelay)(&autostartVM.uStartupDelay));

                    listVM.push_back(autostartVM);
                }
            }
        }

        if (   SUCCEEDED(rc)
            && !listVM.empty())
        {
            ULONG uDelayCurr = 0;

            /* Sort by startup delay and apply base override. */
            listVM.sort(autostartVMCmp);

            std::list<AUTOSTARTVM>::iterator it;
            for (it = listVM.begin(); it != listVM.end(); ++it)
            {
                ComPtr<IMachine> machine;
                ComPtr<IProgress> progress;

                if ((*it).uStartupDelay > uDelayCurr)
                {
                    autostartSvcLogVerbose("Delay starting of the next VMs for %d seconds ...\n",
                                           (*it).uStartupDelay - uDelayCurr);
                    RTThreadSleep(((*it).uStartupDelay - uDelayCurr) * 1000);
                    uDelayCurr = (*it).uStartupDelay;
                }

                CHECK_ERROR_BREAK(g_pVirtualBox, FindMachine((*it).strId.raw(),
                                                             machine.asOutParam()));

                CHECK_ERROR_BREAK(machine, LaunchVMProcess(g_pSession, Bstr("headless").raw(),
                                                           Bstr("").raw(), progress.asOutParam()));
                if (SUCCEEDED(rc) && !progress.isNull())
                {
                    autostartSvcLogVerbose("Waiting for VM \"%ls\" to power on...\n", (*it).strId.raw());
                    CHECK_ERROR(progress, WaitForCompletion(-1));
                    if (SUCCEEDED(rc))
                    {
                        BOOL completed = true;
                        CHECK_ERROR(progress, COMGETTER(Completed)(&completed));
                        if (SUCCEEDED(rc))
                        {
                            ASSERT(completed);

                            LONG iRc;
                            CHECK_ERROR(progress, COMGETTER(ResultCode)(&iRc));
                            if (SUCCEEDED(rc))
                            {
                                if (FAILED(iRc))
                                {
                                    ProgressErrorInfo info(progress);
                                    com::GluePrintErrorInfo(info);
                                }
                                else
                                    autostartSvcLogVerbose("VM \"%ls\" has been successfully started.\n", (*it).strId.raw());
                            }
                        }
                    }
                }
                g_pSession->UnlockMachine();
            }
        }
    }

    return rcExit;
}

