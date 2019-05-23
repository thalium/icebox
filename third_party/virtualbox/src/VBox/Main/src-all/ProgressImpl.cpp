/* $Id: ProgressImpl.cpp $ */
/** @file
 *
 * VirtualBox Progress COM class implementation
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

#include <iprt/types.h>


#if defined(VBOX_WITH_XPCOM)
#include <nsIServiceManager.h>
#include <nsIExceptionService.h>
#include <nsCOMPtr.h>
#endif /* defined(VBOX_WITH_XPCOM) */

#include "ProgressImpl.h"

#if !defined(VBOX_COM_INPROC)
# include "VirtualBoxImpl.h"
#endif
#include "VirtualBoxErrorInfoImpl.h"

#include "Logging.h"

#include <iprt/time.h>
#include <iprt/semaphore.h>
#include <iprt/cpp/utils.h>

#include <VBox/err.h>
#include "AutoCaller.h"

#include "VBoxEvents.h"

Progress::Progress()
#if !defined(VBOX_COM_INPROC)
    : mParent(NULL)
#endif
{
}

Progress::~Progress()
{
}


HRESULT Progress::FinalConstruct()
{
    mCancelable = FALSE;
    mCompleted = FALSE;
    mCanceled = FALSE;
    mResultCode = S_OK;

    m_cOperations
        = m_ulTotalOperationsWeight
        = m_ulOperationsCompletedWeight
        = m_ulCurrentOperation
        = m_ulCurrentOperationWeight
        = m_ulOperationPercent
        = m_cMsTimeout
        = 0;

    // get creation timestamp
    m_ullTimestamp = RTTimeMilliTS();

    m_pfnCancelCallback = NULL;
    m_pvCancelUserArg = NULL;

    mCompletedSem = NIL_RTSEMEVENTMULTI;
    mWaitersCount = 0;

    return Progress::BaseFinalConstruct();
}

void Progress::FinalRelease()
{
    uninit();
    BaseFinalRelease();
}

// public initializer/uninitializer for internal purposes only
////////////////////////////////////////////////////////////////////////////////

/**
 * Initializes the normal progress object. With this variant, one can have
 * an arbitrary number of sub-operation which IProgress can analyze to
 * have a weighted progress computed.
 *
 * For example, say that one IProgress is supposed to track the cloning
 * of two hard disk images, which are 100 MB and 1000 MB in size, respectively,
 * and each of these hard disks should be one sub-operation of the IProgress.
 *
 * Obviously the progress would be misleading if the progress displayed 50%
 * after the smaller image was cloned and would then take much longer for
 * the second half.
 *
 * With weighted progress, one can invoke the following calls:
 *
 * 1) create progress object with cOperations = 2 and ulTotalOperationsWeight =
 *    1100 (100 MB plus 1100, but really the weights can be any ULONG); pass
 *    in ulFirstOperationWeight = 100 for the first sub-operation
 *
 * 2) Then keep calling setCurrentOperationProgress() with a percentage
 *    for the first image; the total progress will increase up to a value
 *    of 9% (100MB / 1100MB * 100%).
 *
 * 3) Then call setNextOperation with the second weight (1000 for the megabytes
 *    of the second disk).
 *
 * 4) Then keep calling setCurrentOperationProgress() with a percentage for
 *    the second image, where 100% of the operation will then yield a 100%
 *    progress of the entire task.
 *
 * Weighting is optional; you can simply assign a weight of 1 to each operation
 * and pass ulTotalOperationsWeight == cOperations to this constructor (but
 * for that variant and for backwards-compatibility a simpler constructor exists
 * in ProgressImpl.h as well).
 *
 * Even simpler, if you need no sub-operations at all, pass in cOperations =
 * ulTotalOperationsWeight = ulFirstOperationWeight = 1.
 *
 * @param aParent       Parent object (only for server-side Progress objects).
 * @param aInitiator    Initiator of the task (for server-side objects. Can be
 *                      NULL which means initiator = parent, otherwise must not
 *                      be NULL).
 * @param aDescription  Overall task description.
 * @param aCancelable   Flag whether the task maybe canceled.
 * @param cOperations   Number of operations within this task (at least 1).
 * @param ulTotalOperationsWeight Total weight of operations; must be the sum of ulFirstOperationWeight and
 *                          what is later passed with each subsequent setNextOperation() call.
 * @param aFirstOperationDescription Description of the first operation.
 * @param ulFirstOperationWeight Weight of first sub-operation.
 */
