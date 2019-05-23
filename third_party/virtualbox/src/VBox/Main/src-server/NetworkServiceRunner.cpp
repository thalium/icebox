/* $Id: NetworkServiceRunner.cpp $ */
/** @file
 * VirtualBox Main - interface for VBox DHCP server
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#include <map>
#include <string>
#include "NetworkServiceRunner.h"
#include <iprt/process.h>
#include <iprt/param.h>
#include <iprt/env.h>
#include <iprt/log.h>
#include <iprt/thread.h>


const std::string NetworkServiceRunner::kNsrKeyName      = "--name";
const std::string NetworkServiceRunner::kNsrKeyNetwork   = "--network";
const std::string NetworkServiceRunner::kNsrKeyTrunkType = "--trunk-type";
const std::string NetworkServiceRunner::kNsrTrunkName    = "--trunk-name";
const std::string NetworkServiceRunner::kNsrMacAddress   = "--mac-address";
const std::string NetworkServiceRunner::kNsrIpAddress    = "--ip-address";
const std::string NetworkServiceRunner::kNsrIpNetmask    = "--netmask";
const std::string NetworkServiceRunner::kNsrKeyNeedMain  = "--need-main";

struct NetworkServiceRunner::Data
{
    Data(const char* aProcName)
        : mProcName(aProcName)
        , mProcess(NIL_RTPROCESS)
        , mKillProcOnStop(false)
    {}
    const char *mProcName;
    RTPROCESS mProcess;
    std::map<std::string, std::string> mOptions;
    bool mKillProcOnStop;
};

NetworkServiceRunner::NetworkServiceRunner(const char *aProcName)
{
    m = new NetworkServiceRunner::Data(aProcName);
}


NetworkServiceRunner::~NetworkServiceRunner()
{
    stop();
    delete m;
    m = NULL;
}


int NetworkServiceRunner::setOption(const std::string& key, const std::string& val)
{
    m->mOptions.insert(std::map<std::string, std::string>::value_type(key, val));
    return VINF_SUCCESS;
}


void NetworkServiceRunner::detachFromServer()
{
    m->mProcess = NIL_RTPROCESS;
}


int NetworkServiceRunner::start(bool aKillProcOnStop)
{
    if (isRunning())
        return VINF_ALREADY_INITIALIZED;

    const char * args[10*2];

    AssertReturn(m->mOptions.size() < 10, VERR_INTERNAL_ERROR);

    /* get the path to the executable */
    char exePathBuf[RTPATH_MAX];
    const char *exePath = RTProcGetExecutablePath(exePathBuf, RTPATH_MAX);
    char *substrSl = strrchr(exePathBuf, '/');
    char *substrBs = strrchr(exePathBuf, '\\');
    char *suffix = substrSl ? substrSl : substrBs;

    if (suffix)
    {
        suffix++;
        strcpy(suffix, m->mProcName);
    }

    int index = 0;

    args[index++] = exePath;

    std::map<std::string, std::string>::const_iterator it;
    for(it = m->mOptions.begin(); it != m->mOptions.end(); ++it)
    {
        args[index++] = it->first.c_str();
        args[index++] = it->second.c_str();
    }

    args[index++] = NULL;

    int rc = RTProcCreate(suffix ? exePath : m->mProcName, args, RTENV_DEFAULT, 0, &m->mProcess);
    if (RT_FAILURE(rc))
        m->mProcess = NIL_RTPROCESS;

    m->mKillProcOnStop = aKillProcOnStop;
    return rc;
}


int NetworkServiceRunner::stop()
{
    /*
     * If the process already terminated, this function will also grab the exit
     * status and transition the process out of zombie status.
     */
    if (!isRunning())
        return VINF_OBJECT_DESTROYED;

    bool fDoKillProc = true;

    if (!m->mKillProcOnStop)
    {
        /*
         * This is a VBoxSVC Main client. Do NOT kill it but assume it was shut
         * down politely. Wait up to 1 second until the process is killed before
         * doing the final hard kill.
         */
        int rc = VINF_SUCCESS;
        for (unsigned int i = 0; i < 100; i++)
        {
            rc = RTProcWait(m->mProcess, RTPROCWAIT_FLAGS_NOBLOCK, NULL);
            if (RT_SUCCESS(rc))
                break;
            RTThreadSleep(10);
        }
        if (rc != VERR_PROCESS_RUNNING)
            fDoKillProc = false;
    }

    if (fDoKillProc)
    {
        RTProcTerminate(m->mProcess);
        int rc = RTProcWait(m->mProcess, RTPROCWAIT_FLAGS_BLOCK, NULL);
        NOREF(rc);
    }

    m->mProcess = NIL_RTPROCESS;
    return VINF_SUCCESS;
}

bool NetworkServiceRunner::isRunning()
{
    if (m->mProcess == NIL_RTPROCESS)
        return false;

    RTPROCSTATUS status;
    int rc = RTProcWait(m->mProcess, RTPROCWAIT_FLAGS_NOBLOCK, &status);

    if (rc == VERR_PROCESS_RUNNING)
        return true;

    m->mProcess = NIL_RTPROCESS;
    return false;
}
