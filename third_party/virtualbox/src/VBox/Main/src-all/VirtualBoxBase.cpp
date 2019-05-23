/* $Id: VirtualBoxBase.cpp $ */

/** @file
 *
 * VirtualBox COM base classes implementation
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

#include <iprt/semaphore.h>
#include <iprt/asm.h>
#include <iprt/cpp/exception.h>

#include <typeinfo>

#if !defined(VBOX_WITH_XPCOM)
#include <iprt/win/windows.h>
#include <dbghelp.h>
#else /* !defined(VBOX_WITH_XPCOM) */
/// @todo remove when VirtualBoxErrorInfo goes away from here
#include <nsIServiceManager.h>
#include <nsIExceptionService.h>
#endif /* !defined(VBOX_WITH_XPCOM) */

#include "VirtualBoxBase.h"
#include "AutoCaller.h"
#include "VirtualBoxErrorInfoImpl.h"
#include "Logging.h"
#include "Global.h"

#include "VBox/com/ErrorInfo.h"
#include "VBox/com/MultiResult.h"

////////////////////////////////////////////////////////////////////////////////
//
// VirtualBoxBase
//
////////////////////////////////////////////////////////////////////////////////

CLASSFACTORY_STAT g_aClassFactoryStats[CLASSFACTORYSTATS_MAX] =
{
    { "--- totals ---", 0 },
    { NULL, 0 }
};

RWLockHandle *g_pClassFactoryStatsLock = NULL;


VirtualBoxBase::VirtualBoxBase() :
    mState(this),
    iFactoryStat(~0U)
{
    mObjectLock = NULL;

    if (!g_pClassFactoryStatsLock)
    {
        RWLockHandle *lock = new RWLockHandle(LOCKCLASS_OBJECTSTATE);
        if (!ASMAtomicCmpXchgPtr(&g_pClassFactoryStatsLock, lock, NULL))
            delete lock;
    }
    Assert(g_pClassFactoryStatsLock);
}

VirtualBoxBase::~VirtualBoxBase()
{
    Assert(iFactoryStat == ~0U);
    if (mObjectLock)
        delete mObjectLock;
}

HRESULT VirtualBoxBase::BaseFinalConstruct()
{
    Assert(iFactoryStat == ~0U);
    if (g_pClassFactoryStatsLock)
    {
        AutoWriteLock alock(g_pClassFactoryStatsLock COMMA_LOCKVAL_SRC_POS);
        g_aClassFactoryStats[0].current++;
        g_aClassFactoryStats[0].overall++;
        const char *pszName = getComponentName();
        uint32_t i = 1;
        while (i < CLASSFACTORYSTATS_MAX && g_aClassFactoryStats[i].psz)
        {
            if (g_aClassFactoryStats[i].psz == pszName)
                break;
            i++;
        }
        if (i < CLASSFACTORYSTATS_MAX)
        {
            if (!g_aClassFactoryStats[i].psz)
            {
                g_aClassFactoryStats[i].psz = pszName;
                g_aClassFactoryStats[i].current = 0;
                g_aClassFactoryStats[i].overall = 0;
            }
            iFactoryStat = i;
            g_aClassFactoryStats[i].current++;
            g_aClassFactoryStats[i].overall++;
        }
        else
            AssertMsg(i < CLASSFACTORYSTATS_MAX, ("%u exhausts size of factory housekeeping array\n", i));
    }
    else
        Assert(g_pClassFactoryStatsLock);

#ifdef RT_OS_WINDOWS
    return CoCreateFreeThreadedMarshaler(this, //GetControllingUnknown(),
                                         m_pUnkMarshaler.asOutParam());
#else
    return S_OK;
#endif
}

