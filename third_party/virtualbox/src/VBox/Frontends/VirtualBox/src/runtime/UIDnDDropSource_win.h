/* $Id: UIDnDDropSource_win.h $ */
/** @file
 * VBox Qt GUI - UIDnDDropSource class declaration.
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

#ifndef ___UIDnDDropSource_win_h___
#define ___UIDnDDropSource_win_h___

/* COM includes: */
#include "COMEnums.h"

class UIDnDDataObject;

/**
 * Implementation of IDropSource for drag and drop on the host.
 */
class UIDnDDropSource : public IDropSource
{
public:

    UIDnDDropSource(QWidget *pParent, UIDnDDataObject *pDataObject);
    virtual ~UIDnDDropSource(void);

public:

    uint32_t GetCurrentAction(void) const { return m_uCurAction; }

public: /* IUnknown methods. */

    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);
    STDMETHOD_(ULONG, AddRef)(void);
    STDMETHOD_(ULONG, Release)(void);

public: /* IDropSource methods. */

    STDMETHOD(QueryContinueDrag)(BOOL fEscapePressed, DWORD dwKeyState);
    STDMETHOD(GiveFeedback)(DWORD dwEffect);

protected:

    /** Pointer to parent widget. */
    QWidget         *m_pParent;
    UIDnDDataObject *m_pDataObject;
    /** The current reference count. */
    LONG             m_cRefCount;
    /** Current (last) drop effect issued. */
    DWORD            m_dwCurEffect;
    /** Current drop action to perform in case of a successful drop. */
    Qt::DropActions  m_uCurAction;
};

#endif /* ___UIDnDDropSource_win_h___ */

