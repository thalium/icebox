/* $Id: EmulatedUSBImpl.cpp $ */
/** @file
 * Emulated USB manager implementation.
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

#define LOG_GROUP LOG_GROUP_MAIN_EMULATEDUSB
#include "LoggingNew.h"

#include "EmulatedUSBImpl.h"
#include "ConsoleImpl.h"

#include <VBox/vmm/pdmusb.h>


/*
 * Emulated USB webcam device instance.
 */
typedef std::map <Utf8Str, Utf8Str> EUSBSettingsMap;

typedef enum EUSBDEVICESTATUS
{
    EUSBDEVICE_CREATED,
    EUSBDEVICE_ATTACHING,
    EUSBDEVICE_ATTACHED
} EUSBDEVICESTATUS;

class EUSBWEBCAM /* : public EUSBDEVICE */
{
    private:
        int32_t volatile mcRefs;

        EmulatedUSB *mpEmulatedUSB;

        RTUUID mUuid;
        char mszUuid[RTUUID_STR_LENGTH];

        Utf8Str mPath;
        Utf8Str mSettings;

        EUSBSettingsMap mDevSettings;
        EUSBSettingsMap mDrvSettings;

        void *mpvObject;

        static DECLCALLBACK(int) emulatedWebcamAttach(PUVM pUVM, EUSBWEBCAM *pThis, const char *pszDriver);
        static DECLCALLBACK(int) emulatedWebcamDetach(PUVM pUVM, EUSBWEBCAM *pThis);

        HRESULT settingsParse(void);

        ~EUSBWEBCAM()
        {
        }

    public:
        EUSBWEBCAM()
            :
            mcRefs(1),
            mpEmulatedUSB(NULL),
            mpvObject(NULL),
            enmStatus(EUSBDEVICE_CREATED)
        {
            RT_ZERO(mUuid);
            RT_ZERO(mszUuid);
        }

        int32_t AddRef(void)
        {
            return ASMAtomicIncS32(&mcRefs);
        }

        void Release(void)
        {
            int32_t c = ASMAtomicDecS32(&mcRefs);
            if (c == 0)
            {
                delete this;
            }
        }

        HRESULT Initialize(Console *pConsole,
                           EmulatedUSB *pEmulatedUSB,
                           const com::Utf8Str *aPath,
                           const com::Utf8Str *aSettings,
                           void *pvObject);
        HRESULT Attach(Console *pConsole,
                       PUVM pUVM,
                       const char *pszDriver);
        HRESULT Detach(Console *pConsole,
                       PUVM pUVM);

        bool HasId(const char *pszId) { return RTStrCmp(pszId, mszUuid) == 0;}

        EUSBDEVICESTATUS enmStatus;
};

static int emulatedWebcamInsertSettings(PCFGMNODE pConfig, EUSBSettingsMap *pSettings)
{
    int rc = VINF_SUCCESS;

    EUSBSettingsMap::const_iterator it;
    for (it = pSettings->begin(); it != pSettings->end(); ++it)
    {
        /* Convert some well known settings for backward compatibility. */
        if (   RTStrCmp(it->first.c_str(), "MaxPayloadTransferSize") == 0
            || RTStrCmp(it->first.c_str(), "MaxFramerate") == 0)
        {
            uint32_t u32 = 0;
            rc = RTStrToUInt32Full(it->second.c_str(), 10, &u32);
            if (rc == VINF_SUCCESS)
            {
                rc = CFGMR3InsertInteger(pConfig, it->first.c_str(), u32);
            }
            else
            {
                if (RT_SUCCESS(rc)) /* VWRN_* */
                {
                    rc = VERR_INVALID_PARAMETER;
                }
            }
        }
        else
        {
            rc = CFGMR3InsertString(pConfig, it->first.c_str(), it->second.c_str());
        }

        if (RT_FAILURE(rc))
        {
            break;
        }
    }

    return rc;
}