void VirtualBoxBase::BaseFinalRelease()
{
    if (g_pClassFactoryStatsLock)
    {
        AutoWriteLock alock(g_pClassFactoryStatsLock COMMA_LOCKVAL_SRC_POS);
        g_aClassFactoryStats[0].current--;
        const char *pszName = getComponentName();
        if (iFactoryStat < CLASSFACTORYSTATS_MAX)
        {
            if (g_aClassFactoryStats[iFactoryStat].psz == pszName)
            {
                g_aClassFactoryStats[iFactoryStat].current--;
                iFactoryStat = ~0U;
            }
            else
                AssertMsgFailed(("could not find factory housekeeping array entry for %s (index %u contains %s)\n", pszName, iFactoryStat, g_aClassFactoryStats[iFactoryStat].psz));
        }
        else
            AssertMsgFailed(("factory housekeeping array corruption, index %u is too large\n", iFactoryStat));
    }
    else
        Assert(g_pClassFactoryStatsLock);

#ifdef RT_OS_WINDOWS
     m_pUnkMarshaler.setNull();
#endif
}

void APIDumpComponentFactoryStats()
{
    if (g_pClassFactoryStatsLock)
    {
        AutoReadLock alock(g_pClassFactoryStatsLock COMMA_LOCKVAL_SRC_POS);
        for (uint32_t i = 0; i < CLASSFACTORYSTATS_MAX && g_aClassFactoryStats[i].psz; i++)
            LogRel(("CFS: component %-30s current %-10u total %-10u\n",
                    g_aClassFactoryStats[i].psz, g_aClassFactoryStats[i].current,
                    g_aClassFactoryStats[i].overall));
    }
    else
        Assert(g_pClassFactoryStatsLock);
}

/**
 * This virtual method returns an RWLockHandle that can be used to
 * protect instance data. This RWLockHandle is generally referred to
 * as the "object lock"; its locking class (for lock order validation)
 * must be returned by another virtual method, getLockingClass(), which
 * by default returns LOCKCLASS_OTHEROBJECT but is overridden by several
 * subclasses such as VirtualBox, Host, Machine and others.
 *
 * On the first call this method lazily creates the RWLockHandle.
 *
 * @return
 */
/* virtual */
RWLockHandle *VirtualBoxBase::lockHandle() const
{
    /* lazy initialization */
    if (RT_LIKELY(mObjectLock))
        return mObjectLock;

    AssertCompile(sizeof(RWLockHandle *) == sizeof(void *));

    // getLockingClass() is overridden by many subclasses to return
    // one of the locking classes listed at the top of AutoLock.h
    RWLockHandle *objLock = new RWLockHandle(getLockingClass());
    if (!ASMAtomicCmpXchgPtr(&mObjectLock, objLock, NULL))
    {
        delete objLock;
        objLock = ASMAtomicReadPtrT(&mObjectLock, RWLockHandle *);
    }
    return objLock;
}

/**
 * Handles unexpected exceptions by turning them into COM errors in release
 * builds or by hitting a breakpoint in the release builds.
 *
 * Usage pattern:
 * @code
        try
        {
            // ...
        }
        catch (LaLalA)
        {
            // ...
        }
        catch (...)
        {
            rc = VirtualBox::handleUnexpectedExceptions(this, RT_SRC_POS);
        }
 * @endcode
 *
 * @param aThis     object where the exception happened
 * @param SRC_POS   "RT_SRC_POS" macro instantiation.
 *  */
/* static */
HRESULT VirtualBoxBase::handleUnexpectedExceptions(VirtualBoxBase *const aThis, RT_SRC_POS_DECL)
{
    try
    {
        /* re-throw the current exception */
        throw;
    }
    catch (const RTCError &err)      // includes all XML exceptions
    {
        return setErrorInternal(E_FAIL, aThis->getClassIID(), aThis->getComponentName(),
                                Utf8StrFmt(tr("%s.\n%s[%d] (%s)"),
                                           err.what(),
                                           pszFile, iLine, pszFunction).c_str(),
                                false /* aWarning */,
                                true /* aLogIt */);
    }
    catch (const std::exception &err)
    {
        return setErrorInternal(E_FAIL, aThis->getClassIID(), aThis->getComponentName(),
                                Utf8StrFmt(tr("Unexpected exception: %s [%s]\n%s[%d] (%s)"),
                                           err.what(), typeid(err).name(),
                                           pszFile, iLine, pszFunction).c_str(),
                                false /* aWarning */,
                                true /* aLogIt */);
    }
    catch (...)
    {
        return setErrorInternal(E_FAIL, aThis->getClassIID(), aThis->getComponentName(),
                                Utf8StrFmt(tr("Unknown exception\n%s[%d] (%s)"),
                                           pszFile, iLine, pszFunction).c_str(),
                                false /* aWarning */,
                                true /* aLogIt */);
    }

#ifndef _MSC_VER /* (unreachable) */
    /* should not get here */
    AssertFailed();
    return E_FAIL;
#endif
}