HRESULT Progress::init(
#if !defined(VBOX_COM_INPROC)
                       VirtualBox *aParent,
#endif
                       IUnknown *aInitiator,
                       Utf8Str aDescription,
                       BOOL aCancelable,
                       ULONG cOperations,
                       ULONG ulTotalOperationsWeight,
                       Utf8Str aFirstOperationDescription,
                       ULONG ulFirstOperationWeight)
{
    LogFlowThisFunc(("aDescription=\"%s\", cOperations=%d, ulTotalOperationsWeight=%d, aFirstOperationDescription=\"%s\", ulFirstOperationWeight=%d\n",
                     aDescription.c_str(),
                     cOperations,
                     ulTotalOperationsWeight,
                     aFirstOperationDescription.c_str(),
                     ulFirstOperationWeight));

    AssertReturn(ulTotalOperationsWeight >= 1, E_INVALIDARG);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    HRESULT rc = S_OK;
    rc = unconst(pEventSource).createObject();
    if (FAILED(rc)) throw rc;

    rc = pEventSource->init();
    if (FAILED(rc)) throw rc;

//    rc = Progress::init(
//#if !defined(VBOX_COM_INPROC)
//                        aParent,
//#endif
//                         aInitiator, aDescription, FALSE, aId);
// NA
#if !defined(VBOX_COM_INPROC)
    AssertReturn(aParent, E_INVALIDARG);
#else
    AssertReturn(aInitiator, E_INVALIDARG);
#endif

#if !defined(VBOX_COM_INPROC)
    /* share parent weakly */
    unconst(mParent) = aParent;
#endif

#if !defined(VBOX_COM_INPROC)
    /* assign (and therefore addref) initiator only if it is not VirtualBox
     * (to avoid cycling); otherwise mInitiator will remain null which means
     * that it is the same as the parent */
    if (aInitiator)
    {
        ComObjPtr<VirtualBox> pVirtualBox(mParent);
        if (!(pVirtualBox == aInitiator))
            unconst(mInitiator) = aInitiator;
    }
#else
    unconst(mInitiator) = aInitiator;
#endif

    unconst(mId).create();

#if !defined(VBOX_COM_INPROC)
    /* add to the global collection of progress operations (note: after
     * creating mId) */
    mParent->i_addProgress(this);
#endif

    unconst(mDescription) = aDescription;


// end of assertion


    if (FAILED(rc)) return rc;

    mCancelable = aCancelable;

    m_cOperations = cOperations;
    m_ulTotalOperationsWeight = ulTotalOperationsWeight;
    m_ulOperationsCompletedWeight = 0;
    m_ulCurrentOperation = 0;
    m_operationDescription = aFirstOperationDescription;
    m_ulCurrentOperationWeight = ulFirstOperationWeight;
    m_ulOperationPercent = 0;

    int vrc = RTSemEventMultiCreate(&mCompletedSem);
    ComAssertRCRet(vrc, E_FAIL);

    RTSemEventMultiReset(mCompletedSem);

    /* Confirm a successful initialization when it's the case */
    if (SUCCEEDED(rc))
        autoInitSpan.setSucceeded();

    return rc;
}

/**
 * Initializes the sub-progress object that represents a specific operation of
 * the whole task.
 *
 * Objects initialized with this method are then combined together into the
 * single task using a Progress instance, so it doesn't require the
 * parent, initiator, description and doesn't create an ID. Note that calling
 * respective getter methods on an object initialized with this method is
 * useless. Such objects are used only to provide a separate wait semaphore and
 * store individual operation descriptions.
 *
 * @param aCancelable       Flag whether the task maybe canceled.
 * @param aOperationCount   Number of sub-operations within this task (at least 1).
 * @param aOperationDescription Description of the individual operation.
 */
