/* $Id: UIThreadPool.h $ */
/** @file
 * VBox Qt GUI - UIThreadPool and UITask classes declaration.
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

#ifndef ___UIThreadPool_h___
#define ___UIThreadPool_h___

/* Qt includes: */
#include <QObject>
#include <QMutex>
#include <QQueue>
#include <QSet>
#include <QVariant>
#include <QVector>
#include <QWaitCondition>

/* Forward declarations: */
class UIThreadWorker;
class UITask;

/** QObject extension used as worker-thread pool.
  * Schedules COM-related GUI tasks to multiple worker-threads. */
class UIThreadPool : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies listeners about @a pTask complete. */
    void sigTaskComplete(UITask *pTask);

public:

    /** Constructs worker-thread pool.
      * @param cMaxWorkers          defines the maximum amount of worker-threads.
      * @param cMsWorkerIdleTimeout defines the maximum amount of time (in ms) which
      *                             pool will wait for the worker-thread on cleanup. */
    UIThreadPool(ulong cMaxWorkers = 3, ulong cMsWorkerIdleTimeout = 5000);
    /** Destructs worker-thread pool. */
    ~UIThreadPool();

    /** Returns whether the 'termination sequence' is started. */
    bool isTerminating() const;
    /** Defines that the 'termination sequence' is started. */
    void setTerminating();

    /** Enqueues @a pTask into the task-queue. */
    void enqueueTask(UITask *pTask);
    /** Returns dequeued top-most task from the task-queue.
      * @remarks Called by the @a pWorker passed as a hint. */
    UITask* dequeueTask(UIThreadWorker *pWorker);

private slots:

    /** Handles @a pTask complete signal. */
    void sltHandleTaskComplete(UITask *pTask);

    /** Handles @a pWorker finished signal. */
    void sltHandleWorkerFinished(UIThreadWorker *pWorker);

private:

    /** @name Worker-thread stuff.
     * @{ */
        /** Holds the maximum amount of time (in ms) which
          * pool will wait for the worker-thread on cleanup. */
        const ulong m_cMsIdleTimeout;
        /** Holds the vector of worker-threads. */
        QVector<UIThreadWorker*> m_workers;
        /** Holds the number of worker-threads.
          * @remarks We cannot use the vector size since it may contain 0 pointers. */
        int m_cWorkers;
        /** Holds the number of idle worker-threads. */
        int m_cIdleWorkers;
        /** Holds whether the 'termination sequence' is started
          * and all worker-threads should terminate ASAP. */
        bool m_fTerminating;
    /** @} */

    /** @name Task stuff
     * @{ */
        /** Holds the queue of pending tasks. */
        QQueue<UITask*> m_pendingTasks;
        /** Holds the set of executing tasks. */
        QSet<UITask*> m_executingTasks;
        /** Holds the condition variable that gets signalled when
          * queuing a new task and there are idle worker threads around.
          * @remarks Idle threads sits in dequeueTask waiting for this.
          *          Thus on thermination, setTerminating() will send a
          *          broadcast signal to wake up all workers (after
          *          setting m_fTerminating of course). */
        QWaitCondition m_taskCondition;
    /** @} */

    /** Holds the guard mutex object protecting
      * all the inter-thread variables. */
    mutable QMutex m_everythingLocker;
};

/** QObject extension used as worker-thread task interface.
  * Describes task to be executed by the UIThreadWorker object. */
class UITask : public QObject
{
    Q_OBJECT;

signals:

    /** Notifies listeners about @a pTask complete. */
    void sigComplete(UITask *pTask);

public:

    /** Task types. */
    enum Type
    {
        Type_MediumEnumeration = 1,
        Type_DetailsPopulation = 2,
    };

    /** Constructs the task of passed @a type. */
    UITask(UITask::Type type) : m_type(type) {}

    /** Returns the type of the task. */
    UITask::Type type() const { return m_type; }

    /** Starts the task. */
    void start();

protected:

    /** Contains the abstract task body.
      * @remarks To be reimplemented in sub-class. */
    virtual void run() = 0;

private:

    /** Holds the type of the task. */
    const UITask::Type m_type;
};

#endif /* !___UIThreadPool_h___ */