/**
 *  Sets error info for the current thread. This is an internal function that
 *  gets eventually called by all public variants.  If @a aWarning is
 *  @c true, then the highest (31) bit in the @a aResultCode value which
 *  indicates the error severity is reset to zero to make sure the receiver will
 *  recognize that the created error info object represents a warning rather
 *  than an error.
 *
 *  @param  aResultCode
 *  @param  aIID
 *  @param  pcszComponent
 *  @param  aText
 *  @param  aWarning
 *  @param  aLogIt
 *  @param  aResultDetail
 */
/* static */
HRESULT VirtualBoxBase::setErrorInternal(HRESULT aResultCode,
                                         const GUID &aIID,
                                         const char *pcszComponent,
                                         Utf8Str aText,
                                         bool aWarning,
                                         bool aLogIt,
                                         LONG aResultDetail /* = 0*/)
{
    /* whether multi-error mode is turned on */
    bool preserve = MultiResult::isMultiEnabled();

    if (aLogIt)
        LogRel(("%s [COM]: aRC=%Rhrc (%#08x) aIID={%RTuuid} aComponent={%s} aText={%s}, preserve=%RTbool aResultDetail=%d\n",
                aWarning ? "WARNING" : "ERROR",
                aResultCode,
                aResultCode,
                &aIID,
                pcszComponent,
                aText.c_str(),
                preserve,
                aResultDetail));

    /* these are mandatory, others -- not */
    AssertReturn((!aWarning && FAILED(aResultCode)) ||
                  (aWarning && aResultCode != S_OK),
                  E_FAIL);

    /* reset the error severity bit if it's a warning */
    if (aWarning)
        aResultCode &= ~0x80000000;

    HRESULT rc = S_OK;

    if (aText.isEmpty())
    {
        /* Some default info */
        switch (aResultCode)
        {
            case E_INVALIDARG:                 aText = "A parameter has an invalid value"; break;
            case E_POINTER:                    aText = "A parameter is an invalid pointer"; break;
            case E_UNEXPECTED:                 aText = "The result of the operation is unexpected"; break;
            case E_ACCESSDENIED:               aText = "The access to an object is not allowed"; break;
            case E_OUTOFMEMORY:                aText = "The allocation of new memory failed"; break;
            case E_NOTIMPL:                    aText = "The requested operation is not implemented"; break;
            case E_NOINTERFACE:                aText = "The requested interface is not implemented"; break;
            case E_FAIL:                       aText = "A general error occurred"; break;
            case E_ABORT:                      aText = "The operation was canceled"; break;
            case VBOX_E_OBJECT_NOT_FOUND:      aText = "Object corresponding to the supplied arguments does not exist"; break;
            case VBOX_E_INVALID_VM_STATE:      aText = "Current virtual machine state prevents the operation"; break;
            case VBOX_E_VM_ERROR:              aText = "Virtual machine error occurred attempting the operation"; break;
            case VBOX_E_FILE_ERROR:            aText = "File not accessible or erroneous file contents"; break;
            case VBOX_E_IPRT_ERROR:            aText = "Runtime subsystem error"; break;
            case VBOX_E_PDM_ERROR:             aText = "Pluggable Device Manager error"; break;
            case VBOX_E_INVALID_OBJECT_STATE:  aText = "Current object state prohibits operation"; break;
            case VBOX_E_HOST_ERROR:            aText = "Host operating system related error"; break;
            case VBOX_E_NOT_SUPPORTED:         aText = "Requested operation is not supported"; break;
            case VBOX_E_XML_ERROR:             aText = "Invalid XML found"; break;
            case VBOX_E_INVALID_SESSION_STATE: aText = "Current session state prohibits operation"; break;
            case VBOX_E_OBJECT_IN_USE:         aText = "Object being in use prohibits operation"; break;
            default:                           aText = "Unknown error"; break;
        }
    }

    do
    {
        ComObjPtr<VirtualBoxErrorInfo> info;
        rc = info.createObject();
        if (FAILED(rc)) break;

#if !defined(VBOX_WITH_XPCOM)

        ComPtr<IVirtualBoxErrorInfo> curInfo;
        if (preserve)
        {
            /* get the current error info if any */
            ComPtr<IErrorInfo> err;
            rc = ::GetErrorInfo(0, err.asOutParam());
            if (FAILED(rc)) break;
            rc = err.queryInterfaceTo(curInfo.asOutParam());
            if (FAILED(rc))
            {
                /* create a IVirtualBoxErrorInfo wrapper for the native
                 * IErrorInfo object */
                ComObjPtr<VirtualBoxErrorInfo> wrapper;
                rc = wrapper.createObject();
                if (SUCCEEDED(rc))
                {
                    rc = wrapper->init(err);
                    if (SUCCEEDED(rc))
                        curInfo = wrapper;
                }
            }
        }
        /* On failure, curInfo will stay null */
        Assert(SUCCEEDED(rc) || curInfo.isNull());

        /* set the current error info and preserve the previous one if any */
        rc = info->initEx(aResultCode, aResultDetail, aIID, pcszComponent, aText, curInfo);
        if (FAILED(rc)) break;

        ComPtr<IErrorInfo> err;
        rc = info.queryInterfaceTo(err.asOutParam());
        if (SUCCEEDED(rc))
            rc = ::SetErrorInfo(0, err);

#else // !defined(VBOX_WITH_XPCOM)

        nsCOMPtr <nsIExceptionService> es;
        es = do_GetService(NS_EXCEPTIONSERVICE_CONTRACTID, &rc);
        if (NS_SUCCEEDED(rc))
        {
            nsCOMPtr <nsIExceptionManager> em;
            rc = es->GetCurrentExceptionManager(getter_AddRefs(em));
            if (FAILED(rc)) break;

            ComPtr<IVirtualBoxErrorInfo> curInfo;
            if (preserve)
            {
                /* get the current error info if any */
                ComPtr<nsIException> ex;
                rc = em->GetCurrentException(ex.asOutParam());
                if (FAILED(rc)) break;
                rc = ex.queryInterfaceTo(curInfo.asOutParam());
                if (FAILED(rc))
                {
                    /* create a IVirtualBoxErrorInfo wrapper for the native
                     * nsIException object */
                    ComObjPtr<VirtualBoxErrorInfo> wrapper;
                    rc = wrapper.createObject();
                    if (SUCCEEDED(rc))
                    {
                        rc = wrapper->init(ex);
                        if (SUCCEEDED(rc))
                            curInfo = wrapper;
                    }
                }
            }
            /* On failure, curInfo will stay null */
            Assert(SUCCEEDED(rc) || curInfo.isNull());

            /* set the current error info and preserve the previous one if any */
            rc = info->initEx(aResultCode, aResultDetail, aIID, pcszComponent, Bstr(aText), curInfo);
            if (FAILED(rc)) break;

            ComPtr<nsIException> ex;
            rc = info.queryInterfaceTo(ex.asOutParam());
            if (SUCCEEDED(rc))
                rc = em->SetCurrentException(ex);
        }
        else if (rc == NS_ERROR_UNEXPECTED)
        {
            /*
             *  It is possible that setError() is being called by the object
             *  after the XPCOM shutdown sequence has been initiated
             *  (for example, when XPCOM releases all instances it internally
             *  references, which can cause object's FinalConstruct() and then
             *  uninit()). In this case, do_GetService() above will return
             *  NS_ERROR_UNEXPECTED and it doesn't actually make sense to
             *  set the exception (nobody will be able to read it).
             */
            Log1WarningFunc(("Will not set an exception because nsIExceptionService is not available (NS_ERROR_UNEXPECTED). XPCOM is being shutdown?\n"));
            rc = NS_OK;
        }

#endif // !defined(VBOX_WITH_XPCOM)
    }
    while (0);

    AssertComRC(rc);

    return SUCCEEDED(rc) ? aResultCode : rc;
}