HRESULT Progress::init(BOOL aCancelable,
                       ULONG aOperationCount,
                       Utf8Str aOperationDescription)
{
    LogFlowThisFunc(("aOperationDescription=\"%s\"\n", aOperationDescription.c_str()));

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    HRESULT rc = S_OK;
    /* Guarantees subclasses call this method at the proper time */
    NOREF(autoInitSpan);

    if (FAILED(rc)) return rc;

    mCancelable = aCancelable;

    // for this variant we assume for now that all operations are weighed "1"
    // and equal total weight = operation count
    m_cOperations = aOperationCount;
    m_ulTotalOperationsWeight = aOperationCount;
    m_ulOperationsCompletedWeight = 0;
    m_ulCurrentOperation = 0;
    m_operationDescription = aOperationDescription;
    m_ulCurrentOperationWeight = 1;
    m_ulOperationPercent = 0;

    int vrc = RTSemEventMultiCreate(&mCompletedSem);
    ComAssertRCRet(vrc, E_FAIL);

    RTSemEventMultiReset(mCompletedSem);

    /* Confirm a successful initialization when it's the case */
    if (SUCCEEDED(rc))
        autoInitSpan.setSucceeded();

    return rc;
}


/**
 * Uninitializes the instance and sets the ready flag to FALSE.
 *
 * Called either from FinalRelease() or by the parent when it gets destroyed.
 */
void Progress::uninit()
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    /* wake up all threads still waiting on occasion */
    if (mWaitersCount > 0)
    {
        LogFlow(("WARNING: There are still %d threads waiting for '%s' completion!\n",
                 mWaitersCount, mDescription.c_str()));
        RTSemEventMultiSignal(mCompletedSem);
    }

    RTSemEventMultiDestroy(mCompletedSem);

    /* release initiator (effective only if mInitiator has been assigned in init()) */
    unconst(mInitiator).setNull();

#if !defined(VBOX_COM_INPROC)
    if (mParent)
    {
        /* remove the added progress on failure to complete the initialization */
        if (autoUninitSpan.initFailed() && mId.isValid() && !mId.isZero())
            mParent->i_removeProgress(mId.ref());

        unconst(mParent) = NULL;
    }
#endif
}


// public methods only for internal purposes
////////////////////////////////////////////////////////////////////////////////

/**
 * Marks the whole task as complete and sets the result code.
 *
 * If the result code indicates a failure (|FAILED(@a aResultCode)|) then this
 * method will import the error info from the current thread and assign it to
 * the errorInfo attribute (it will return an error if no info is available in
 * such case).
 *
 * If the result code indicates a success (|SUCCEEDED(@a aResultCode)|) then
 * the current operation is set to the last.
 *
 * Note that this method may be called only once for the given Progress object.
 * Subsequent calls will assert.
 *
 * @param aResultCode   Operation result code.
 */
HRESULT Progress::i_notifyComplete(HRESULT aResultCode)
{
    HRESULT rc;
    ComPtr<IVirtualBoxErrorInfo> errorInfo;
    if (FAILED(aResultCode))
    {
        /* try to import error info from the current thread */
#if !defined(VBOX_WITH_XPCOM)
        ComPtr<IErrorInfo> err;
        rc = ::GetErrorInfo(0, err.asOutParam());
        if (rc == S_OK && err)
            rc = err.queryInterfaceTo(errorInfo.asOutParam());
#else /* !defined(VBOX_WITH_XPCOM) */
        nsCOMPtr<nsIExceptionService> es;
        es = do_GetService(NS_EXCEPTIONSERVICE_CONTRACTID, &rc);
        if (NS_SUCCEEDED(rc))
        {
            nsCOMPtr <nsIExceptionManager> em;
            rc = es->GetCurrentExceptionManager(getter_AddRefs(em));
            if (NS_SUCCEEDED(rc))
            {
                ComPtr<nsIException> ex;
                rc = em->GetCurrentException(ex.asOutParam());
                if (NS_SUCCEEDED(rc) && ex)
                    rc = ex.queryInterfaceTo(errorInfo.asOutParam());
            }
        }
#endif /* !defined(VBOX_WITH_XPCOM) */
    }

    return i_notifyCompleteEI(aResultCode, errorInfo);
}

/**
 * Wrapper around Progress:notifyCompleteV.
 */
HRESULT Progress::i_notifyComplete(HRESULT aResultCode,
                                   const GUID &aIID,
                                   const char *pcszComponent,
                                   const char *aText,
                                   ...)
{
    va_list va;
    va_start(va, aText);
    HRESULT hrc = i_notifyCompleteV(aResultCode, aIID, pcszComponent, aText, va);
    va_end(va);
    return hrc;
}

