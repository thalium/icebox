/* $Id: AudioDriver.h $ */
/** @file
 * VirtualBox audio base class for Main audio drivers.
 */

/*
 * Copyright (C) 2018 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_AUDIODRIVER
#define ____H_AUDIODRIVER

#include <VBox/com/ptr.h>
#include <VBox/com/string.h>
#include <VBox/com/AutoLock.h>

using namespace com;

/**
 * Audio driver configuration for audio drivers implemented
 * in Main.
 */
struct AudioDriverCfg
{
    AudioDriverCfg(Utf8Str a_strDev = "", unsigned a_uInst = 0, unsigned a_uLUN = 0, Utf8Str a_strName = "")
        : strDev(a_strDev)
        , uInst(a_uInst)
        , uLUN(a_uLUN)
        , strName(a_strName) { }

    AudioDriverCfg& operator=(AudioDriverCfg that)
    {
        this->strDev  = that.strDev;
        this->uInst   = that.uInst;
        this->uLUN    = that.uLUN;
        this->strName = that.strName;

        return *this;
    }

    /** The device name. */
    Utf8Str              strDev;
    /** The device instance. */
    unsigned             uInst;
    /** The LUN the driver is attached to.
     *  Set the UINT8_MAX if not attached. */
    unsigned             uLUN;
    /** The driver name. */
    Utf8Str              strName;
};

class Console;

/**
 * Base class for all audio drivers implemented in Main.
 */
class AudioDriver
{

public:
    AudioDriver(Console *pConsole);
    virtual ~AudioDriver();

    Console *GetParent(void) { return mpConsole; }

    AudioDriverCfg *GetConfig(void) { return &mCfg; }
    int InitializeConfig(AudioDriverCfg *pCfg);

    /** Checks if audio is configured or not. */
    bool isConfigured() const { return mCfg.strName.isNotEmpty(); }

    bool IsAttached(void) { return mfAttached; }

    int doAttachDriverViaEmt(PUVM pUVM, util::AutoWriteLock *pAutoLock);
    int doDetachDriverViaEmt(PUVM pUVM, util::AutoWriteLock *pAutoLock);

protected:
    static DECLCALLBACK(int) attachDriverOnEmt(AudioDriver *pThis);
    static DECLCALLBACK(int) detachDriverOnEmt(AudioDriver *pThis);

    int configure(unsigned uLUN, bool fAttach);

    /**
     * Optional (virtual) function to give the derived audio driver
     * class the ability to add (or change) the driver configuration
     * entries when setting up.
     *
     * @return VBox status code.
     * @param  pLunCfg          CFGM configuration node of the driver.
     */
    virtual int configureDriver(PCFGMNODE pLunCfg) { RT_NOREF(pLunCfg); return VINF_SUCCESS; }

protected:

    /** Pointer to parent. */
    Console             *mpConsole;
    /** The driver's configuration. */
    AudioDriverCfg       mCfg;
    /** Whether the driver is attached or not. */
    bool                 mfAttached;
};

#endif /* !____H_AUDIODRIVER */