/**
 * Shortcut instance method to calling the static setErrorInternal with the
 * class interface ID and component name inserted correctly. This uses the
 * virtual getClassIID() and getComponentName() methods which are automatically
 * defined by the VIRTUALBOXBASE_ADD_ERRORINFO_SUPPORT macro.
 * @param   aResultCode
 * @return
 */
HRESULT VirtualBoxBase::setError(HRESULT aResultCode)
{
    return setErrorInternal(aResultCode,
                            this->getClassIID(),
                            this->getComponentName(),
                            "",
                            false /* aWarning */,
                            true /* aLogIt */);
}

/**
 * Shortcut instance method to calling the static setErrorInternal with the
 * class interface ID and component name inserted correctly. This uses the
 * virtual getClassIID() and getComponentName() methods which are automatically
 * defined by the VIRTUALBOXBASE_ADD_ERRORINFO_SUPPORT macro.
 * @param   aResultCode
 * @param   pcsz
 * @return
 */
HRESULT VirtualBoxBase::setError(HRESULT aResultCode, const char *pcsz, ...)
{
    va_list args;
    va_start(args, pcsz);
    HRESULT rc = setErrorInternal(aResultCode,
                                  this->getClassIID(),
                                  this->getComponentName(),
                                  Utf8Str(pcsz, args),
                                  false /* aWarning */,
                                  true /* aLogIt */);
    va_end(args);
    return rc;
}