/**
 * Marks the operation as complete and attaches full error info.
 *
 * @param aResultCode   Operation result (error) code, must not be S_OK.
 * @param aIID          IID of the interface that defines the error.
 * @param pcszComponent Name of the component that generates the error.
 * @param aText         Error message (must not be null), an RTStrPrintf-like
 *                      format string in UTF-8 encoding.
 * @param va            List of arguments for the format string.
 */
HRESULT Progress::i_notifyCompleteV(HRESULT aResultCode,
                                    const GUID &aIID,
                                    const char *pcszComponent,
                                    const char *aText,
                                    va_list va)
{
    /* expected to be used only in case of error */
    Assert(FAILED(aResultCode));

    Utf8Str text(aText, va);
    ComObjPtr<VirtualBoxErrorInfo> errorInfo;
    HRESULT rc = errorInfo.createObject();
    AssertComRCReturnRC(rc);
    errorInfo->init(aResultCode, aIID, pcszComponent, text);

    return i_notifyCompleteEI(aResultCode, errorInfo);
}

/**
 * Marks the operation as complete and attaches full error info.
 *
 * This is where the actual work is done, the related methods all end up here.
 *
 * @param aResultCode   Operation result (error) code, must not be S_OK.
 * @param aErrorInfo            List of arguments for the format string.
 */
HRESULT Progress::i_notifyCompleteEI(HRESULT aResultCode, const ComPtr<IVirtualBoxErrorInfo> &aErrorInfo)
{
    LogThisFunc(("aResultCode=%d\n", aResultCode));
    /* on failure we expect error info, on success there must be none */
    AssertMsg(FAILED(aResultCode) ^ aErrorInfo.isNull(),
              ("No error info but trying to set a failed result (%08X)!\n",
               aResultCode));

    AutoCaller autoCaller(this);
    AssertComRCReturnRC(autoCaller.rc());

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    AssertReturn(mCompleted == FALSE, E_FAIL);

    if (mCanceled && SUCCEEDED(aResultCode))
        aResultCode = E_FAIL;

    mCompleted = TRUE;
    mResultCode = aResultCode;
    if (SUCCEEDED(aResultCode))
    {
        m_ulCurrentOperation = m_cOperations - 1; /* last operation */
        m_ulOperationPercent = 100;
    }
    mErrorInfo = aErrorInfo;

#if !defined VBOX_COM_INPROC
    /* remove from the global collection of pending progress operations */
    if (mParent)
        mParent->i_removeProgress(mId.ref());
#endif

    /* wake up all waiting threads */
    if (mWaitersCount > 0)
        RTSemEventMultiSignal(mCompletedSem);

    fireProgressTaskCompletedEvent(pEventSource, mId.toUtf16().raw());

    return S_OK;
}

/**
 * Notify the progress object that we're almost at the point of no return.
 *
 * This atomically checks for and disables cancelation.  Calls to
 * IProgress::Cancel() made after a successful call to this method will fail
 * and the user can be told.  While this isn't entirely clean behavior, it
 * prevents issues with an irreversible actually operation succeeding while the
 * user believe it was rolled back.
 *
 * @returns Success indicator.
 * @retval  true on success.
 * @retval  false if the progress object has already been canceled or is in an
 *          invalid state
 */
bool Progress::i_notifyPointOfNoReturn(void)
{
    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), false);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (mCanceled)
    {
        LogThisFunc(("returns false\n"));
        return false;
    }

    mCancelable = FALSE;
    LogThisFunc(("returns true\n"));
    return true;
}

/**
 * Sets the cancelation callback, checking for cancelation first.
 *
 * @returns Success indicator.
 * @retval  true on success.
 * @retval  false if the progress object has already been canceled or is in an
 *          invalid state
 *
 * @param   pfnCallback     The function to be called upon cancelation.
 * @param   pvUser          The callback argument.
 */
bool Progress::i_setCancelCallback(void (*pfnCallback)(void *), void *pvUser)
{
    AutoCaller autoCaller(this);
    AssertComRCReturn(autoCaller.rc(), false);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    i_checkForAutomaticTimeout();
    if (mCanceled)
        return false;

    m_pvCancelUserArg   = pvUser;
    m_pfnCancelCallback = pfnCallback;
    return true;
}

/**
 * @callback_method_impl{FNRTPROGRESS,
 *      Works the progress of the current operation.}
 */
