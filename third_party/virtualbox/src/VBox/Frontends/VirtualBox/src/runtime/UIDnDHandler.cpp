/* $Id: UIDnDHandler.cpp $ */
/** @file
 * VBox Qt GUI - UIDnDHandler class implementation.
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

#ifdef VBOX_WITH_PRECOMPILED_HEADERS
# include <precomp.h>
#else  /* !VBOX_WITH_PRECOMPILED_HEADERS */

/* Qt includes: */
#include <QApplication>
#include <QKeyEvent>
#include <QStringList>
#include <QTimer>
#include <QDrag>
#include <QUrl>

/* VirtualBox interface declarations: */
#ifndef VBOX_WITH_XPCOM
# include "VirtualBox.h"
#else
# include "VirtualBox_XPCOM.h"
#endif

/* GUI includes: */
# include "UIDnDHandler.h"
#ifdef VBOX_WITH_DRAG_AND_DROP_GH
# include "CDnDSource.h"
# ifdef RT_OS_WINDOWS
#  include "UIDnDDataObject_win.h"
#  include "UIDnDDropSource_win.h"
# endif
# include "UIDnDMIMEData.h"
#endif /* VBOX_WITH_DRAG_AND_DROP_GH */
#include "UIMessageCenter.h"
#include "UISession.h"

/* COM includes: */
# include "CConsole.h"
# include "CGuest.h"
# include "CGuestDnDSource.h"
# include "CGuestDnDTarget.h"
# include "CSession.h"

#endif /* !VBOX_WITH_PRECOMPILED_HEADERS */

#ifdef LOG_GROUP
 #undef LOG_GROUP
#endif
#define LOG_GROUP LOG_GROUP_GUEST_DND
#include <VBox/log.h>

#if 0
# ifdef DEBUG
#  include <QTextStream>
#  include <QFile>
/** Enable this to log debug output of a Qt debug build to a file defined by DEBUG_DND_QT_LOGFILE. */
#  define DEBUG_DND_QT
#  ifdef RT_OS_WINDOWS
#   define DEBUG_DND_QT_LOGFILE "c:\\temp\\VBoxQt.log"
#  else
#   define DEBUG_DND_QT_LOGFILE "/var/tmp/vboxqt.log"
#  endif
# endif /* DEBUG */
#endif


UIDnDHandler::UIDnDHandler(UISession *pSession, QWidget *pParent)
    : m_pSession(pSession)
    , m_pParent(pParent)
    , m_enmOpMode(DNDMODE_UNKNOWN)
    , m_fIsPending(false)
    , m_fDataRetrieved(false)
#ifndef RT_OS_WINDOWS
    , m_pMIMEData(NULL)
#endif
{
    AssertPtr(pSession);
    m_dndSource = static_cast<CDnDSource>(pSession->guest().GetDnDSource());
    m_dndTarget = static_cast<CDnDTarget>(pSession->guest().GetDnDTarget());
}

UIDnDHandler::~UIDnDHandler(void)
{
}

/*
 * Frontend -> Target.
 */

Qt::DropAction UIDnDHandler::dragEnter(ulong screenID, int x, int y,
                                       Qt::DropAction proposedAction, Qt::DropActions possibleActions,
                                       const QMimeData *pMimeData)
{
    LogFlowFunc(("enmOpMode=%RU32, screenID=%RU32, x=%d, y=%d, action=%ld\n",
                 m_enmOpMode, screenID, x, y, toVBoxDnDAction(proposedAction)));

    if (   m_enmOpMode != DNDMODE_UNKNOWN
        && m_enmOpMode != DNDMODE_HOSTTOGUEST)
        return Qt::IgnoreAction;

    /* Ask the guest for starting a DnD event. */
    KDnDAction result = m_dndTarget.Enter(screenID,
                                          x,
                                          y,
                                          toVBoxDnDAction(proposedAction),
                                          toVBoxDnDActions(possibleActions),
                                          pMimeData->formats().toVector());
    if (m_dndTarget.isOk())
    {
        setOpMode(DNDMODE_HOSTTOGUEST);
        return toQtDnDAction(result);
    }

    return Qt::IgnoreAction;
}

