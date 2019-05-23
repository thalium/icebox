/* $Id: ClientToken.cpp $ */
/** @file
 *
 * VirtualBox API client session crash token handling
 */

/*
 * Copyright (C) 2004-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#define LOG_GROUP LOG_GROUP_MAIN
#include <iprt/asm.h>
#include <iprt/assert.h>
#include <VBox/log.h>
#include <iprt/semaphore.h>
#include <iprt/process.h>

#ifdef VBOX_WITH_SYS_V_IPC_SESSION_WATCHER
# include <errno.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/ipc.h>
# include <sys/sem.h>
#endif

#include <VBox/com/defs.h>

#include <vector>

#include "VirtualBoxBase.h"
#include "AutoCaller.h"
#include "ClientToken.h"
#include "MachineImpl.h"

Machine::ClientToken::ClientToken()
{
    AssertReleaseFailed();
}

Machine::ClientToken::~ClientToken()
{
#if defined(RT_OS_WINDOWS)
    if (mClientToken)
    {
        LogFlowFunc(("Closing mClientToken=%p\n", mClientToken));
        ::CloseHandle(mClientToken);
    }
#elif defined(RT_OS_OS2)
    if (mClientToken != NULLHANDLE)
        ::DosCloseMutexSem(mClientToken);
#elif defined(VBOX_WITH_SYS_V_IPC_SESSION_WATCHER)
    if (mClientToken >= 0)
        ::semctl(mClientToken, 0, IPC_RMID);
# ifdef VBOX_WITH_NEW_SYS_V_KEYGEN
    mClientTokenId = "0";
# endif /* VBOX_WITH_NEW_SYS_V_KEYGEN */
#elif defined(VBOX_WITH_GENERIC_SESSION_WATCHER)
    /* release the token, uses reference counting */
    if (mClientToken)
    {
        if (!mClientTokenPassed)
            mClientToken->Release();
        mClientToken = NULL;
    }
#else
# error "Port me!"
#endif
    mClientToken = CTTOKENARG;
}