/*static*/ DECLCALLBACK(int) Progress::i_iprtProgressCallback(unsigned uPercentage, void *pvUser)
{
    Progress *pThis = (Progress *)pvUser;

    /*
     * Same as setCurrentOperationProgress, except we don't fail on mCompleted.
     */
    AutoWriteLock alock(pThis COMMA_LOCKVAL_SRC_POS);
    int vrc = VINF_SUCCESS;
    if (!pThis->mCompleted)
    {
        pThis->i_checkForAutomaticTimeout();
        if (!pThis->mCanceled)
        {
            if (uPercentage > pThis->m_ulOperationPercent)
                pThis->setCurrentOperationProgress(uPercentage);
        }
        else
        {
            Assert(pThis->mCancelable);
            vrc = VERR_CANCELLED;
        }
    }
    /* else ignored */
    return vrc;
}

/**
 * @callback_method_impl{FNVDPROGRESS,
 *      Progress::i_iprtProgressCallback with parameters switched around.}
 */
/*static*/ DECLCALLBACK(int) Progress::i_vdProgressCallback(void *pvUser, unsigned uPercentage)
{
    return i_iprtProgressCallback(uPercentage, pvUser);
}

// IProgress properties
/////////////////////////////////////////////////////////////////////////////

HRESULT Progress::getId(com::Guid &aId)
{
    /* mId is constant during life time, no need to lock */
    aId = mId;

    return S_OK;
}

HRESULT Progress::getDescription(com::Utf8Str &aDescription)
{
    /* mDescription is constant during life time, no need to lock */
    aDescription = mDescription;

    return S_OK;
}
HRESULT Progress::getInitiator(ComPtr<IUnknown> &aInitiator)
{
    /* mInitiator/mParent are constant during life time, no need to lock */
#if !defined(VBOX_COM_INPROC)
    if (mInitiator)
        mInitiator.queryInterfaceTo(aInitiator.asOutParam());
    else
    {
        ComObjPtr<VirtualBox> pVirtualBox(mParent);
        pVirtualBox.queryInterfaceTo(aInitiator.asOutParam());
    }
#else
    mInitiator.queryInterfaceTo(aInitiator.asOutParam());
#endif

    return S_OK;
}

HRESULT Progress::getCancelable(BOOL *aCancelable)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aCancelable = mCancelable;

    return S_OK;
}

HRESULT Progress::getPercent(ULONG *aPercent)
{
    /* i_checkForAutomaticTimeout requires a write lock. */
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (mCompleted && SUCCEEDED(mResultCode))
        *aPercent = 100;
    else
    {
        ULONG ulPercent = (ULONG)i_calcTotalPercent();
        // do not report 100% until we're really really done with everything
        // as the Qt GUI dismisses progress dialogs in that case
        if (    ulPercent == 100
             && (    m_ulOperationPercent < 100
                  || (m_ulCurrentOperation < m_cOperations -1)
                )
           )
            *aPercent = 99;
        else
            *aPercent = ulPercent;
    }

    i_checkForAutomaticTimeout();

    return S_OK;
}

HRESULT Progress::getTimeRemaining(LONG *aTimeRemaining)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (mCompleted)
        *aTimeRemaining = 0;
    else
    {
        double dPercentDone = i_calcTotalPercent();
        if (dPercentDone < 1)
            *aTimeRemaining = -1;       // unreliable, or avoid division by 0 below
        else
        {
            uint64_t ullTimeNow = RTTimeMilliTS();
            uint64_t ullTimeElapsed = ullTimeNow - m_ullTimestamp;
            uint64_t ullTimeTotal = (uint64_t)((double)ullTimeElapsed * 100 / dPercentDone);
            uint64_t ullTimeRemaining = ullTimeTotal - ullTimeElapsed;

//          LogFunc(("dPercentDone = %RI32, ullTimeNow = %RI64, ullTimeElapsed = %RI64, ullTimeTotal = %RI64, ullTimeRemaining = %RI64\n",
//                   (uint32_t)dPercentDone, ullTimeNow, ullTimeElapsed, ullTimeTotal, ullTimeRemaining));

            *aTimeRemaining = (LONG)(ullTimeRemaining / 1000);
        }
    }

    return S_OK;
}

HRESULT Progress::getCompleted(BOOL *aCompleted)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aCompleted = mCompleted;

    return S_OK;
}

HRESULT Progress::getCanceled(BOOL *aCanceled)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aCanceled = mCanceled;

    return S_OK;
}