/**
 * Shortcut instance method to calling the static setErrorInternal with the
 * class interface ID and component name inserted correctly. This uses the
 * virtual getClassIID() and getComponentName() methods which are automatically
 * defined by the VIRTUALBOXBASE_ADD_ERRORINFO_SUPPORT macro.
 * @param   ei
 * @return
 */
HRESULT VirtualBoxBase::setError(const com::ErrorInfo &ei)
{
    /* whether multi-error mode is turned on */
    bool preserve = MultiResult::isMultiEnabled();

    HRESULT rc = S_OK;

    do
    {
        ComObjPtr<VirtualBoxErrorInfo> info;
        rc = info.createObject();
        if (FAILED(rc)) break;

#if !defined(VBOX_WITH_XPCOM)

        ComPtr<IVirtualBoxErrorInfo> curInfo;
        if (preserve)
        {
            /* get the current error info if any */
            ComPtr<IErrorInfo> err;
            rc = ::GetErrorInfo(0, err.asOutParam());
            if (FAILED(rc)) break;
            rc = err.queryInterfaceTo(curInfo.asOutParam());
            if (FAILED(rc))
            {
                /* create a IVirtualBoxErrorInfo wrapper for the native
                 * IErrorInfo object */
                ComObjPtr<VirtualBoxErrorInfo> wrapper;
                rc = wrapper.createObject();
                if (SUCCEEDED(rc))
                {
                    rc = wrapper->init(err);
                    if (SUCCEEDED(rc))
                        curInfo = wrapper;
                }
            }
        }
        /* On failure, curInfo will stay null */
        Assert(SUCCEEDED(rc) || curInfo.isNull());

        /* set the current error info and preserve the previous one if any */
        rc = info->init(ei, curInfo);
        if (FAILED(rc)) break;

        ComPtr<IErrorInfo> err;
        rc = info.queryInterfaceTo(err.asOutParam());
        if (SUCCEEDED(rc))
            rc = ::SetErrorInfo(0, err);

#else // !defined(VBOX_WITH_XPCOM)

        nsCOMPtr <nsIExceptionService> es;
        es = do_GetService(NS_EXCEPTIONSERVICE_CONTRACTID, &rc);
        if (NS_SUCCEEDED(rc))
        {
            nsCOMPtr <nsIExceptionManager> em;
            rc = es->GetCurrentExceptionManager(getter_AddRefs(em));
            if (FAILED(rc)) break;

            ComPtr<IVirtualBoxErrorInfo> curInfo;
            if (preserve)
            {
                /* get the current error info if any */
                ComPtr<nsIException> ex;
                rc = em->GetCurrentException(ex.asOutParam());
                if (FAILED(rc)) break;
                rc = ex.queryInterfaceTo(curInfo.asOutParam());
                if (FAILED(rc))
                {
                    /* create a IVirtualBoxErrorInfo wrapper for the native
                     * nsIException object */
                    ComObjPtr<VirtualBoxErrorInfo> wrapper;
                    rc = wrapper.createObject();
                    if (SUCCEEDED(rc))
                    {
                        rc = wrapper->init(ex);
                        if (SUCCEEDED(rc))
                            curInfo = wrapper;
                    }
                }
            }
            /* On failure, curInfo will stay null */
            Assert(SUCCEEDED(rc) || curInfo.isNull());

            /* set the current error info and preserve the previous one if any */
            rc = info->init(ei, curInfo);
            if (FAILED(rc)) break;

            ComPtr<nsIException> ex;
            rc = info.queryInterfaceTo(ex.asOutParam());
            if (SUCCEEDED(rc))
                rc = em->SetCurrentException(ex);
        }
        else if (rc == NS_ERROR_UNEXPECTED)
        {
            /*
             *  It is possible that setError() is being called by the object
             *  after the XPCOM shutdown sequence has been initiated
             *  (for example, when XPCOM releases all instances it internally
             *  references, which can cause object's FinalConstruct() and then
             *  uninit()). In this case, do_GetService() above will return
             *  NS_ERROR_UNEXPECTED and it doesn't actually make sense to
             *  set the exception (nobody will be able to read it).
             */
            Log1WarningFunc(("Will not set an exception because nsIExceptionService is not available (NS_ERROR_UNEXPECTED). XPCOM is being shutdown?\n"));
            rc = NS_OK;
        }

#endif // !defined(VBOX_WITH_XPCOM)
    }
    while (0);

    AssertComRC(rc);

    return SUCCEEDED(rc) ? ei.getResultCode() : rc;
}