Qt::DropAction UIDnDHandler::dragMove(ulong screenID, int x, int y,
                                      Qt::DropAction proposedAction, Qt::DropActions possibleActions,
                                      const QMimeData *pMimeData)
{
    LogFlowFunc(("enmOpMode=%RU32, screenID=%RU32, x=%d, y=%d, action=%ld\n",
                 m_enmOpMode, screenID, x, y, toVBoxDnDAction(proposedAction)));

    if (m_enmOpMode != DNDMODE_HOSTTOGUEST)
        return Qt::IgnoreAction;

    /* Notify the guest that the mouse has been moved while doing
     * a drag'n drop operation. */
    KDnDAction result = m_dndTarget.Move(screenID,
                                         x,
                                         y,
                                         toVBoxDnDAction(proposedAction),
                                         toVBoxDnDActions(possibleActions),
                                         pMimeData->formats().toVector());
    if (m_dndTarget.isOk())
        return toQtDnDAction(result);

    return Qt::IgnoreAction;
}

Qt::DropAction UIDnDHandler::dragDrop(ulong screenID, int x, int y,
                                      Qt::DropAction proposedAction, Qt::DropActions possibleActions,
                                      const QMimeData *pMimeData)
{
    LogFlowFunc(("enmOpMode=%RU32, screenID=%RU32, x=%d, y=%d, action=%ld\n",
                 m_enmOpMode, screenID, x, y, toVBoxDnDAction(proposedAction)));

    if (m_enmOpMode != DNDMODE_HOSTTOGUEST)
        return Qt::IgnoreAction;

    /* The format the guest requests. */
    QString strFormat;
    /* Ask the guest for dropping data. */
    KDnDAction enmResult = m_dndTarget.Drop(screenID,
                                            x,
                                            y,
                                            toVBoxDnDAction(proposedAction),
                                            toVBoxDnDActions(possibleActions),
                                            pMimeData->formats().toVector(), strFormat);

    /* Has the guest accepted the drop event? */
    if (   m_dndTarget.isOk()
        && enmResult != KDnDAction_Ignore)
    {
        LogFlowFunc(("strFormat=%s ...\n", strFormat.toUtf8().constData()));

        QByteArray arrBytes;

        /*
         * Does the host support the format requested by the guest?
         * Lookup the format in the MIME data object.
         */
        AssertPtr(pMimeData);
        if (pMimeData->formats().indexOf(strFormat) >= 0)
        {
            arrBytes = pMimeData->data(strFormat);
            Assert(!arrBytes.isEmpty());
        }
        /*
         * The host does not support the format requested by the guest.
         * This can happen if the host wants to send plan text, for example, but
         * the guest requested something else, e.g. an URI list.
         *
         * In that case dictate the guest to use a fixed format from the host,
         * so instead returning the requested URI list, return the original
         * data format from the host. The guest has to try to deal with that then.
         **/
        else
        {
            LogRel3(("DnD: Guest requested a different format '%s'\n", strFormat.toUtf8().constData()));
            LogRel3(("DnD: The host offered:\n"));
#if 0
            for (QStringList::iterator itFmt  = pMimeData->formats().begin();
                                       itFmt != pMimeData->formats().end(); itFmt++)
            {
                QString strTemp = *itFmt;
                LogRel3(("DnD: \t%s\n", strTemp.toUtf8().constData()));
            }
#endif
            if (pMimeData->hasText())
            {
                LogRel3(("DnD: Converting data to text ...\n"));
                arrBytes  = pMimeData->text().toUtf8();
                strFormat = "text/plain;charset=utf-8";
            }
            else
            {
                LogRel(("DnD: Error: Could not convert host format to guest format\n"));
                enmResult = KDnDAction_Ignore;
            }
        }

        if (arrBytes.size()) /* Anything to send? */
        {
            /* Convert data to a vector. */
            QVector<uint8_t> vecData(arrBytes.size()); /** @todo Can this throw or anything? */
            AssertReleaseMsg(vecData.size() == arrBytes.size(), ("Drag and drop format buffer size does not match"));
            memcpy(vecData.data(), arrBytes.constData(), arrBytes.size());

            /* Send data to the guest. */
            LogRel3(("DnD: Host is sending %d bytes of data as '%s'\n", vecData.size(), strFormat.toUtf8().constData()));
            CProgress progress = m_dndTarget.SendData(screenID, strFormat, vecData);

            if (m_dndTarget.isOk())
            {
                msgCenter().showModalProgressDialog(progress,
                                                    tr("Dropping data ..."), ":/progress_dnd_hg_90px.png",
                                                    m_pParent);

                LogFlowFunc(("Transfer fCompleted=%RTbool, fCanceled=%RTbool, hr=%Rhrc\n",
                             progress.GetCompleted(), progress.GetCanceled(), progress.GetResultCode()));

                BOOL fCanceled = progress.GetCanceled();
                if (   !fCanceled
                    && (   !progress.isOk()
                        ||  progress.GetResultCode() != 0))
                {
                    msgCenter().cannotDropDataToGuest(progress, m_pParent);
                    enmResult = KDnDAction_Ignore;
                }
            }
            else
            {
                msgCenter().cannotDropDataToGuest(m_dndTarget, m_pParent);
                enmResult = KDnDAction_Ignore;
            }
        }
        else /* Error. */
            enmResult = KDnDAction_Ignore;
    }

    /*
     * Since the mouse button has been release this in any case marks
     * the end of the current transfer direction. So reset the current
     * mode as well here.
     */
    setOpMode(DNDMODE_UNKNOWN);

    return toQtDnDAction(enmResult);
}