HRESULT Progress::getResultCode(LONG *aResultCode)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (!mCompleted)
        return setError(E_FAIL,
                        tr("Result code is not available, operation is still in progress"));

    *aResultCode = mResultCode;

    return S_OK;
}

HRESULT Progress::getErrorInfo(ComPtr<IVirtualBoxErrorInfo> &aErrorInfo)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (!mCompleted)
        return setError(E_FAIL,
                        tr("Error info is not available, operation is still in progress"));

    mErrorInfo.queryInterfaceTo(aErrorInfo.asOutParam());

    return S_OK;
}

HRESULT Progress::getOperationCount(ULONG *aOperationCount)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aOperationCount = m_cOperations;

    return S_OK;
}

HRESULT Progress::getOperation(ULONG *aOperation)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aOperation = m_ulCurrentOperation;

    return S_OK;
}

HRESULT Progress::getOperationDescription(com::Utf8Str &aOperationDescription)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aOperationDescription = m_operationDescription;

    return S_OK;
}

HRESULT Progress::getOperationPercent(ULONG *aOperationPercent)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (mCompleted && SUCCEEDED(mResultCode))
        *aOperationPercent = 100;
    else
        *aOperationPercent = m_ulOperationPercent;

    return S_OK;
}

HRESULT Progress::getOperationWeight(ULONG *aOperationWeight)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aOperationWeight = m_ulCurrentOperationWeight;

    return S_OK;
}

HRESULT Progress::getTimeout(ULONG *aTimeout)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aTimeout = m_cMsTimeout;

    return S_OK;
}

HRESULT Progress::setTimeout(ULONG aTimeout)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (!mCancelable)
        return setError(VBOX_E_INVALID_OBJECT_STATE,
                        tr("Operation cannot be canceled"));
    m_cMsTimeout = aTimeout;

    return S_OK;
}


// IProgress methods
/////////////////////////////////////////////////////////////////////////////

/**
 * Updates the percentage value of the current operation.
 *
 * @param aPercent  New percentage value of the operation in progress
 *                  (in range [0, 100]).
 */
HRESULT Progress::setCurrentOperationProgress(ULONG aPercent)
{
    AssertMsgReturn(aPercent <= 100, ("%u\n", aPercent), E_INVALIDARG);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    i_checkForAutomaticTimeout();
    if (mCancelable && mCanceled)
        AssertReturn(!mCompleted, E_FAIL);
    AssertReturn(!mCompleted && !mCanceled, E_FAIL);

    if (m_ulOperationPercent != aPercent)
    {
        m_ulOperationPercent = aPercent;
        ULONG actualPercent = 0;
        getPercent(&actualPercent);
        fireProgressPercentageChangedEvent(pEventSource, mId.toUtf16().raw(), actualPercent);
    }

    return S_OK;
}

/**
 * Signals that the current operation is successfully completed and advances to
 * the next operation. The operation percentage is reset to 0.
 *
 * @param aNextOperationDescription  Description of the next operation.
 * @param aNextOperationsWeight     Weight of the next operation.
 *
 * @note The current operation must not be the last one.
 */
HRESULT Progress::setNextOperation(const com::Utf8Str &aNextOperationDescription, ULONG aNextOperationsWeight)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (mCanceled)
        return E_FAIL;
    AssertReturn(!mCompleted, E_FAIL);
    AssertReturn(m_ulCurrentOperation + 1 < m_cOperations, E_FAIL);

    ++m_ulCurrentOperation;
    m_ulOperationsCompletedWeight += m_ulCurrentOperationWeight;

    m_operationDescription = aNextOperationDescription;
    m_ulCurrentOperationWeight = aNextOperationsWeight;
    m_ulOperationPercent = 0;

    LogThisFunc(("%s: aNextOperationsWeight = %d; m_ulCurrentOperation is now %d, m_ulOperationsCompletedWeight is now %d\n",
                 m_operationDescription.c_str(), aNextOperationsWeight, m_ulCurrentOperation, m_ulOperationsCompletedWeight));

    /* wake up all waiting threads */
    if (mWaitersCount > 0)
        RTSemEventMultiSignal(mCompletedSem);

    ULONG actualPercent = 0;
    getPercent(&actualPercent);
    fireProgressPercentageChangedEvent(pEventSource, mId.toUtf16().raw(), actualPercent);

    return S_OK;
}