/* static */ DECLCALLBACK(int) EUSBWEBCAM::emulatedWebcamAttach(PUVM pUVM, EUSBWEBCAM *pThis, const char *pszDriver)
{
    PCFGMNODE pInstance = CFGMR3CreateTree(pUVM);
    PCFGMNODE pConfig;
    CFGMR3InsertNode(pInstance,   "Config", &pConfig);
    int rc = emulatedWebcamInsertSettings(pConfig, &pThis->mDevSettings);
    if (RT_FAILURE(rc))
        return rc;
    PCFGMNODE pEUSB;
    CFGMR3InsertNode(pConfig,       "EmulatedUSB", &pEUSB);
    CFGMR3InsertString(pEUSB,         "Id", pThis->mszUuid);
    CFGMR3InsertInteger(pEUSB,        "pfnCallback", (uintptr_t)EmulatedUSB::i_eusbCallback);
    CFGMR3InsertInteger(pEUSB,        "pvCallback", (uintptr_t)pThis->mpEmulatedUSB);

    PCFGMNODE pLunL0;
    CFGMR3InsertNode(pInstance,   "LUN#0", &pLunL0);
    CFGMR3InsertString(pLunL0,      "Driver", pszDriver);
    CFGMR3InsertNode(pLunL0,        "Config", &pConfig);
    CFGMR3InsertString(pConfig,       "DevicePath", pThis->mPath.c_str());
    CFGMR3InsertInteger(pConfig,      "Object", (uintptr_t)pThis->mpvObject);
    rc = emulatedWebcamInsertSettings(pConfig, &pThis->mDrvSettings);
    if (RT_FAILURE(rc))
        return rc;

    /* pInstance will be used by PDM and deallocated on error. */
    rc = PDMR3UsbCreateEmulatedDevice(pUVM, "Webcam", pInstance, &pThis->mUuid, NULL);
    LogRelFlowFunc(("PDMR3UsbCreateEmulatedDevice %Rrc\n", rc));
    return rc;
}

/* static */ DECLCALLBACK(int) EUSBWEBCAM::emulatedWebcamDetach(PUVM pUVM, EUSBWEBCAM *pThis)
{
    return PDMR3UsbDetachDevice(pUVM, &pThis->mUuid);
}

HRESULT EUSBWEBCAM::Initialize(Console *pConsole,
                               EmulatedUSB *pEmulatedUSB,
                               const com::Utf8Str *aPath,
                               const com::Utf8Str *aSettings,
                               void *pvObject)
{
    HRESULT hrc = S_OK;

    int vrc = RTUuidCreate(&mUuid);
    if (RT_SUCCESS(vrc))
    {
        RTStrPrintf(mszUuid, sizeof(mszUuid), "%RTuuid", &mUuid);
        hrc = mPath.assignEx(*aPath);
        if (SUCCEEDED(hrc))
        {
            hrc = mSettings.assignEx(*aSettings);
        }

        if (SUCCEEDED(hrc))
        {
            hrc = settingsParse();

            if (SUCCEEDED(hrc))
            {
                mpEmulatedUSB = pEmulatedUSB;
                mpvObject = pvObject;
            }
        }
    }

    if (SUCCEEDED(hrc) && RT_FAILURE(vrc))
    {
        LogFlowThisFunc(("%Rrc\n", vrc));
        hrc = pConsole->setError(VBOX_E_IPRT_ERROR,
                                 "Init emulated USB webcam (%Rrc)", vrc);
    }

    return hrc;
}