void UIDnDHandler::dragLeave(ulong screenID)
{
    LogFlowFunc(("enmOpMode=%RU32, screenID=%RU32\n", m_enmOpMode, screenID));

    if (m_enmOpMode == DNDMODE_HOSTTOGUEST)
    {
        m_dndTarget.Leave(screenID);
        setOpMode(DNDMODE_UNKNOWN);
    }
}

#ifdef DEBUG_DND_QT
QTextStream *g_pStrmLogQt = NULL; /* Output stream for Qt debug logging. */

/* static */
void UIDnDHandler::debugOutputQt(QtMsgType type, const QMessageLogContext &context, const QString &strMessage)
{
    QString strMsg;
    switch (type)
    {
    case QtWarningMsg:
        strMsg += "[W]";
        break;
    case QtCriticalMsg:
        strMsg += "[C]";
        break;
    case QtFatalMsg:
        strMsg += "[F]";
        break;
    case QtDebugMsg:
    default:
        strMsg += "[D]";
        break;
    }

    if (g_pStrmLogQt)
        (*g_pStrmLogQt) << strMsg << " " << strMessage << endl;
}
#endif /* DEBUG_DND_QT */

/*
 * Source -> Frontend.
 */

int UIDnDHandler::dragStartInternal(const QStringList &lstFormats,
                                    Qt::DropAction defAction, Qt::DropActions actions)
{
    RT_NOREF(defAction);
    int rc = VINF_SUCCESS;

#ifdef VBOX_WITH_DRAG_AND_DROP_GH

    LogFlowFunc(("defAction=0x%x\n", defAction));
    LogFlowFunc(("Number of formats: %d\n", lstFormats.size()));
# ifdef DEBUG
    for (int i = 0; i < lstFormats.size(); i++)
        LogFlowFunc(("\tFormat %d: %s\n", i, lstFormats.at(i).toUtf8().constData()));
# endif

# ifdef DEBUG_DND_QT
    QFile *pFileDebugQt = new QFile(DEBUG_DND_QT_LOGFILE);
    if (pFileDebugQt->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
    {
        g_pStrmLogQt = new QTextStream(pFileDebugQt);

        qInstallMessageHandler(UIDnDHandler::debugOutputQt);
        qDebug("========================================================================");
    }
# endif

# ifdef RT_OS_WINDOWS
    UIDnDDataObject *pDataObject = new UIDnDDataObject(this, lstFormats);
    if (!pDataObject)
        return VERR_NO_MEMORY;
    UIDnDDropSource *pDropSource = new UIDnDDropSource(m_pParent, pDataObject);
    if (!pDropSource)
        return VERR_NO_MEMORY;

    DWORD dwOKEffects = DROPEFFECT_NONE;
    if (actions)
    {
        if (actions & Qt::CopyAction)
            dwOKEffects |= DROPEFFECT_COPY;
        if (actions & Qt::MoveAction)
            dwOKEffects |= DROPEFFECT_MOVE;
        if (actions & Qt::LinkAction)
            dwOKEffects |= DROPEFFECT_LINK;
    }

    DWORD dwEffect;
    LogRel2(("DnD: Starting drag and drop operation\n", dwOKEffects));
    LogRel3(("DnD: DoDragDrop dwOKEffects=0x%x\n", dwOKEffects));
    HRESULT hr = ::DoDragDrop(pDataObject, pDropSource, dwOKEffects, &dwEffect);
    LogRel3(("DnD: DoDragDrop ended with hr=%Rhrc, dwEffect=%RI32\n", hr, dwEffect));

    if (pDropSource)
        pDropSource->Release();
    if (pDataObject)
        pDataObject->Release();

# else /* !RT_OS_WINDOWS */

    QDrag *pDrag = new QDrag(m_pParent);
    if (!pDrag)
        return VERR_NO_MEMORY;

    /* Note: pMData is transferred to the QDrag object, so no need for deletion. */
    m_pMIMEData = new UIDnDMIMEData(this, lstFormats, defAction, actions);
    if (!m_pMIMEData)
    {
        delete pDrag;
        return VERR_NO_MEMORY;
    }

    /* Inform the MIME data object of any changes in the current action. */
    connect(pDrag, SIGNAL(actionChanged(Qt::DropAction)),
            m_pMIMEData, SLOT(sltDropActionChanged(Qt::DropAction)));

    /* Invoke this handler as data needs to be retrieved by our derived QMimeData class. */
    connect(m_pMIMEData, SIGNAL(sigGetData(Qt::DropAction, const QString&, QVariant::Type, QVariant&)),
            this, SLOT(sltGetData(Qt::DropAction, const QString&, QVariant::Type, QVariant&)));

    /*
     * Set MIME data object and start the (modal) drag'n drop operation on the host.
     * This does not block Qt's event loop, however (on Windows it would).
     */
    pDrag->setMimeData(m_pMIMEData);
    LogFlowFunc(("Executing modal drag'n drop operation ...\n"));

    Qt::DropAction dropAction;
#  ifdef RT_OS_DARWIN
#    ifdef VBOX_WITH_DRAG_AND_DROP_PROMISES
        dropAction = pDrag->exec(actions, defAction);
#    else
        /* Without having VBOX_WITH_DRAG_AND_DROP_PROMISES enabled drag and drop
         * will not work on OS X! It also requires some handcrafted patches within Qt
         * (which also needs VBOX_WITH_DRAG_AND_DROP_PROMISES set there). */
        dropAction = Qt::IgnoreAction;
        rc = VERR_NOT_SUPPORTED;
#    endif
#  else /* !RT_OS_DARWIN */
    dropAction = pDrag->exec(actions, defAction);
#  endif /* RT_OS_DARWIN */
    LogRel3(("DnD: Ended with dropAction=%ld\n", UIDnDHandler::toVBoxDnDAction(dropAction)));

    /* Note: The UIDnDMimeData object will not be not accessible here anymore,
     *       since QDrag had its ownership and deleted it after the (blocking)
     *       QDrag::exec() call. */

    /* pDrag will be cleaned up by Qt automatically. */

# endif /* !RT_OS_WINDOWS */

    reset();

#ifdef DEBUG_DND_QT
    if (g_pStrmLogQt)
    {
        delete g_pStrmLogQt;
        g_pStrmLogQt = NULL;
    }

    if (pFileDebugQt)
    {
        pFileDebugQt->close();
        delete pFileDebugQt;
    }
#endif /* DEBUG_DND_QT */

#else /* !VBOX_WITH_DRAG_AND_DROP_GH */

    rc = VERR_NOT_SUPPORTED;

#endif /* VBOX_WITH_DRAG_AND_DROP_GH */

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int UIDnDHandler::dragCheckPending(ulong screenID)
{
    int rc;
#ifdef VBOX_WITH_DRAG_AND_DROP_GH

    LogFlowFunc(("enmOpMode=%RU32, fIsPending=%RTbool, screenID=%RU32\n", m_enmOpMode, m_fIsPending, screenID));

    {
        QMutexLocker AutoReadLock(&m_ReadLock);

        if (   m_enmOpMode != DNDMODE_UNKNOWN
            && m_enmOpMode != DNDMODE_GUESTTOHOST) /* Wrong mode set? */
            return VINF_SUCCESS;

        if (m_fIsPending) /* Pending operation is in progress. */
            return VINF_SUCCESS;
    }

    QMutexLocker AutoWriteLock(&m_WriteLock);
    m_fIsPending = true;
    AutoWriteLock.unlock();

    /**
     * How this works: Source is asking the target if there is any DnD
     * operation pending, when the mouse leaves the guest window. On
     * return there is some info about a running DnD operation
     * (or defaultAction is KDnDAction_Ignore if not). With
     * this information we create a Qt QDrag object with our own QMimeType
     * implementation and call exec.
     *
     * Note: This function *blocks* until the actual drag'n drop operation
     *       has been finished (successfully or not)!
     */
    CGuest guest = m_pSession->guest();

    /* Clear our current data set. */
    m_dataSource.lstFormats.clear();
    m_dataSource.vecActions.clear();

    /* Ask the guest if there is a drag and drop operation pending (on the guest). */
    QVector<QString> vecFormats;
    m_dataSource.defaultAction = m_dndSource.DragIsPending(screenID, vecFormats, m_dataSource.vecActions);

    LogRelMax3(10, ("DnD: Default action is: 0x%x\n", m_dataSource.defaultAction));
    LogRelMax3(10, ("DnD: Number of supported guest actions: %d\n", m_dataSource.vecActions.size()));
        for (int i = 0; i < m_dataSource.vecActions.size(); i++)
            LogRelMax3(10, ("DnD: \tAction %d: 0x%x\n", i, m_dataSource.vecActions.at(i)));

    LogRelMax3(10, ("DnD: Number of supported guest formats: %d\n", vecFormats.size()));
        for (int i = 0; i < vecFormats.size(); i++)
        {
            const QString &strFmtGuest = vecFormats.at(i);
            LogRelMax3(10, ("DnD: \tFormat %d: %s\n", i, strFmtGuest.toUtf8().constData()));
        }

    LogFlowFunc(("defaultAction=0x%x, vecFormatsSize=%d, vecActionsSize=%d\n",
                 m_dataSource.defaultAction, vecFormats.size(), m_dataSource.vecActions.size()));

    if (   m_dataSource.defaultAction != KDnDAction_Ignore
        && vecFormats.size())
    {
        for (int i = 0; i < vecFormats.size(); i++)
        {
            const QString &strFormat = vecFormats.at(i);
            m_dataSource.lstFormats << strFormat;
        }

        rc = VINF_SUCCESS; /* There's a valid pending drag and drop operation on the guest. */
    }
    else /* No format data from the guest arrived yet. */
        rc = VERR_NO_DATA;

    AutoWriteLock.relock();
    m_fIsPending = false;
    AutoWriteLock.unlock();

#else /* !VBOX_WITH_DRAG_AND_DROP_GH */

    NOREF(screenID);

    rc = VERR_NOT_SUPPORTED;

#endif /* VBOX_WITH_DRAG_AND_DROP_GH */

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int UIDnDHandler::dragStart(ulong screenID)
{
    int rc;
#ifdef VBOX_WITH_DRAG_AND_DROP_GH

    NOREF(screenID);

    LogFlowFuncEnter();

    /* Sanity checks. */
    if (   !m_dataSource.lstFormats.size()
        ||  m_dataSource.defaultAction == KDnDAction_Ignore
        || !m_dataSource.vecActions.size())
    {
        return VERR_INVALID_PARAMETER;
    }

    setOpMode(DNDMODE_GUESTTOHOST);

    rc = dragStartInternal(m_dataSource.lstFormats,
                           toQtDnDAction(m_dataSource.defaultAction), toQtDnDActions(m_dataSource.vecActions));

#else /* !VBOX_WITH_DRAG_AND_DROP_GH */

    NOREF(screenID);

    rc = VERR_NOT_SUPPORTED;

#endif /* VBOX_WITH_DRAG_AND_DROP_GH */

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int UIDnDHandler::dragStop(ulong screenID)
{
    int rc;
#ifdef VBOX_WITH_DRAG_AND_DROP_GH

    NOREF(screenID);

    reset();
    rc = VINF_SUCCESS;

#else /* !VBOX_WITH_DRAG_AND_DROP_GH */

    NOREF(screenID);

    rc = VERR_NOT_SUPPORTED;

#endif /* VBOX_WITH_DRAG_AND_DROP_GH */

    LogFlowFuncLeaveRC(rc);
    return rc;
}

void UIDnDHandler::reset(void)
{
    LogFlowFuncEnter();

    m_fDataRetrieved = false;
    m_fIsPending = false;

    setOpMode(DNDMODE_UNKNOWN);
}

int UIDnDHandler::retrieveData(Qt::DropAction          dropAction,
                               const QString          &strMIMEType,
                                     QVector<uint8_t> &vecData)
{
    if (!strMIMEType.compare("application/x-qt-mime-type-name", Qt::CaseInsensitive))
        return VINF_SUCCESS;

    int rc;
    if (!m_fDataRetrieved)
    {
        /*
         * Retrieve the actual data from the guest.
         */
        rc = retrieveDataInternal(dropAction, strMIMEType, m_vecData);
        if (RT_FAILURE(rc))
            LogRel3(("DnD: Receiving data failed: %Rrc\n", rc));
        else
            m_fDataRetrieved = true;
    }
    else /* Data already received, supply cached version. */
        rc = VINF_SUCCESS;

    if (RT_SUCCESS(rc))
        vecData = m_vecData;

    return rc;
}

int UIDnDHandler::retrieveData(      Qt::DropAction  dropAction,
                               const QString        &strMIMEType,
                                     QVariant::Type  vaType,
                                     QVariant       &vaData)
{
    QVector<uint8_t> vecData;
    int rc = retrieveData(dropAction, strMIMEType, vecData);
    if (RT_SUCCESS(rc))
    {
        /* If no/an invalid variant is set, try to guess the variant type.
         * This can happen on OS X. */
        if (vaType == QVariant::Invalid)
            vaType = UIDnDMIMEData::getVariantType(strMIMEType);

        rc = UIDnDMIMEData::getDataAsVariant(vecData, strMIMEType, vaType, vaData);
    }

    LogFlowFuncLeaveRC(rc);
    return rc;
}

int UIDnDHandler::retrieveDataInternal(      Qt::DropAction    dropAction,
                                       const QString          &strMIMEType,
                                             QVector<uint8_t> &vecData)
{
    LogRel3(("DnD: Retrieving data from guest as '%s' (%d)\n", qPrintable(strMIMEType), dropAction));

    int rc = VINF_SUCCESS;

    /* Indicate to the guest that we have dropped the data on the host.
     * The guest then will initiate the actual "drop" operation into our proxy on the guest. */
    Assert(!m_dndSource.isNull());
    CProgress progress = m_dndSource.Drop(strMIMEType,
                                          UIDnDHandler::toVBoxDnDAction(dropAction));
    LogFlowFunc(("Source: isOk=%RTbool\n", m_dndSource.isOk()));
    if (m_dndSource.isOk())
    {
        /* Send a mouse event with released mouse buttons into the guest that triggers
         * the "drop" event in our proxy window on the guest. */
        AssertPtr(m_pSession);
        m_pSession->mouse().PutMouseEvent(0, 0, 0, 0, 0);

        msgCenter().showModalProgressDialog(progress,
                                            tr("Retrieving data ..."), ":/progress_dnd_gh_90px.png",
                                            m_pParent);

        LogFlowFunc(("Progress: fCanceled=%RTbool, fCompleted=%RTbool, isOk=%RTbool, hrc=%Rhrc\n",
                     progress.GetCanceled(), progress.GetCompleted(), progress.isOk(), progress.GetResultCode()));

        if (!progress.GetCanceled())
        {
            rc =   (   progress.isOk()
                    && progress.GetResultCode() == 0)
                 ? VINF_SUCCESS : VERR_GENERAL_FAILURE; /** @todo Fudge; do a GetResultCode() to rc translation. */

            if (RT_SUCCESS(rc))
            {
                /* After we successfully retrieved data from the source we query it from Main. */
                vecData = m_dndSource.ReceiveData(); /** @todo QVector.size() is "int" only!? */
                if (m_dndSource.isOk())
                {
                    if (vecData.isEmpty())
                        rc = VERR_NO_DATA;
                }
                else
                {
                    msgCenter().cannotDropDataToHost(m_dndSource, m_pParent);
                    rc = VERR_GENERAL_FAILURE; /** @todo Fudge; do a GetResultCode() to rc translation. */
                }
            }
            else
                msgCenter().cannotDropDataToHost(progress, m_pParent);
        }
        else /* Don't pop up a message. */
            rc = VERR_CANCELLED;
    }
    else
    {
        msgCenter().cannotDropDataToHost(m_dndSource, m_pParent);
        rc = VERR_GENERAL_FAILURE; /** @todo Fudge; do a GetResultCode() to rc translation. */
    }

    setOpMode(DNDMODE_UNKNOWN);

    LogFlowFuncLeaveRC(rc);
    return rc;
}

void UIDnDHandler::setOpMode(DNDOPMODE enmMode)
{
    QMutexLocker AutoWriteLock(&m_WriteLock);
    m_enmOpMode = enmMode;
    LogFunc(("Operation mode is now: %RU32\n", m_enmOpMode));
}

int UIDnDHandler::sltGetData(      Qt::DropAction  dropAction,
                             const QString        &strMIMEType,
                                   QVariant::Type  vaType,
                                   QVariant       &vaData)
{
    int rc = retrieveData(dropAction, strMIMEType, vaType, vaData);
    LogFlowFuncLeaveRC(rc);
    return rc;
}

/*
 * Drag and Drop helper methods
 */

/* static */
KDnDAction UIDnDHandler::toVBoxDnDAction(Qt::DropAction action)
{
    if (action == Qt::CopyAction)
        return KDnDAction_Copy;
    if (action == Qt::MoveAction)
        return KDnDAction_Move;
    if (action == Qt::LinkAction)
        return KDnDAction_Link;

    return KDnDAction_Ignore;
}

/* static */
QVector<KDnDAction> UIDnDHandler::toVBoxDnDActions(Qt::DropActions actions)
{
    QVector<KDnDAction> vbActions;
    if (actions.testFlag(Qt::IgnoreAction))
        vbActions << KDnDAction_Ignore;
    if (actions.testFlag(Qt::CopyAction))
        vbActions << KDnDAction_Copy;
    if (actions.testFlag(Qt::MoveAction))
        vbActions << KDnDAction_Move;
    if (actions.testFlag(Qt::LinkAction))
        vbActions << KDnDAction_Link;

    return vbActions;
}

/* static */
Qt::DropAction UIDnDHandler::toQtDnDAction(KDnDAction action)
{
    Qt::DropAction dropAct = Qt::IgnoreAction;
    if (action == KDnDAction_Copy)
        dropAct = Qt::CopyAction;
    if (action == KDnDAction_Move)
        dropAct = Qt::MoveAction;
    if (action == KDnDAction_Link)
        dropAct = Qt::LinkAction;

    LogFlowFunc(("dropAct=0x%x\n", dropAct));
    return dropAct;
}

/* static */
Qt::DropActions UIDnDHandler::toQtDnDActions(const QVector<KDnDAction> &vecActions)
{
    Qt::DropActions dropActs = Qt::IgnoreAction;
    for (int i = 0; i < vecActions.size(); i++)
    {
        switch (vecActions.at(i))
        {
            case KDnDAction_Ignore:
                dropActs |= Qt::IgnoreAction;
                break;
            case KDnDAction_Copy:
                dropActs |= Qt::CopyAction;
                break;
            case KDnDAction_Move:
                dropActs |= Qt::MoveAction;
                break;
            case KDnDAction_Link:
                dropActs |= Qt::LinkAction;
                break;
            default:
                break;
        }
    }

    LogFlowFunc(("dropActions=0x%x\n", int(dropActs)));
    return dropActs;
}