/**
 * @note XPCOM: when this method is not called on the main XPCOM thread, it
 *       simply blocks the thread until mCompletedSem is signalled. If the
 *       thread has its own event queue (hmm, what for?) that it must run, then
 *       calling this method will definitely freeze event processing.
 */
HRESULT Progress::waitForCompletion(LONG aTimeout)
{
    LogFlowThisFuncEnter();
    LogFlowThisFunc(("aTimeout=%d\n", aTimeout));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* if we're already completed, take a shortcut */
    if (!mCompleted)
    {
        int vrc = VINF_SUCCESS;
        bool fForever = aTimeout < 0;
        int64_t timeLeft = aTimeout;
        int64_t lastTime = RTTimeMilliTS();

        while (!mCompleted && (fForever || timeLeft > 0))
        {
            mWaitersCount++;
            alock.release();
            vrc = RTSemEventMultiWait(mCompletedSem,
                                      fForever ? RT_INDEFINITE_WAIT : (RTMSINTERVAL)timeLeft);
            alock.acquire();
            mWaitersCount--;

            /* the last waiter resets the semaphore */
            if (mWaitersCount == 0)
                RTSemEventMultiReset(mCompletedSem);

            if (RT_FAILURE(vrc) && vrc != VERR_TIMEOUT)
                break;

            if (!fForever)
            {
                int64_t now = RTTimeMilliTS();
                timeLeft -= now - lastTime;
                lastTime = now;
            }
        }

        if (RT_FAILURE(vrc) && vrc != VERR_TIMEOUT)
            return setError(VBOX_E_IPRT_ERROR,
                            tr("Failed to wait for the task completion (%Rrc)"),
                            vrc);
    }

    LogFlowThisFuncLeave();

    return S_OK;
}

/**
 * @note XPCOM: when this method is not called on the main XPCOM thread, it
 *       simply blocks the thread until mCompletedSem is signalled. If the
 *       thread has its own event queue (hmm, what for?) that it must run, then
 *       calling this method will definitely freeze event processing.
 */
HRESULT Progress::waitForOperationCompletion(ULONG aOperation, LONG aTimeout)

{
    LogFlowThisFuncEnter();
    LogFlowThisFunc(("aOperation=%d, aTimeout=%d\n", aOperation, aTimeout));

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    CheckComArgExpr(aOperation, aOperation < m_cOperations);

    /* if we're already completed or if the given operation is already done,
     * then take a shortcut */
    if (    !mCompleted
         && aOperation >= m_ulCurrentOperation)
    {
        int vrc = VINF_SUCCESS;
        bool fForever = aTimeout < 0;
        int64_t timeLeft = aTimeout;
        int64_t lastTime = RTTimeMilliTS();

        while (    !mCompleted && aOperation >= m_ulCurrentOperation
                && (fForever || timeLeft > 0))
        {
            mWaitersCount ++;
            alock.release();
            vrc = RTSemEventMultiWait(mCompletedSem,
                                      fForever ? RT_INDEFINITE_WAIT : (unsigned) timeLeft);
            alock.acquire();
            mWaitersCount--;

            /* the last waiter resets the semaphore */
            if (mWaitersCount == 0)
                RTSemEventMultiReset(mCompletedSem);

            if (RT_FAILURE(vrc) && vrc != VERR_TIMEOUT)
                break;

            if (!fForever)
            {
                int64_t now = RTTimeMilliTS();
                timeLeft -= now - lastTime;
                lastTime = now;
            }
        }

        if (RT_FAILURE(vrc) && vrc != VERR_TIMEOUT)
            return setError(E_FAIL,
                            tr("Failed to wait for the operation completion (%Rrc)"),
                            vrc);
    }

    LogFlowThisFuncLeave();

    return S_OK;
}