HRESULT EUSBWEBCAM::settingsParse(void)
{
    HRESULT hr = S_OK;

    /* Parse mSettings string:
     * "[dev:|drv:]Name1=Value1;[dev:|drv:]Name2=Value2"
     */
    char *pszSrc = mSettings.mutableRaw();

    if (pszSrc)
    {
        while (*pszSrc)
        {
            /* Does the setting belong to device of driver. Default is both. */
            bool fDev = true;
            bool fDrv = true;
            if (RTStrNICmp(pszSrc, RT_STR_TUPLE("drv:")) == 0)
            {
                pszSrc += sizeof("drv:")-1;
                fDev = false;
            }
            else if (RTStrNICmp(pszSrc, RT_STR_TUPLE("dev:")) == 0)
            {
                pszSrc += sizeof("dev:")-1;
                fDrv = false;
            }

            char *pszEq = RTStrStr(pszSrc, "=");
            if (!pszEq)
            {
                hr = E_INVALIDARG;
                break;
            }

            char *pszEnd = RTStrStr(pszEq, ";");
            if (!pszEnd)
            {
                pszEnd = pszEq + strlen(pszEq);
            }

            *pszEq = 0;
            char chEnd = *pszEnd;
            *pszEnd = 0;

            /* Empty strings not allowed. */
            if (*pszSrc != 0 && pszEq[1] != 0)
            {
                if (fDev)
                {
                    mDevSettings[pszSrc] = &pszEq[1];
                }
                if (fDrv)
                {
                    mDrvSettings[pszSrc] = &pszEq[1];
                }
            }

            *pszEq = '=';
            *pszEnd = chEnd;

            pszSrc = pszEnd;
            if (*pszSrc == ';')
            {
                pszSrc++;
            }
        }

        if (SUCCEEDED(hr))
        {
            EUSBSettingsMap::const_iterator it;
            for (it = mDevSettings.begin(); it != mDevSettings.end(); ++it)
                LogRelFlowFunc(("[dev:%s] = [%s]\n", it->first.c_str(), it->second.c_str()));
            for (it = mDrvSettings.begin(); it != mDrvSettings.end(); ++it)
                LogRelFlowFunc(("[drv:%s] = [%s]\n", it->first.c_str(), it->second.c_str()));
        }
    }

    return hr;
}

HRESULT EUSBWEBCAM::Attach(Console *pConsole,
                           PUVM pUVM,
                           const char *pszDriver)
{
    HRESULT hrc = S_OK;

    int  vrc = VMR3ReqCallWaitU(pUVM, 0 /* idDstCpu (saved state, see #6232) */,
                                (PFNRT)emulatedWebcamAttach, 3,
                                pUVM, this, pszDriver);

    if (SUCCEEDED(hrc) && RT_FAILURE(vrc))
    {
        LogFlowThisFunc(("%Rrc\n", vrc));
        hrc = pConsole->setError(VBOX_E_IPRT_ERROR,
                                 "Attach emulated USB webcam (%Rrc)", vrc);
    }

    return hrc;
}

HRESULT EUSBWEBCAM::Detach(Console *pConsole,
                           PUVM pUVM)
{
    HRESULT hrc = S_OK;

    int vrc = VMR3ReqCallWaitU(pUVM, 0 /* idDstCpu (saved state, see #6232) */,
                              (PFNRT)emulatedWebcamDetach, 2,
                              pUVM, this);

    if (SUCCEEDED(hrc) && RT_FAILURE(vrc))
    {
        LogFlowThisFunc(("%Rrc\n", vrc));
        hrc = pConsole->setError(VBOX_E_IPRT_ERROR,
                                 "Detach emulated USB webcam (%Rrc)", vrc);
    }

    return hrc;
}


/*
 * EmulatedUSB implementation.
 */
DEFINE_EMPTY_CTOR_DTOR(EmulatedUSB)

HRESULT EmulatedUSB::FinalConstruct()
{
    return BaseFinalConstruct();
}

void EmulatedUSB::FinalRelease()
{
    uninit();

    BaseFinalRelease();
}

/*
 * Initializes the instance.
 *
 * @param pConsole   The owner.
 */
HRESULT EmulatedUSB::init(ComObjPtr<Console> pConsole)
{
    LogFlowThisFunc(("\n"));

    ComAssertRet(!pConsole.isNull(), E_INVALIDARG);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    m.pConsole = pConsole;

    /* Confirm a successful initialization */
    autoInitSpan.setSucceeded();

    return S_OK;
}

