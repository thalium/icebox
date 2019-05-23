/* $Id: ProgressProxyImpl.h $ */
/** @file
 * IProgress implementation for Machine::LaunchVMProcess in VBoxSVC.
 */

/*
 * Copyright (C) 2006-2016 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_PROGRESSPROXYIMPL
#define ____H_PROGRESSPROXYIMPL

#include "ProgressImpl.h"
#include "AutoCaller.h"


/**
 * The ProgressProxy class allows proxying the important Progress calls and
 * attributes to a different IProgress object for a period of time.
 */
class ATL_NO_VTABLE ProgressProxy :
    public Progress
{
public:
    VIRTUALBOXBASE_ADD_ERRORINFO_SUPPORT(ProgressProxy, IProgress)

    DECLARE_NOT_AGGREGATABLE(ProgressProxy)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(ProgressProxy)
        COM_INTERFACE_ENTRY(ISupportErrorInfo)
        COM_INTERFACE_ENTRY(IProgress)
        COM_INTERFACE_ENTRY2(IDispatch, IProgress)
        VBOX_TWEAK_INTERFACE_ENTRY(IProgress)
    END_COM_MAP()

    HRESULT FinalConstruct();
    void    FinalRelease();
    HRESULT init(
#ifndef VBOX_COM_INPROC
                 VirtualBox *pParent,
#endif
                 IUnknown *pInitiator,
                 CBSTR bstrDescription,
                 BOOL fCancelable);
    HRESULT init(
#ifndef VBOX_COM_INPROC
                 VirtualBox *pParent,
#endif
                 IUnknown *pInitiator,
                 CBSTR bstrDescription,
                 BOOL fCancelable,
                 ULONG uTotalOperationsWeight,
                 CBSTR bstrFirstOperationDescription,
                 ULONG uFirstOperationWeight,
                 ULONG cOtherProgressObjectOperations);
    void    uninit();

    // IProgress properties
    STDMETHOD(COMGETTER(Cancelable))(BOOL *aCancelable);
    STDMETHOD(COMGETTER(Percent))(ULONG *aPercent);
    STDMETHOD(COMGETTER(TimeRemaining))(LONG *aTimeRemaining);
    STDMETHOD(COMGETTER(Completed))(BOOL *aCompleted);
    STDMETHOD(COMGETTER(Canceled))(BOOL *aCanceled);
    STDMETHOD(COMGETTER(ResultCode))(LONG *aResultCode);
    STDMETHOD(COMGETTER(ErrorInfo))(IVirtualBoxErrorInfo **aErrorInfo);
    //STDMETHOD(COMGETTER(OperationCount))(ULONG *aOperationCount); - not necessary
    STDMETHOD(COMGETTER(Operation))(ULONG *aOperation);
    STDMETHOD(COMGETTER(OperationDescription))(BSTR *aOperationDescription);
    STDMETHOD(COMGETTER(OperationPercent))(ULONG *aOperationPercent);
    STDMETHOD(COMSETTER(Timeout))(ULONG aTimeout);
    STDMETHOD(COMGETTER(Timeout))(ULONG *aTimeout);

    // IProgress methods
    STDMETHOD(WaitForCompletion)(LONG aTimeout);
    STDMETHOD(WaitForOperationCompletion)(ULONG aOperation, LONG aTimeout);
    STDMETHOD(Cancel)();
    STDMETHOD(SetCurrentOperationProgress)(ULONG aPercent);
    STDMETHOD(SetNextOperation)(IN_BSTR bstrNextOperationDescription, ULONG ulNextOperationsWeight);

    // public methods only for internal purposes

    HRESULT notifyComplete(HRESULT aResultCode);
    HRESULT notifyComplete(HRESULT aResultCode,
                           const GUID &aIID,
                           const char *pcszComponent,
                           const char *aText, ...);
    bool setOtherProgressObject(IProgress *pOtherProgress);

protected:
    void clearOtherProgressObjectInternal(bool fEarly);
    void copyProgressInfo(IProgress *pOtherProgress, bool fEarly);

private:
    /** The other progress object.  This can be NULL. */
    ComPtr<IProgress> mptrOtherProgress;
    /** Set if the other progress object has multiple operations. */
    bool mfMultiOperation;
    /** The weight the other progress object started at. */
    ULONG muOtherProgressStartWeight;
    /** The weight of other progress object. */
    ULONG muOtherProgressWeight;
    /** The operation number the other progress object started at. */
    ULONG muOtherProgressStartOperation;

};

#endif /* !____H_PROGRESSPROXYIMPL */

