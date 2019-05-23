/* $Id: VBoxAutostartStop.cpp $ */
/** @file
 * VBoxAutostart - VirtualBox Autostart service, stop machines during system shutdown.
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

#include <iprt/thread.h>
#include <iprt/stream.h>
#include <iprt/log.h>
#include <iprt/assert.h>
#include <iprt/message.h>

#include <algorithm>
#include <list>
#include <string>

#include "VBoxAutostart.h"

using namespace com;

/**
 * VM list entry.
 */
typedef struct AUTOSTOPVM
{
    /** ID of the VM to start. */
    Bstr           strId;
    /** Action to do with the VM. */
    AutostopType_T enmAutostopType;
} AUTOSTOPVM;

static HRESULT autostartSaveVMState(ComPtr<IConsole> &console)
{
    HRESULT rc = S_OK;
    ComPtr<IMachine> machine;
    ComPtr<IProgress> progress;

    do
    {
        /* first pause so we don't trigger a live save which needs more time/resources */
        bool fPaused = false;
        rc = console->Pause();
        if (FAILED(rc))
        {
            bool fError = true;
            if (rc == VBOX_E_INVALID_VM_STATE)
            {
                /* check if we are already paused */
                MachineState_T machineState;
                CHECK_ERROR_BREAK(console, COMGETTER(State)(&machineState));
                /* the error code was lost by the previous instruction */
                rc = VBOX_E_INVALID_VM_STATE;
                if (machineState != MachineState_Paused)
                {
                    RTMsgError("Machine in invalid state %d -- %s\n",
                               machineState, machineStateToName(machineState, false));
                }
                else
                {
                    fError = false;
                    fPaused = true;
                }
            }
            if (fError)
                break;
        }

        CHECK_ERROR(console, COMGETTER(Machine)(machine.asOutParam()));
        CHECK_ERROR(machine, SaveState(progress.asOutParam()));
        if (FAILED(rc))
        {
            if (!fPaused)
                console->Resume();
            break;
        }

        rc = showProgress(progress);
        CHECK_PROGRESS_ERROR(progress, ("Failed to save machine state"));
        if (FAILED(rc))
        {
            if (!fPaused)
                console->Resume();
        }
    } while (0);

    return rc;
}

DECLHIDDEN(RTEXITCODE) autostartStopMain(PCFGAST pCfgAst)
{
    RT_NOREF(pCfgAst);
    RTEXITCODE rcExit = RTEXITCODE_SUCCESS;
    std::list<AUTOSTOPVM> listVM;

    /*
     * Build a list of all VMs we need to autostop first, apply the overrides
     * from the configuration and start the VMs afterwards.
     */
    com::SafeIfaceArray<IMachine> machines;
    HRESULT rc = g_pVirtualBox->COMGETTER(Machines)(ComSafeArrayAsOutParam(machines));
    if (SUCCEEDED(rc))
    {
        /*
         * Iterate through the collection and construct a list of machines
         * we have to check.
         */
        for (size_t i = 0; i < machines.size(); ++i)
        {
            if (machines[i])
            {
                BOOL fAccessible;
                CHECK_ERROR_BREAK(machines[i], COMGETTER(Accessible)(&fAccessible));
                if (!fAccessible)
                    continue;

                AutostopType_T enmAutostopType;
                CHECK_ERROR_BREAK(machines[i], COMGETTER(AutostopType)(&enmAutostopType));
                if (enmAutostopType != AutostopType_Disabled)
                {
                    AUTOSTOPVM autostopVM;

                    CHECK_ERROR_BREAK(machines[i], COMGETTER(Id)(autostopVM.strId.asOutParam()));
                    autostopVM.enmAutostopType = enmAutostopType;

                    listVM.push_back(autostopVM);
                }
            }
        }

        if (   SUCCEEDED(rc)
            && !listVM.empty())
        {
            std::list<AUTOSTOPVM>::iterator it;
            for (it = listVM.begin(); it != listVM.end(); ++it)
            {
                MachineState_T enmMachineState;
                ComPtr<IMachine> machine;

                CHECK_ERROR_BREAK(g_pVirtualBox, FindMachine((*it).strId.raw(),
                                                             machine.asOutParam()));

                CHECK_ERROR_BREAK(machine, COMGETTER(State)(&enmMachineState));

                /* Wait until the VM changes from a transient state back. */
                while (   enmMachineState >= MachineState_FirstTransient
                       && enmMachineState <= MachineState_LastTransient)
                {
                    RTThreadSleep(1000);
                    CHECK_ERROR_BREAK(machine, COMGETTER(State)(&enmMachineState));
                }

                /* Only power off running machines. */
                if (   enmMachineState == MachineState_Running
                    || enmMachineState == MachineState_Paused)
                {
                    ComPtr<IConsole> console;
                    ComPtr<IProgress> progress;

                    /* open a session for the VM */
                    CHECK_ERROR_BREAK(machine, LockMachine(g_pSession, LockType_Shared));

                    /* get the associated console */
                    CHECK_ERROR_BREAK(g_pSession, COMGETTER(Console)(console.asOutParam()));

                    switch ((*it).enmAutostopType)
                    {
                        case AutostopType_SaveState:
                        {
                            rc = autostartSaveVMState(console);
                            break;
                        }
                        case AutostopType_PowerOff:
                        {
                            CHECK_ERROR_BREAK(console, PowerDown(progress.asOutParam()));

                            rc = showProgress(progress);
                            CHECK_PROGRESS_ERROR(progress, ("Failed to power off machine"));
                            break;
                        }
                        case AutostopType_AcpiShutdown:
                        {
                            BOOL fGuestEnteredACPI = false;
                            CHECK_ERROR_BREAK(console, GetGuestEnteredACPIMode(&fGuestEnteredACPI));
                            if (fGuestEnteredACPI && enmMachineState == MachineState_Running)
                            {
                                CHECK_ERROR_BREAK(console, PowerButton());

                                autostartSvcLogInfo("Waiting for VM \"%ls\" to power off...\n", (*it).strId.raw());

                                do
                                {
                                    RTThreadSleep(1000);
                                    CHECK_ERROR_BREAK(machine, COMGETTER(State)(&enmMachineState));
                                } while (enmMachineState == MachineState_Running);
                            }
                            else
                            {
                                /* Use save state instead and log this to the console. */
                                autostartSvcLogWarning("The guest of VM \"%ls\" does not support ACPI shutdown or is currently paused, saving state...\n",
                                                       (*it).strId.raw());
                                rc = autostartSaveVMState(console);
                            }
                            break;
                        }
                        default:
                            autostartSvcLogWarning("Unknown autostop type for VM \"%ls\"\n", (*it).strId.raw());
                    }
                    g_pSession->UnlockMachine();
                }
            }
        }
    }

    return rcExit;
}