/*
 * Uninitializes the instance.
 * Called either from FinalRelease() or by the parent when it gets destroyed.
 */
void EmulatedUSB::uninit()
{
    LogFlowThisFunc(("\n"));

    m.pConsole.setNull();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    for (WebcamsMap::iterator it = m.webcams.begin(); it != m.webcams.end(); ++it)
    {
        EUSBWEBCAM *p = it->second;
        if (p)
        {
            it->second = NULL;
            p->Release();
        }
    }
    m.webcams.clear();
    alock.release();

    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;
}

HRESULT EmulatedUSB::getWebcams(std::vector<com::Utf8Str> &aWebcams)
{
    HRESULT hrc = S_OK;

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    try
    {
        aWebcams.resize(m.webcams.size());
    }
    catch (std::bad_alloc &)
    {
        hrc = E_OUTOFMEMORY;
    }
    catch (...)
    {
        hrc = E_FAIL;
    }

    if (SUCCEEDED(hrc))
    {
        size_t i;
        WebcamsMap::const_iterator it;
        for (i = 0, it = m.webcams.begin(); it != m.webcams.end(); ++it)
            aWebcams[i++] = it->first;
    }

    return hrc;
}

static const Utf8Str s_pathDefault(".0");

HRESULT EmulatedUSB::webcamAttach(const com::Utf8Str &aPath,
                                  const com::Utf8Str &aSettings)
{
    return i_webcamAttachInternal(aPath, aSettings, "HostWebcam", NULL);
}

HRESULT EmulatedUSB::i_webcamAttachInternal(const com::Utf8Str &aPath,
                                            const com::Utf8Str &aSettings,
                                            const char *pszDriver,
                                            void *pvObject)
{
    HRESULT hrc = S_OK;

    const Utf8Str &path = aPath.isEmpty() || aPath == "."? s_pathDefault: aPath;

    Console::SafeVMPtr ptrVM(m.pConsole);
    if (ptrVM.isOk())
    {
        EUSBWEBCAM *p = new EUSBWEBCAM();
        if (p)
        {
            hrc = p->Initialize(m.pConsole, this, &path, &aSettings, pvObject);
            if (SUCCEEDED(hrc))
            {
                AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
                WebcamsMap::const_iterator it = m.webcams.find(path);
                if (it == m.webcams.end())
                {
                    p->AddRef();
                    try
                    {
                        m.webcams[path] = p;
                    }
                    catch (std::bad_alloc &)
                    {
                        hrc = E_OUTOFMEMORY;
                    }
                    catch (...)
                    {
                        hrc = E_FAIL;
                    }
                    p->enmStatus = EUSBDEVICE_ATTACHING;
                }
                else
                {
                    hrc = E_FAIL;
                }
            }

            if (SUCCEEDED(hrc))
            {
                hrc = p->Attach(m.pConsole, ptrVM.rawUVM(), pszDriver);
            }

            AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
            if (SUCCEEDED(hrc))
            {
                p->enmStatus = EUSBDEVICE_ATTACHED;
            }
            else
            {
                if (p->enmStatus != EUSBDEVICE_CREATED)
                {
                    m.webcams.erase(path);
                }
            }
            alock.release();

            p->Release();
        }
        else
        {
            hrc = E_OUTOFMEMORY;
        }
    }
    else
    {
        hrc = VBOX_E_INVALID_VM_STATE;
    }

    return hrc;
}

HRESULT EmulatedUSB::webcamDetach(const com::Utf8Str &aPath)
{
    return i_webcamDetachInternal(aPath);
}

