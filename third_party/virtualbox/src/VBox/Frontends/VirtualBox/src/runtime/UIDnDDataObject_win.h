/* $Id: UIDnDDataObject_win.h $ */
/** @file
 * VBox Qt GUI - UIDnDDataObject class declaration.
 */

/*
 * Copyright (C) 2014-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIDnDDataObject_win_h___
#define ___UIDnDDataObject_win_h___

#include <iprt/critsect.h>

#include <QString>
#include <QStringList>
#include <QVariant>

/* COM includes: */
#include "COMEnums.h"
#include "CDndSource.h"
#include "CSession.h"

/* Forward declarations: */
class UIDnDHandler;

class UIDnDDataObject : public IDataObject
{
public:

    enum DnDDataObjectStatus
    {
        DnDDataObjectStatus_Uninitialized = 0,
        DnDDataObjectStatus_Initialized,
        DnDDataObjectStatus_Dropping,
        DnDDataObjectStatus_Dropped,
        DnDDataObjectStatus_Aborted,
        DnDDataObjectStatus_32Bit_Hack = 0x7fffffff
    };

public:

    UIDnDDataObject(UIDnDHandler *pDnDHandler, const QStringList &lstFormats);
    virtual ~UIDnDDataObject(void);

public: /* IUnknown methods. */

    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

public: /* IDataObject methods. */

    STDMETHOD(GetData)(LPFORMATETC pFormatEtc, LPSTGMEDIUM pMedium);
    STDMETHOD(GetDataHere)(LPFORMATETC pFormatEtc, LPSTGMEDIUM pMedium);
    STDMETHOD(QueryGetData)(LPFORMATETC pFormatEtc);
    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC pFormatEtc,  LPFORMATETC pFormatEtcOut);
    STDMETHOD(SetData)(LPFORMATETC pFormatEtc, LPSTGMEDIUM pMedium, BOOL fRelease);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc);
    STDMETHOD(DAdvise)(LPFORMATETC pFormatEtc, DWORD fAdvise, IAdviseSink *pAdvSink, DWORD *pdwConnection);
    STDMETHOD(DUnadvise)(DWORD dwConnection);
    STDMETHOD(EnumDAdvise)(IEnumSTATDATA **ppEnumAdvise);

public:

    static const char *ClipboardFormatToString(CLIPFORMAT fmt);

    int Abort(void);
    void Signal(void);
    int Signal(const QString &strFormat, const void *pvData, uint32_t cbData);

protected:

    void SetStatus(DnDDataObjectStatus enmStatus);

    bool LookupFormatEtc(LPFORMATETC pFormatEtc, ULONG *puIndex);
    void RegisterFormat(LPFORMATETC pFormatEtc, CLIPFORMAT clipFormat, TYMED tyMed = TYMED_HGLOBAL,
                        LONG lindex = -1, DWORD dwAspect = DVASPECT_CONTENT, DVTARGETDEVICE *pTargetDevice = NULL);

    /** Pointe rto drag and drop handler. */
    UIDnDHandler           *m_pDnDHandler;
    /** Current drag and drop status. */
    DnDDataObjectStatus     m_enmStatus;
    /** Internal reference count of this object. */
    LONG                    m_cRefs;
    /** Number of native formats registered. This can be a different number than supplied with mlstFormats. */
    ULONG                   m_cFormats;
    FORMATETC              *m_pFormatEtc;
    STGMEDIUM              *m_pStgMedium;
    RTSEMEVENT              m_SemEvent;
    QStringList             m_lstFormats;
    QString                 m_strFormat;
    /** The retrieved data as a QVariant. Needed for buffering in case a second format needs the same data,
     *  e.g. CF_TEXT and CF_UNICODETEXT. */
    QVariant                m_vaData;
    /** Whether the data already was retrieved or not. */
    bool                    m_fDataRetrieved;
    /** The retrieved data as a raw buffer. */
    void                   *m_pvData;
    /** Raw buffer size (in bytes). */
    uint32_t                m_cbData;
};

#endif /* !___UIDnDDataObject_win_h___ */