/**
 * Converts the VBox status code a COM one and sets the error info.
 *
 * The VBox status code is made available to the API user via
 * IVirtualBoxErrorInfo::resultDetail attribute.
 *
 * @param   vrc             The VBox status code.
 * @return  COM status code appropriate for @a vrc.
 *
 * @sa      VirtualBoxBase::setError(HRESULT)
 */
HRESULT VirtualBoxBase::setErrorVrc(int vrc)
{
    return setErrorInternal(Global::vboxStatusCodeToCOM(vrc),
                            this->getClassIID(),
                            this->getComponentName(),
                            Utf8StrFmt("%Rrc", vrc),
                            false /* aWarning */,
                            true /* aLogIt */,
                            vrc /* aResultDetail */);
}

/**
 * Converts the VBox status code a COM one and sets the error info.
 *
 * @param   vrc             The VBox status code.
 * @param   pcszMsgFmt      Error message format string.
 * @param   ...             Argument specified in the @a pcszMsgFmt
 * @return  COM status code appropriate for @a vrc.
 *
 * @sa      VirtualBoxBase::setError(HRESULT, const char *, ...)
 */
HRESULT VirtualBoxBase::setErrorVrc(int vrc, const char *pcszMsgFmt, ...)
{
    va_list va;
    va_start(va, pcszMsgFmt);
    HRESULT hrc = setErrorInternal(Global::vboxStatusCodeToCOM(vrc),
                                   this->getClassIID(),
                                   this->getComponentName(),
                                   Utf8Str(pcszMsgFmt, va),
                                   false /* aWarning */,
                                   true /* aLogIt */,
                                   vrc /* aResultDetail */);
    va_end(va);
    return hrc;
}

/**
 * Sets error info with both a COM status and an VBox status code.
 *
 * The VBox status code is made available to the API user via
 * IVirtualBoxErrorInfo::resultDetail attribute.
 *
 * @param   hrc             The COM status code to return.
 * @param   vrc             The VBox status code.
 * @return  Most likely @a hrc, see setErrorInternal.
 *
 * @sa      VirtualBoxBase::setError(HRESULT)
 */
HRESULT VirtualBoxBase::setErrorBoth(HRESULT hrc, int vrc)
{
    return setErrorInternal(hrc,
                            this->getClassIID(),
                            this->getComponentName(),
                            Utf8StrFmt("%Rrc", vrc),
                            false /* aWarning */,
                            true /* aLogIt */,
                            vrc /* aResultDetail */);
}

