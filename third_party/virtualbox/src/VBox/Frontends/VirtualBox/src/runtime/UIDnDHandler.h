/* $Id: UIDnDHandler.h $ */
/** @file
 * VBox Qt GUI - UIDnDHandler class declaration..
 */

/*
 * Copyright (C) 2011-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___UIDnDHandler_h___
#define ___UIDnDHandler_h___

/* Qt includes: */
#include <QMimeData>
#include <QMutex>
#include <QStringList>

/* COM includes: */
#include "COMEnums.h"
#include "CDnDTarget.h"
#include "CDnDSource.h"

/* Forward declarations: */
class QMimeData;

class UIDnDMIMEData;
class UISession;

class UIDnDHandler: public QObject
{
    Q_OBJECT;

public:

    UIDnDHandler(UISession *pSession, QWidget *pParent);
    virtual ~UIDnDHandler(void);

    /**
     * Current operation mode.
     * Note: The operation mode is independent of the machine's overall
     *       drag and drop mode.
     */
    typedef enum DNDOPMODE
    {
        /** Unknown mode. */
        DNDMODE_UNKNOWN     = 0,
        /** Host to guest. */
        DNDMODE_HOSTTOGUEST = 1,
        /** Guest to host. */
        DNDMODE_GUESTTOHOST = 2,
        /** @todo Implement guest to guest. */
        /** The usual 32-bit type blow up. */
        DNDMODE_32BIT_HACK = 0x7fffffff
    } DNDOPMODE;

    /**
     * Drag and drop data set from the source.
     */
    typedef struct UIDnDDataSource
    {
        /** List of formats supported by the source. */
        QStringList         lstFormats;
        /** List of allowed drop actions from the source. */
        QVector<KDnDAction> vecActions;
        /** Default drop action from the source. */
        KDnDAction          defaultAction;

    } UIDnDDataSource;

    void                       reset(void);

    /* Frontend -> Target. */
    Qt::DropAction             dragEnter(ulong screenId, int x, int y, Qt::DropAction proposedAction, Qt::DropActions possibleActions, const QMimeData *pMimeData);
    Qt::DropAction             dragMove (ulong screenId, int x, int y, Qt::DropAction proposedAction, Qt::DropActions possibleActions, const QMimeData *pMimeData);
    Qt::DropAction             dragDrop (ulong screenId, int x, int y, Qt::DropAction proposedAction, Qt::DropActions possibleActions, const QMimeData *pMimeData);
    void                       dragLeave(ulong screenId);

    /* Source -> Frontend. */
    int                        dragCheckPending(ulong screenId);
    int                        dragStart(ulong screenId);
    int                        dragStop(ulong screenID);

    /* Data access. */
    int                        retrieveData(Qt::DropAction dropAction, const QString &strMIMEType, QVector<uint8_t> &vecData);
    int                        retrieveData(Qt::DropAction dropAction, const QString &strMIMEType, QVariant::Type vaType, QVariant &vaData);

public:

    static KDnDAction          toVBoxDnDAction(Qt::DropAction action);
    static QVector<KDnDAction> toVBoxDnDActions(Qt::DropActions actions);
    static Qt::DropAction      toQtDnDAction(KDnDAction action);
    static Qt::DropActions     toQtDnDActions(const QVector<KDnDAction> &vecActions);

public slots:

    /**
     * Called by UIDnDMIMEData (Linux, OS X, Solaris) to start retrieving the actual data
     * from the guest. This function will block and show a modal progress dialog until
     * the data transfer is complete.
     *
     * @return IPRT status code.
     * @param dropAction            Drop action to perform.
     * @param strMIMEType           MIME data type.
     * @param vaType                Qt's variant type of the MIME data.
     * @param vaData                Reference to QVariant where to store the retrieved data.
     */
    int                        sltGetData(Qt::DropAction dropAction, const QString &strMIMEType, QVariant::Type vaType, QVariant &vaData);

protected:

#ifdef DEBUG
    static void                debugOutputQt(QtMsgType type, const char *pszMsg);
#endif

protected:

    int                        dragStartInternal(const QStringList &lstFormats, Qt::DropAction defAction, Qt::DropActions actions);
    int                        retrieveDataInternal(Qt::DropAction dropAction, const QString &strMIMEType, QVector<uint8_t> &vecData);
    void                       setOpMode(DNDOPMODE enmMode);

protected:

    /** Pointer to UI session. */
    UISession        *m_pSession;
    /** Pointer to parent widget. */
    QWidget          *m_pParent;
    /** Drag and drop source instance. */
    CDnDSource        m_dndSource;
    /** Drag and drop target instance. */
    CDnDTarget        m_dndTarget;
    /** Current operation mode, indicating the transfer direction.
     *  Note: This is independent of the current drag and drop
     *        mode being set for this VM! */
    DNDOPMODE         m_enmOpMode;
    /** Current data from the source (if any).
     *  At the momenet we only support one source at a time. */
    UIDnDDataSource   m_dataSource;
    /** Flag indicating if a drag operation is pending currently. */
    bool              m_fIsPending;
    /** Flag indicating whether data has been retrieved from
     *  the guest already or not. */
    bool              m_fDataRetrieved;
    QMutex            m_ReadLock;
    QMutex            m_WriteLock;
    /** Data received from the guest. */
    QVector<uint8_t>  m_vecData;

#ifndef RT_OS_WINDOWS
    /** Pointer to MIMEData instance used for handling
     *  own MIME times on non-Windows host OSes. */
    UIDnDMIMEData    *m_pMIMEData;
    friend class UIDnDMIMEData;
#endif
};
#endif /* ___UIDnDHandler_h___ */

