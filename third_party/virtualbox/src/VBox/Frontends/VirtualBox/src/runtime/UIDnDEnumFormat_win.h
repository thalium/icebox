/* $Id: UIDnDEnumFormat_win.h $ */
/** @file
 * VBox Qt GUI - UIDnDEnumFormat class declaration.
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

#ifndef ___UIDnDEnumFormat_win_h___
#define ___UIDnDEnumFormat_win_h___


class UIDnDEnumFormatEtc : public IEnumFORMATETC
{
public:

    UIDnDEnumFormatEtc(FORMATETC *pFormatEtc, ULONG cFormats);
    virtual ~UIDnDEnumFormatEtc(void);

public:

    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

    STDMETHOD(Next)(ULONG cFormats, FORMATETC *pFormatEtc, ULONG *pcFetched);
    STDMETHOD(Skip)(ULONG cFormats);
    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(IEnumFORMATETC ** ppEnumFormatEtc);

public:

    static void CopyFormat(FORMATETC *pFormatDest, FORMATETC *pFormatSource);
    static HRESULT CreateEnumFormatEtc(UINT cFormats, FORMATETC *pFormatEtc, IEnumFORMATETC **ppEnumFormatEtc);

private:

    LONG        m_lRefCount;
    ULONG       m_nIndex;
    ULONG       m_nNumFormats;
    FORMATETC * m_pFormatEtc;
};

#endif /* ___UIDnDEnumFormat_win_h___ */

