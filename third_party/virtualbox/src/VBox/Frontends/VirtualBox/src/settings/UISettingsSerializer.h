/* $Id: UISettingsSerializer.h $ */
/** @file
 * VBox Qt GUI - UISettingsSerializer class declaration.
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

#ifndef ___UISettingsSerializer_h___
#define ___UISettingsSerializer_h___

/* Qt includes: */
#include <QThread>
#include <QVariant>
#include <QWaitCondition>
#include <QMutex>
#include <QList>
#include <QMap>

/* GUI includes: */
#include "QIWithRetranslateUI.h"
#include "QIDialog.h"

/* Forward declarations: */
class UISettingsPage;
class QProgressBar;
class QILabel;
class QLabel;

/* Type definitions: */
typedef QList<UISettingsPage*> UISettingsPageList;
typedef QMap<int, UISettingsPage*> UISettingsPageMap;

/** QThread reimplementation used for
  * loading/saving settings in async mode. */
class UISettingsSerializer : public QThread
{
    Q_OBJECT;

signals:

    /** Notifies listeners about process has been started. */
    void sigNotifyAboutProcessStarted();
    /** Notifies listeners about process reached @a iValue. */
    void sigNotifyAboutProcessProgressChanged(int iValue);
    /** Notifies listeners about process has been finished. */
    void sigNotifyAboutProcessFinished();

    /** Notifies GUI thread about some page was processed. */
    void sigNotifyAboutPageProcessed(int iPageId);
    /** Notifies GUI thread about all pages were processed. */
    void sigNotifyAboutPagesProcessed();

    /** Notifies listeners about particular operation progress change.
      * @param iOperations  holds the number of operations CProgress have,
      * @param strOperation holds the description of the current CProgress operation,
      * @param iOperation   holds the index of the current CProgress operation,
      * @param iPercent     holds the percentage of the current CProgress operation. */
    void sigOperationProgressChange(ulong iOperations, QString strOperation,
                                    ulong iOperation, ulong iPercent);

    /** Notifies listeners about particular COM error.
      * @param strErrorInfo holds the details of the error happened. */
    void sigOperationProgressError(QString strErrorInfo);

public:

    /** Serialization directions. */
    enum SerializationDirection { Load, Save };

    /** Constructor.
      * @param pParent   being passed to the base-class,
      * @param direction determines the load/save direction,
      * @param data      contains the wrapper(s) to load/save the data from/to,
      * @param pages     contains the page(s) to load/save the data to/from. */
    UISettingsSerializer(QObject *pParent, SerializationDirection direction,
                         const QVariant &data, const UISettingsPageList &pages);

    /** Destructor. */
    ~UISettingsSerializer();

    /** Returns the load/save direction. */
    SerializationDirection direction() const { return m_direction; }

    /** Returns the instance of wrapper(s) to load/save the data from/to. */
    QVariant& data() { return m_data; }

    /** Returns the count of the page(s) to load/save the data to/from. */
    int pageCount() const { return m_pages.size(); }

    /** Raises the priority of page with @a iPageId. */
    void raisePriorityOfPage(int iPageId);

public slots:

    /** Starts the process of data loading with passed @a priority. */
    void start(Priority priority = InheritPriority);

protected slots:

    /** Handles the fact of page with @a iPageId was processed. */
    void sltHandleProcessedPage(int iPageId);

    /** Handles the fact of all pages were processed. */
    void sltHandleProcessedPages();

protected:

    /** Worker-thread serialization rutine. */
    void run();

    /** Holds the load/save direction. */
    const SerializationDirection m_direction;

    /** Holds the wrapper(s) to load/save the data from/to. */
    QVariant m_data;
    /** Holds the page(s) to load/save the data to/from. */
    UISettingsPageMap m_pages;
    /** Holds the page(s) to load/save the data to/from for which that task was done. */
    UISettingsPageMap m_pagesDone;

    /** Holds whether the save was complete. */
    bool m_fSavingComplete;
    /** Holds the ID of the high priority page. */
    int m_iIdOfHighPriorityPage;
    /** Holds the synchronization mutex. */
    QMutex m_mutex;
    /** Holds the synchronization condition. */
    QWaitCondition m_condition;
};

/** QIDialog reimplementation used to
  * reflect the settings serialization operation. */
class UISettingsSerializerProgress : public QIWithRetranslateUI<QIDialog>
{
    Q_OBJECT;

signals:

    /** Asks itself for process start. */
    void sigAskForProcessStart();

public:

    /** Constructor.
      * @param pParent   being passed to the base-class,
      * @param direction determines the load/save direction,
      * @param data      contains the wrapper(s) to load/save the data from/to,
      * @param pages     contains the page(s) to load/save the data to/from. */
    UISettingsSerializerProgress(QWidget *pParent, UISettingsSerializer::SerializationDirection direction,
                                 const QVariant &data, const UISettingsPageList &pages);

    /** Executes the dialog. */
    int exec();

    /** Returns the instance of wrapper(s) to load/save the data from/to. */
    QVariant& data();

    /** Returns whether there were no errors. */
    bool isClean() const { return m_fClean; }

protected:

    /** Prepare routine. */
    void prepare();

    /** Translate routine: */
    void retranslateUi();

    /** Close event-handler called with the given window system @a pEvent. */
    virtual void closeEvent(QCloseEvent *pEvent);

private slots:

    /** Hides the modal dialog and sets the result code to <i>Rejected</i>. */
    virtual void reject();

    /** Starts the process. */
    void sltStartProcess();

    /** Handles process progress change to @a iValue. */
    void sltHandleProcessProgressChange(int iValue);

    /** Handles particular operation progress change.
      * @param iOperations  holds the number of operations CProgress have,
      * @param strOperation holds the description of the current CProgress operation,
      * @param iOperation   holds the index of the current CProgress operation,
      * @param iPercent     holds the percentage of the current CProgress operation. */
    void sltHandleOperationProgressChange(ulong iOperations, QString strOperation,
                                          ulong iOperation, ulong iPercent);

    /** Handles particular COM error.
      * @param strErrorInfo holds the details of the error happened. */
    void sltHandleOperationProgressError(QString strErrorInfo);

private:

    /** Holds the load/save direction. */
    const UISettingsSerializer::SerializationDirection m_direction;

    /** Holds the wrapper(s) to load/save the data from/to. */
    QVariant m_data;
    /** Holds the page(s) to load/save the data to/from. */
    UISettingsPageList m_pages;

    /** Holds the pointer to the thread loading/saving settings in async mode. */
    UISettingsSerializer *m_pSerializer;

    /** Holds the operation progress label. */
    QLabel *m_pLabelOperationProgress;
    /** Holds the operation progress bar. */
    QProgressBar *m_pBarOperationProgress;

    /** Holds the sub-operation progress label. */
    QILabel *m_pLabelSubOperationProgress;
    /** Holds the sub-operation progress bar. */
    QProgressBar *m_pBarSubOperationProgress;

    /** Holds whether there were no errors. */
    bool m_fClean;

    /** Holds the template for the sub-operation progress label. */
    static QString m_strProgressDescriptionTemplate;
};

#endif /* !___UISettingsSerializer_h___ */
