/* $Id: HostPower.cpp $ */
/** @file
 *
 * VirtualBox interface to host's power notification service
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

#include "HostPower.h"
#include "Logging.h"

#include <VBox/com/ptr.h>

#include "VirtualBoxImpl.h"
#include "MachineImpl.h"

#include <iprt/mem.h>
#include <iprt/cpp/utils.h>

HostPowerService::HostPowerService(VirtualBox *aVirtualBox)
{
    AssertPtr(aVirtualBox);
    mVirtualBox = aVirtualBox;
}

HostPowerService::~HostPowerService()
{
}

void HostPowerService::notify(Reason_T aReason)
{
    SessionMachinesList machines;
    VirtualBox::InternalControlList controls;

    HRESULT rc = S_OK;

    switch (aReason)
    {
        case Reason_HostSuspend:
        {
            LogFunc(("HOST SUSPEND\n"));

#ifdef VBOX_WITH_RESOURCE_USAGE_API
            /* Suspend performance sampling to avoid unnecessary callbacks due to jumps in time. */
            PerformanceCollector *perfcollector = mVirtualBox->i_performanceCollector();

            if (perfcollector)
                perfcollector->suspendSampling();
#endif
            mVirtualBox->i_getOpenedMachines(machines, &controls);

            /* pause running VMs */
            for (VirtualBox::InternalControlList::const_iterator it = controls.begin();
                 it != controls.end();
                 ++it)
            {
                ComPtr<IInternalSessionControl> pControl = *it;

                /* PauseWithReason() will simply return a failure if
                 * the VM is in an inappropriate state */
                rc = pControl->PauseWithReason(Reason_HostSuspend);
                if (FAILED(rc))
                    continue;

                /* save the control to un-pause the VM later */
                mSessionControls.push_back(pControl);
            }

            LogRel(("Host suspending: Paused %d VMs\n", mSessionControls.size()));
            break;
        }

        case Reason_HostResume:
        {
            LogFunc(("HOST RESUME\n"));

            size_t resumed = 0;

            /* go through VMs we paused on Suspend */
            for (size_t i = 0; i < mSessionControls.size(); ++i)
            {
                /* note that Resume() will simply return a failure if the VM is
                 * in an inappropriate state (it will also fail if the VM has
                 * been somehow closed by this time already so that the
                 * console reference we have is dead) */
                rc = mSessionControls[i]->ResumeWithReason(Reason_HostResume);
                if (FAILED(rc))
                    continue;

                ++resumed;
            }

            LogRel(("Host resumed: Resumed %d VMs\n", resumed));

#ifdef VBOX_WITH_RESOURCE_USAGE_API
            /* Resume the performance sampling. */
            PerformanceCollector *perfcollector = mVirtualBox->i_performanceCollector();

            if (perfcollector)
                perfcollector->resumeSampling();
#endif

            mSessionControls.clear();
            break;
        }

        case Reason_HostBatteryLow:
        {
            LogFunc(("BATTERY LOW\n"));

            Bstr value;
            rc = mVirtualBox->GetExtraData(Bstr("VBoxInternal2/SavestateOnBatteryLow").raw(),
                                           value.asOutParam());
            int fGlobal = 0;
            if (SUCCEEDED(rc) && !value.isEmpty())
            {
                if (value != "0")
                    fGlobal = 1;
                else if (value == "0")
                    fGlobal = -1;
            }

            mVirtualBox->i_getOpenedMachines(machines, &controls);
            size_t saved = 0;

            /* save running VMs */
            for (SessionMachinesList::const_iterator it = machines.begin();
                 it != machines.end();
                 ++it)
            {
                ComPtr<SessionMachine> pMachine = *it;
                rc = pMachine->GetExtraData(Bstr("VBoxInternal2/SavestateOnBatteryLow").raw(),
                                            value.asOutParam());
                int fPerVM = 0;
                if (SUCCEEDED(rc) && !value.isEmpty())
                {
                    /* per-VM overrides global */
                    if (value != "0")
                        fPerVM = 2;
                    else if (value == "0")
                        fPerVM = -2;
                }

                /* default is true */
                if (fGlobal + fPerVM >= 0)
                {
                    ComPtr<IProgress> progress;

                    /* SessionMachine::i_saveStateWithReason() will return
                     * a failure if the VM is in an inappropriate state */
                    rc = pMachine->i_saveStateWithReason(Reason_HostBatteryLow, progress);
                    if (FAILED(rc))
                    {
                        LogRel(("SaveState '%s' failed with %Rhrc\n", pMachine->i_getName().c_str(), rc));
                        continue;
                    }

                    /* Wait until the operation has been completed. */
                    rc = progress->WaitForCompletion(-1);
                    if (SUCCEEDED(rc))
                    {
                        LONG iRc;
                        progress->COMGETTER(ResultCode)(&iRc);
                        rc = iRc;
                    }

                    AssertMsg(SUCCEEDED(rc), ("SaveState WaitForCompletion failed with %Rhrc (%#08X)\n", rc, rc));

                    if (SUCCEEDED(rc))
                    {
                        LogRel(("SaveState '%s' succeeded\n", pMachine->i_getName().c_str()));
                        ++saved;
                    }
                }
            }
            LogRel(("Battery Low: saved %d VMs\n", saved));
            break;
        }

        default:
            /* nothing */;
    }
}
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