/**
 * Sets error info with a message and both a COM status and an VBox status code.
 *
 * The VBox status code is made available to the API user via
 * IVirtualBoxErrorInfo::resultDetail attribute.
 *
 * @param   hrc             The COM status code to return.
 * @param   vrc             The VBox status code.
 * @param   pcszMsgFmt      Error message format string.
 * @param   ...             Argument specified in the @a pcszMsgFmt
 * @return  Most likely @a hrc, see setErrorInternal.
 *
 * @sa      VirtualBoxBase::setError(HRESULT, const char *, ...)
 */
HRESULT VirtualBoxBase::setErrorBoth(HRESULT hrc, int vrc, const char *pcszMsgFmt, ...)
{
    va_list va;
    va_start(va, pcszMsgFmt);
    hrc = setErrorInternal(hrc,
                           this->getClassIID(),
                           this->getComponentName(),
                           Utf8Str(pcszMsgFmt, va),
                           false /* aWarning */,
                           true /* aLogIt */,
                           vrc /* aResultDetail */);
    va_end(va);
    return hrc;
}

/**
 * Like setError(), but sets the "warning" bit in the call to setErrorInternal().
 * @param aResultCode
 * @param pcsz
 * @return
 */
HRESULT VirtualBoxBase::setWarning(HRESULT aResultCode, const char *pcsz, ...)
{
    va_list args;
    va_start(args, pcsz);
    HRESULT rc = setErrorInternal(aResultCode,
                                  this->getClassIID(),
                                  this->getComponentName(),
                                  Utf8Str(pcsz, args),
                                  true /* aWarning */,
                                  true /* aLogIt */);
    va_end(args);
    return rc;
}

/**
 * Like setError(), but disables the "log" flag in the call to setErrorInternal().
 * @param aResultCode
 * @param pcsz
 * @return
 */
HRESULT VirtualBoxBase::setErrorNoLog(HRESULT aResultCode, const char *pcsz, ...)
{
    va_list args;
    va_start(args, pcsz);
    HRESULT rc = setErrorInternal(aResultCode,
                                  this->getClassIID(),
                                  this->getComponentName(),
                                  Utf8Str(pcsz, args),
                                  false /* aWarning */,
                                  false /* aLogIt */);
    va_end(args);
    return rc;
}

/**
 * Clear the current error information.
 */
/*static*/
void VirtualBoxBase::clearError(void)
{
#if !defined(VBOX_WITH_XPCOM)
    ::SetErrorInfo(0, NULL);
#else
    HRESULT rc = S_OK;
    nsCOMPtr <nsIExceptionService> es;
    es = do_GetService(NS_EXCEPTIONSERVICE_CONTRACTID, &rc);
    if (NS_SUCCEEDED(rc))
    {
        nsCOMPtr <nsIExceptionManager> em;
        rc = es->GetCurrentExceptionManager(getter_AddRefs(em));
        if (SUCCEEDED(rc))
            em->SetCurrentException(NULL);
    }
#endif
}


////////////////////////////////////////////////////////////////////////////////
//
// MultiResult methods
//
////////////////////////////////////////////////////////////////////////////////

RTTLS MultiResult::sCounter = NIL_RTTLS;

/*static*/
void MultiResult::incCounter()
{
    if (sCounter == NIL_RTTLS)
    {
        sCounter = RTTlsAlloc();
        AssertReturnVoid(sCounter != NIL_RTTLS);
    }

    uintptr_t counter = (uintptr_t)RTTlsGet(sCounter);
    ++counter;
    RTTlsSet(sCounter, (void*)counter);
}

/*static*/
void MultiResult::decCounter()
{
    uintptr_t counter = (uintptr_t)RTTlsGet(sCounter);
    AssertReturnVoid(counter != 0);
    --counter;
    RTTlsSet(sCounter, (void*)counter);
}

/*static*/
bool MultiResult::isMultiEnabled()
{
    if (sCounter == NIL_RTTLS)
       return false;

    return ((uintptr_t)RTTlsGet(MultiResult::sCounter)) > 0;
}