HRESULT EmulatedUSB::i_webcamDetachInternal(const com::Utf8Str &aPath)
{
    HRESULT hrc = S_OK;

    const Utf8Str &path = aPath.isEmpty() || aPath == "."? s_pathDefault: aPath;

    Console::SafeVMPtr ptrVM(m.pConsole);
    if (ptrVM.isOk())
    {
        EUSBWEBCAM *p = NULL;

        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        WebcamsMap::iterator it = m.webcams.find(path);
        if (it != m.webcams.end())
        {
            if (it->second->enmStatus == EUSBDEVICE_ATTACHED)
            {
                p = it->second;
                m.webcams.erase(it);
            }
        }
        alock.release();

        if (p)
        {
            hrc = p->Detach(m.pConsole, ptrVM.rawUVM());
            p->Release();
        }
        else
        {
            hrc = E_INVALIDARG;
        }
    }
    else
    {
        hrc = VBOX_E_INVALID_VM_STATE;
    }

    return hrc;
}

/* static */ DECLCALLBACK(int) EmulatedUSB::eusbCallbackEMT(EmulatedUSB *pThis, char *pszId, uint32_t iEvent,
                                                            void *pvData, uint32_t cbData)
{
    LogRelFlowFunc(("id %s event %d, data %p %d\n", pszId, iEvent, pvData, cbData));

    NOREF(cbData);

    int rc = VINF_SUCCESS;
    if (iEvent == 0)
    {
        com::Utf8Str path;
        HRESULT hr = pThis->webcamPathFromId(&path, pszId);
        if (SUCCEEDED(hr))
        {
            hr = pThis->webcamDetach(path);
            if (FAILED(hr))
            {
                rc = VERR_INVALID_STATE;
            }
        }
        else
        {
            rc = VERR_NOT_FOUND;
        }
    }
    else
    {
        rc = VERR_INVALID_PARAMETER;
    }

    RTMemFree(pszId);
    RTMemFree(pvData);

    LogRelFlowFunc(("rc %Rrc\n", rc));
    return rc;
}

/* static */ DECLCALLBACK(int) EmulatedUSB::i_eusbCallback(void *pv, const char *pszId, uint32_t iEvent,
                                                           const void *pvData, uint32_t cbData)
{
    /* Make a copy of parameters, forward to EMT and leave the callback to not hold any lock in the device. */
    int rc = VINF_SUCCESS;

    void *pvIdCopy = NULL;
    void *pvDataCopy = NULL;
    if (cbData > 0)
    {
       pvDataCopy = RTMemDup(pvData, cbData);
       if (!pvDataCopy)
       {
           rc = VERR_NO_MEMORY;
       }
    }

    if (RT_SUCCESS(rc))
    {
        pvIdCopy = RTMemDup(pszId, strlen(pszId) + 1);
        if (!pvIdCopy)
        {
            rc = VERR_NO_MEMORY;
        }
    }

    if (RT_SUCCESS(rc))
    {
        EmulatedUSB *pThis = (EmulatedUSB *)pv;
        Console::SafeVMPtr ptrVM(pThis->m.pConsole);
        if (ptrVM.isOk())
        {
            /* No wait. */
            rc = VMR3ReqCallNoWaitU(ptrVM.rawUVM(), 0 /* idDstCpu */,
                                    (PFNRT)EmulatedUSB::eusbCallbackEMT, 5,
                                    pThis, pvIdCopy, iEvent, pvDataCopy, cbData);
        }
        else
        {
            rc = VERR_INVALID_STATE;
        }
    }

    if (RT_FAILURE(rc))
    {
        RTMemFree(pvIdCopy);
        RTMemFree(pvDataCopy);
    }

    return rc;
}

HRESULT EmulatedUSB::webcamPathFromId(com::Utf8Str *pPath, const char *pszId)
{
    HRESULT hr = S_OK;

    Console::SafeVMPtr ptrVM(m.pConsole);
    if (ptrVM.isOk())
    {
        AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
        WebcamsMap::const_iterator it;
        for (it = m.webcams.begin(); it != m.webcams.end(); ++it)
        {
            EUSBWEBCAM *p = it->second;
            if (p->HasId(pszId))
            {
                *pPath = it->first;
                break;
            }
        }

        if (it == m.webcams.end())
        {
            hr = E_FAIL;
        }
        alock.release();
    }
    else
    {
        hr = VBOX_E_INVALID_VM_STATE;
    }

    return hr;
}

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
