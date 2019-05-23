/* $Id: ClientToken.h $ */

/** @file
 *
 * VirtualBox API client session token abstraction
 */

/*
 * Copyright (C) 2013-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_CLIENTTOKEN
#define ____H_CLIENTTOKEN

#include <VBox/com/ptr.h>
#include <VBox/com/AutoLock.h>

#include "MachineImpl.h"
#ifdef VBOX_WITH_GENERIC_SESSION_WATCHER
# include "TokenImpl.h"
#endif /* VBOX_WITH_GENERIC_SESSION_WATCHER */

#if defined(RT_OS_WINDOWS)
# define CTTOKENARG NULL
# define CTTOKENTYPE HANDLE
#elif defined(RT_OS_OS2)
# define CTTOKENARG NULLHANDLE
# define CTTOKENTYPE HMTX
#elif defined(VBOX_WITH_SYS_V_IPC_SESSION_WATCHER)
# define CTTOKENARG -1
# define CTTOKENTYPE int
#elif defined(VBOX_WITH_GENERIC_SESSION_WATCHER)
# define CTTOKENARG NULL
# define CTTOKENTYPE MachineToken *
#else
# error "Port me!"
#endif

/**
 * Class which represents a token which can be used to check for client
 * crashes and similar purposes.
 */
class Machine::ClientToken
{
public:
    /**
     * Constructor which creates a usable instance
     *
     * @param pMachine          Reference to Machine object
     * @param pSessionMachine   Reference to corresponding SessionMachine object
     */
    ClientToken(const ComObjPtr<Machine> &pMachine, SessionMachine *pSessionMachine);

    /**
     * Default destructor. Cleans everything up.
     */
    ~ClientToken();

    /**
     * Check if object contains a usable token.
     */
    bool isReady();

    /**
     * Query token ID, which is a unique string value for this token. Do not
     * assume any specific content/format, it is opaque information.
     */
    void getId(Utf8Str &strId);

    /**
     * Query token, which is platform dependent.
     */
    CTTOKENTYPE getToken();

#ifndef VBOX_WITH_GENERIC_SESSION_WATCHER
    /**
     * Release token now. Returns information if the client has terminated.
     */
    bool release();
#endif /* !VBOX_WITH_GENERIC_SESSION_WATCHER */

private:
    /**
     * Default constructor. Don't use, will not create a sensible instance.
     */
    ClientToken();

    Machine *mMachine;
    CTTOKENTYPE mClientToken;
    Utf8Str mClientTokenId;
#ifdef VBOX_WITH_GENERIC_SESSION_WATCHER
    bool mClientTokenPassed;
#endif
};

#endif /* !____H_CLIENTTOKEN */
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