HRESULT Progress::waitForAsyncProgressCompletion(const ComPtr<IProgress> &aPProgressAsync)
{
    LogFlowThisFuncEnter();

    /* Note: we don't lock here, cause we just using public methods. */

    HRESULT rc           = S_OK;
    BOOL fCancelable     = FALSE;
    BOOL fCompleted      = FALSE;
    BOOL fCanceled       = FALSE;
    ULONG prevPercent    = UINT32_MAX;
    ULONG currentPercent = 0;
    ULONG cOp            = 0;
    /* Is the async process cancelable? */
    rc = aPProgressAsync->COMGETTER(Cancelable)(&fCancelable);
    if (FAILED(rc)) return rc;
    /* Loop as long as the sync process isn't completed. */
    while (SUCCEEDED(aPProgressAsync->COMGETTER(Completed(&fCompleted))))
    {
        /* We can forward any cancel request to the async process only when
         * it is cancelable. */
        if (fCancelable)
        {
            rc = COMGETTER(Canceled)(&fCanceled);
            if (FAILED(rc)) return rc;
            if (fCanceled)
            {
                rc = aPProgressAsync->Cancel();
                if (FAILED(rc)) return rc;
            }
        }
        /* Even if the user canceled the process, we have to wait until the
           async task has finished his work (cleanup and such). Otherwise there
           will be sync trouble (still wrong state, dead locks, ...) on the
           used objects. So just do nothing, but wait for the complete
           notification. */
        if (!fCanceled)
        {
            /* Check if the current operation has changed. It is also possible that
             * in the meantime more than one async operation was finished. So we
             * have to loop as long as we reached the same operation count. */
            ULONG curOp;
            for (;;)
            {
                rc = aPProgressAsync->COMGETTER(Operation(&curOp));
                if (FAILED(rc)) return rc;
                if (cOp != curOp)
                {
                    Bstr bstr;
                    ULONG currentWeight;
                    rc = aPProgressAsync->COMGETTER(OperationDescription(bstr.asOutParam()));
                    if (FAILED(rc)) return rc;
                    rc = aPProgressAsync->COMGETTER(OperationWeight(&currentWeight));
                    if (FAILED(rc)) return rc;
                    rc = SetNextOperation(bstr.raw(), currentWeight);
                    if (FAILED(rc)) return rc;
                    ++cOp;
                }
                else
                    break;
            }

            rc = aPProgressAsync->COMGETTER(OperationPercent(&currentPercent));
            if (FAILED(rc)) return rc;
            if (currentPercent != prevPercent)
            {
                prevPercent = currentPercent;
                rc = SetCurrentOperationProgress(currentPercent);
                if (FAILED(rc)) return rc;
            }
        }
        if (fCompleted)
            break;

        /* Make sure the loop is not too tight */
        rc = aPProgressAsync->WaitForCompletion(100);
        if (FAILED(rc)) return rc;
    }

    LogFlowThisFuncLeave();

    return rc;
}

HRESULT Progress::cancel()
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    if (!mCancelable)
        return setError(VBOX_E_INVALID_OBJECT_STATE,
                        tr("Operation cannot be canceled"));

    if (!mCanceled)
    {
        LogThisFunc(("Canceling\n"));
        mCanceled = TRUE;
        if (m_pfnCancelCallback)
            m_pfnCancelCallback(m_pvCancelUserArg);

    }
    else
        LogThisFunc(("Already canceled\n"));

    return S_OK;
}

HRESULT Progress::getEventSource(ComPtr<IEventSource> &aEventSource)
{
    /* event source is const, no need to lock */
    pEventSource.queryInterfaceTo(aEventSource.asOutParam());
    return S_OK;
}

// private internal helpers
/////////////////////////////////////////////////////////////////////////////

/**
 * Internal helper to compute the total percent value based on the member values and
 * returns it as a "double". This is used both by GetPercent (which returns it as a
 * rounded ULONG) and GetTimeRemaining().
 *
 * Requires locking by the caller!
 *
 * @return fractional percentage as a double value.
 */
double Progress::i_calcTotalPercent()
{
    // avoid division by zero
    if (m_ulTotalOperationsWeight == 0)
        return 0.0;

    double dPercent = (    (double)m_ulOperationsCompletedWeight  // weight of operations that have been completed
                         + ((double)m_ulOperationPercent *
                            (double)m_ulCurrentOperationWeight / 100.0)  // plus partial weight of the current operation
                      ) * 100.0 / (double)m_ulTotalOperationsWeight;

    return dPercent;
}

/**
 * Internal helper for automatically timing out the operation.
 *
 * The caller must hold the object write lock.
 */
void Progress::i_checkForAutomaticTimeout(void)
{
    AssertReturnVoid(isWriteLockOnCurrentThread());

    if (   m_cMsTimeout
        && mCancelable
        && !mCanceled
        && RTTimeMilliTS() - m_ullTimestamp > m_cMsTimeout)
        Cancel();
}