Machine::ClientToken::ClientToken(const ComObjPtr<Machine> &pMachine,
                                  SessionMachine *pSessionMachine) :
    mMachine(pMachine)
{
#if defined(RT_OS_WINDOWS)
    NOREF(pSessionMachine);
    Bstr tokenId = pMachine->mData->m_strConfigFileFull;
    for (size_t i = 0; i < tokenId.length(); i++)
        if (tokenId.raw()[i] == '\\')
            tokenId.raw()[i] = '/';
    mClientToken = ::CreateMutex(NULL, FALSE, tokenId.raw());
    mClientTokenId = tokenId;
    AssertMsg(mClientToken,
              ("Cannot create token '%s', err=%d",
               mClientTokenId.c_str(), ::GetLastError()));
#elif defined(RT_OS_OS2)
    NOREF(pSessionMachine);
    Utf8Str ipcSem = Utf8StrFmt("\\SEM32\\VBOX\\VM\\{%RTuuid}",
                                pMachine->mData->mUuid.raw());
    mClientTokenId = ipcSem;
    APIRET arc = ::DosCreateMutexSem((PSZ)ipcSem.c_str(), &mClientToken, 0, FALSE);
    AssertMsg(arc == NO_ERROR,
              ("Cannot create token '%s', arc=%ld",
               ipcSem.c_str(), arc));
#elif defined(VBOX_WITH_SYS_V_IPC_SESSION_WATCHER)
    NOREF(pSessionMachine);
# ifdef VBOX_WITH_NEW_SYS_V_KEYGEN
#  if defined(RT_OS_FREEBSD) && (HC_ARCH_BITS == 64)
    /** @todo Check that this still works correctly. */
    AssertCompileSize(key_t, 8);
#  else
    AssertCompileSize(key_t, 4);
#  endif
    key_t key;
    mClientToken = -1;
    mClientTokenId = "0";
    for (uint32_t i = 0; i < 1 << 24; i++)
    {
        key = ((uint32_t)'V' << 24) | i;
        int sem = ::semget(key, 1, S_IRUSR | S_IWUSR | IPC_CREAT | IPC_EXCL);
        if (sem >= 0 || (errno != EEXIST && errno != EACCES))
        {
            mClientToken = sem;
            if (sem >= 0)
                mClientTokenId = BstrFmt("%u", key);
            break;
        }
    }
# else /* !VBOX_WITH_NEW_SYS_V_KEYGEN */
    Utf8Str semName = pMachine->mData->m_strConfigFileFull;
    char *pszSemName = NULL;
    RTStrUtf8ToCurrentCP(&pszSemName, semName);
    key_t key = ::ftok(pszSemName, 'V');
    RTStrFree(pszSemName);

    mClientToken = ::semget(key, 1, S_IRWXU | S_IRWXG | S_IRWXO | IPC_CREAT);
# endif /* !VBOX_WITH_NEW_SYS_V_KEYGEN */

    int errnoSave = errno;
    if (mClientToken < 0 && errnoSave == ENOSYS)
    {
        mMachine->setError(E_FAIL,
                           tr("Cannot create IPC semaphore. Most likely your host kernel lacks "
                              "support for SysV IPC. Check the host kernel configuration for "
                              "CONFIG_SYSVIPC=y"));
        mClientToken = CTTOKENARG;
        return;
    }
    /* ENOSPC can also be the result of VBoxSVC crashes without properly freeing
     * the token */
    if (mClientToken < 0 && errnoSave == ENOSPC)
    {
#ifdef RT_OS_LINUX
        mMachine->setError(E_FAIL,
                           tr("Cannot create IPC semaphore because the system limit for the "
                              "maximum number of semaphore sets (SEMMNI), or the system wide "
                              "maximum number of semaphores (SEMMNS) would be exceeded. The "
                              "current set of SysV IPC semaphores can be determined from "
                              "the file /proc/sysvipc/sem"));
#else
        mMachine->setError(E_FAIL,
                           tr("Cannot create IPC semaphore because the system-imposed limit "
                              "on the maximum number of allowed  semaphores or semaphore "
                              "identifiers system-wide would be exceeded"));
#endif
        mClientToken = CTTOKENARG;
        return;
    }
    AssertMsgReturnVoid(mClientToken >= 0, ("Cannot create token, errno=%d", errnoSave));
    /* set the initial value to 1 */
    int rv = ::semctl(mClientToken, 0, SETVAL, 1);
    errnoSave = errno;
    if (rv != 0)
    {
        ::semctl(mClientToken, 0, IPC_RMID);
        mClientToken = CTTOKENARG;
        AssertMsgFailedReturnVoid(("Cannot init token, errno=%d", errnoSave));
    }
#elif defined(VBOX_WITH_GENERIC_SESSION_WATCHER)
    ComObjPtr<MachineToken> pToken;
    HRESULT rc = pToken.createObject();
    if (SUCCEEDED(rc))
    {
        rc = pToken->init(pSessionMachine);
        if (SUCCEEDED(rc))
        {
            mClientToken = pToken;
            if (mClientToken)
            {
                rc = mClientToken->AddRef();
                if (FAILED(rc))
                    mClientToken = NULL;
            }
        }
    }
    pToken.setNull();
    mClientTokenPassed = false;
    /* mClientTokenId isn't really used */
    mClientTokenId = pMachine->mData->m_strConfigFileFull;
    AssertMsg(mClientToken,
              ("Cannot create token '%s', rc=%Rhrc",
               mClientTokenId.c_str(), rc));
#else
# error "Port me!"
#endif
}

bool Machine::ClientToken::isReady()
{
    return mClientToken != CTTOKENARG;
}

void Machine::ClientToken::getId(Utf8Str &strId)
{
    strId = mClientTokenId;
}

CTTOKENTYPE Machine::ClientToken::getToken()
{
#ifdef VBOX_WITH_GENERIC_SESSION_WATCHER
    mClientTokenPassed = true;
#endif /* VBOX_WITH_GENERIC_SESSION_WATCHER */
    return mClientToken;
}

#ifndef VBOX_WITH_GENERIC_SESSION_WATCHER
bool Machine::ClientToken::release()
{
    bool terminated = false;

#if defined(RT_OS_WINDOWS)
    AssertMsg(mClientToken, ("semaphore must be created"));

    /* release the token */
    ::ReleaseMutex(mClientToken);
    terminated = true;
#elif defined(RT_OS_OS2)
    AssertMsg(mClientToken, ("semaphore must be created"));

    /* release the token */
    ::DosReleaseMutexSem(mClientToken);
    terminated = true;
#elif defined(VBOX_WITH_SYS_V_IPC_SESSION_WATCHER)
    AssertMsg(mClientToken >= 0, ("semaphore must be created"));
    int val = ::semctl(mClientToken, 0, GETVAL);
    if (val > 0)
    {
        /* the semaphore is signaled, meaning the session is terminated */
        terminated = true;
    }
#elif defined(VBOX_WITH_GENERIC_SESSION_WATCHER)
    /** @todo r=klaus never tested, this code is not reached */
    AssertMsg(mClientToken, ("token must be created"));
    /* release the token, uses reference counting */
    if (mClientToken)
    {
        if (!mClientTokenPassed)
            mClientToken->Release();
        mClientToken = NULL;
    }
    terminated = true;
#else
# error "Port me!"
#endif
    return terminated;
}
#endif /* !VBOX_WITH_GENERIC_SESSION_WATCHER */

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
