/* $Id: PerformanceImpl.cpp $ */

/** @file
 *
 * VBox Performance API COM Classes implementation
 */

/*
 * Copyright (C) 2008-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

/*
 * Rules of engagement:
 * 1) All performance objects must be destroyed by PerformanceCollector only!
 * 2) All public methods of PerformanceCollector must be protected with
 *    read or write lock.
 * 3) samplerCallback only uses the write lock during the third phase
 *    which pulls data into SubMetric objects. This is where object destruction
 *    and all list modifications are done. The pre-collection phases are
 *    run without any locks which is only possible because:
 * 4) Public methods of PerformanceCollector as well as pre-collection methods
      cannot modify lists or destroy objects, and:
 * 5) Pre-collection methods cannot modify metric data.
 */

#include "PerformanceImpl.h"

#include "AutoCaller.h"
#include "Logging.h"

#include <iprt/process.h>

#include <VBox/err.h>
#include <VBox/settings.h>

#include <vector>
#include <algorithm>
#include <functional>

#include "Performance.h"

static const char *g_papcszMetricNames[] =
{
    "CPU/Load/User",
    "CPU/Load/User:avg",
    "CPU/Load/User:min",
    "CPU/Load/User:max",
    "CPU/Load/Kernel",
    "CPU/Load/Kernel:avg",
    "CPU/Load/Kernel:min",
    "CPU/Load/Kernel:max",
    "CPU/Load/Idle",
    "CPU/Load/Idle:avg",
    "CPU/Load/Idle:min",
    "CPU/Load/Idle:max",
    "CPU/MHz",
    "CPU/MHz:avg",
    "CPU/MHz:min",
    "CPU/MHz:max",
    "Net/*/Load/Rx",
    "Net/*/Load/Rx:avg",
    "Net/*/Load/Rx:min",
    "Net/*/Load/Rx:max",
    "Net/*/Load/Tx",
    "Net/*/Load/Tx:avg",
    "Net/*/Load/Tx:min",
    "Net/*/Load/Tx:max",
    "RAM/Usage/Total",
    "RAM/Usage/Total:avg",
    "RAM/Usage/Total:min",
    "RAM/Usage/Total:max",
    "RAM/Usage/Used",
    "RAM/Usage/Used:avg",
    "RAM/Usage/Used:min",
    "RAM/Usage/Used:max",
    "RAM/Usage/Free",
    "RAM/Usage/Free:avg",
    "RAM/Usage/Free:min",
    "RAM/Usage/Free:max",
    "RAM/VMM/Used",
    "RAM/VMM/Used:avg",
    "RAM/VMM/Used:min",
    "RAM/VMM/Used:max",
    "RAM/VMM/Free",
    "RAM/VMM/Free:avg",
    "RAM/VMM/Free:min",
    "RAM/VMM/Free:max",
    "RAM/VMM/Ballooned",
    "RAM/VMM/Ballooned:avg",
    "RAM/VMM/Ballooned:min",
    "RAM/VMM/Ballooned:max",
    "RAM/VMM/Shared",
    "RAM/VMM/Shared:avg",
    "RAM/VMM/Shared:min",
    "RAM/VMM/Shared:max",
    "Guest/CPU/Load/User",
    "Guest/CPU/Load/User:avg",
    "Guest/CPU/Load/User:min",
    "Guest/CPU/Load/User:max",
    "Guest/CPU/Load/Kernel",
    "Guest/CPU/Load/Kernel:avg",
    "Guest/CPU/Load/Kernel:min",
    "Guest/CPU/Load/Kernel:max",
    "Guest/CPU/Load/Idle",
    "Guest/CPU/Load/Idle:avg",
    "Guest/CPU/Load/Idle:min",
    "Guest/CPU/Load/Idle:max",
    "Guest/RAM/Usage/Total",
    "Guest/RAM/Usage/Total:avg",
    "Guest/RAM/Usage/Total:min",
    "Guest/RAM/Usage/Total:max",
    "Guest/RAM/Usage/Free",
    "Guest/RAM/Usage/Free:avg",
    "Guest/RAM/Usage/Free:min",
    "Guest/RAM/Usage/Free:max",
    "Guest/RAM/Usage/Balloon",
    "Guest/RAM/Usage/Balloon:avg",
    "Guest/RAM/Usage/Balloon:min",
    "Guest/RAM/Usage/Balloon:max",
    "Guest/RAM/Usage/Shared",
    "Guest/RAM/Usage/Shared:avg",
    "Guest/RAM/Usage/Shared:min",
    "Guest/RAM/Usage/Shared:max",
    "Guest/RAM/Usage/Cache",
    "Guest/RAM/Usage/Cache:avg",
    "Guest/RAM/Usage/Cache:min",
    "Guest/RAM/Usage/Cache:max",
    "Guest/Pagefile/Usage/Total",
    "Guest/Pagefile/Usage/Total:avg",
    "Guest/Pagefile/Usage/Total:min",
    "Guest/Pagefile/Usage/Total:max",
};

////////////////////////////////////////////////////////////////////////////////
// PerformanceCollector class
////////////////////////////////////////////////////////////////////////////////

// constructor / destructor
////////////////////////////////////////////////////////////////////////////////

PerformanceCollector::PerformanceCollector()
  : mMagic(0), mUnknownGuest("unknown guest")
{
}

PerformanceCollector::~PerformanceCollector() {}

HRESULT PerformanceCollector::FinalConstruct()
{
    LogFlowThisFunc(("\n"));

    return BaseFinalConstruct();
}

void PerformanceCollector::FinalRelease()
{
    LogFlowThisFunc(("\n"));
    BaseFinalRelease();
}

// public initializer/uninitializer for internal purposes only
////////////////////////////////////////////////////////////////////////////////

/**
 * Initializes the PerformanceCollector object.
 */
HRESULT PerformanceCollector::init()
{
    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    LogFlowThisFuncEnter();

    HRESULT rc = S_OK;

    m.hal = pm::createHAL();
    m.gm = new pm::CollectorGuestManager;

    /* Let the sampler know it gets a valid collector.  */
    mMagic = PERFORMANCE_METRIC_MAGIC;

    /* Start resource usage sampler */
    int vrc = RTTimerLRCreate(&m.sampler, VBOX_USAGE_SAMPLER_MIN_INTERVAL,
                              &PerformanceCollector::staticSamplerCallback, this);
    AssertMsgRC(vrc, ("Failed to create resource usage sampling timer(%Rra)\n", vrc));
    if (RT_FAILURE(vrc))
        rc = E_FAIL;

    if (SUCCEEDED(rc))
        autoInitSpan.setSucceeded();

    LogFlowThisFuncLeave();

    return rc;
}

/**
 * Uninitializes the PerformanceCollector object.
 *
 * Called either from FinalRelease() or by the parent when it gets destroyed.
 */
void PerformanceCollector::uninit()
{
    LogFlowThisFuncEnter();

    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
    {
        LogFlowThisFunc(("Already uninitialized.\n"));
        LogFlowThisFuncLeave();
        return;
    }

    /* Destroy unregistered metrics */
    BaseMetricList::iterator it;
    for (it = m.baseMetrics.begin(); it != m.baseMetrics.end();)
        if ((*it)->isUnregistered())
        {
            delete *it;
            it = m.baseMetrics.erase(it);
        }
        else
            ++it;
    Assert(m.baseMetrics.size() == 0);
    /*
     * Now when we have destroyed all base metrics that could
     * try to pull data from unregistered CollectorGuest objects
     * it is safe to destroy them as well.
     */
    m.gm->destroyUnregistered();

    /* Destroy resource usage sampler */
    int vrc = RTTimerLRDestroy(m.sampler);
    AssertMsgRC(vrc, ("Failed to destroy resource usage sampling timer (%Rra)\n", vrc));
    m.sampler = NULL;

    /* Invalidate the magic now. */
    mMagic = 0;

    //delete m.factory;
    //m.factory = NULL;

    delete m.gm;
    m.gm = NULL;
    delete m.hal;
    m.hal = NULL;

    LogFlowThisFuncLeave();
}

// IPerformanceCollector properties
////////////////////////////////////////////////////////////////////////////////

HRESULT PerformanceCollector::getMetricNames(std::vector<com::Utf8Str> &aMetricNames)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aMetricNames.resize(RT_ELEMENTS(g_papcszMetricNames));
    for (size_t i = 0; i < RT_ELEMENTS(g_papcszMetricNames); i++)
        aMetricNames[i] = g_papcszMetricNames[i];

    return S_OK;
}

// IPerformanceCollector methods
////////////////////////////////////////////////////////////////////////////////

HRESULT PerformanceCollector::toIPerformanceMetric(pm::Metric *src, ComPtr<IPerformanceMetric> &dst)
{
    ComObjPtr<PerformanceMetric> metric;
    HRESULT rc = metric.createObject();
    if (SUCCEEDED(rc))
        rc = metric->init(src);
    AssertComRCReturnRC(rc);
    dst = metric;
    return rc;
}

HRESULT PerformanceCollector::toIPerformanceMetric(pm::BaseMetric *src, ComPtr<IPerformanceMetric> &dst)
{
    ComObjPtr<PerformanceMetric> metric;
    HRESULT rc = metric.createObject();
    if (SUCCEEDED(rc))
        rc = metric->init(src);
    AssertComRCReturnRC(rc);
    dst = metric;
    return rc;
}

const Utf8Str& PerformanceCollector::getFailedGuestName()
{
    pm::CollectorGuest *pGuest = m.gm->getBlockedGuest();
    if (pGuest)
        return pGuest->getVMName();
    return mUnknownGuest;
}

HRESULT PerformanceCollector::getMetrics(const std::vector<com::Utf8Str> &aMetricNames,
                                         const std::vector<ComPtr<IUnknown> > &aObjects,
                                         std::vector<ComPtr<IPerformanceMetric> > &aMetrics)
{
    HRESULT rc = S_OK;

    pm::Filter filter(aMetricNames, aObjects);

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    MetricList filteredMetrics;
    MetricList::iterator it;
    for (it = m.metrics.begin(); it != m.metrics.end(); ++it)
        if (filter.match((*it)->getObject(), (*it)->getName()))
            filteredMetrics.push_back(*it);

    aMetrics.resize(filteredMetrics.size());
    int i = 0;
    for (it = filteredMetrics.begin(); it != filteredMetrics.end(); ++it)
    {
        ComObjPtr<PerformanceMetric> metric;
        rc = metric.createObject();
        if (SUCCEEDED(rc))
            rc = metric->init(*it);
        AssertComRCReturnRC(rc);
        LogFlow(("PerformanceCollector::GetMetrics() store a metric at retMetrics[%d]...\n", i));
        aMetrics[i++] = metric;
    }
    return rc;
}

HRESULT PerformanceCollector::setupMetrics(const std::vector<com::Utf8Str> &aMetricNames,
                                           const std::vector<ComPtr<IUnknown> > &aObjects,
                                           ULONG aPeriod,
                                           ULONG aCount,
                                           std::vector<ComPtr<IPerformanceMetric> > &aAffectedMetrics)
{
    pm::Filter filter(aMetricNames, aObjects);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);

    HRESULT rc = S_OK;
    BaseMetricList filteredMetrics;
    BaseMetricList::iterator it;
    for (it = m.baseMetrics.begin(); it != m.baseMetrics.end(); ++it)
        if (filter.match((*it)->getObject(), (*it)->getName()))
        {
            LogFlow(("PerformanceCollector::SetupMetrics() setting period to %u, count to %u for %s\n",
                     aPeriod, aCount, (*it)->getName()));
            (*it)->init(aPeriod, aCount);
            if (aPeriod == 0 || aCount == 0)
            {
                LogFlow(("PerformanceCollector::SetupMetrics() disabling %s\n",
                         (*it)->getName()));
                rc = (*it)->disable();
                if (FAILED(rc))
                    break;
            }
            else
            {
                LogFlow(("PerformanceCollector::SetupMetrics() enabling %s\n",
                         (*it)->getName()));
                rc = (*it)->enable();
                if (FAILED(rc))
                    break;
            }
            filteredMetrics.push_back(*it);
        }

    aAffectedMetrics.resize(filteredMetrics.size());
    int i = 0;
    for (it = filteredMetrics.begin();
         it != filteredMetrics.end() && SUCCEEDED(rc); ++it)
        rc = toIPerformanceMetric(*it, aAffectedMetrics[i++]);

    if (FAILED(rc))
        return setError(E_FAIL, "Failed to setup metrics for '%s'",
                        getFailedGuestName().c_str());
    return rc;
}

HRESULT PerformanceCollector::enableMetrics(const std::vector<com::Utf8Str> &aMetricNames,
                                            const std::vector<ComPtr<IUnknown> > &aObjects,
                                            std::vector<ComPtr<IPerformanceMetric> > &aAffectedMetrics)
{
    pm::Filter filter(aMetricNames, aObjects);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS); /* Write lock is not needed atm since we are */
                                /* fiddling with enable bit only, but we */
                                /* care for those who come next :-). */

    HRESULT rc = S_OK;
    BaseMetricList filteredMetrics;
    BaseMetricList::iterator it;
    for (it = m.baseMetrics.begin(); it != m.baseMetrics.end(); ++it)
        if (filter.match((*it)->getObject(), (*it)->getName()))
        {
            rc = (*it)->enable();
            if (FAILED(rc))
                break;
            filteredMetrics.push_back(*it);
        }

    aAffectedMetrics.resize(filteredMetrics.size());
    int i = 0;
    for (it = filteredMetrics.begin();
         it != filteredMetrics.end() && SUCCEEDED(rc); ++it)
        rc = toIPerformanceMetric(*it, aAffectedMetrics[i++]);

    LogFlowThisFuncLeave();

    if (FAILED(rc))
        return setError(E_FAIL, "Failed to enable metrics for '%s'",
                        getFailedGuestName().c_str());
    return rc;
}

HRESULT PerformanceCollector::disableMetrics(const std::vector<com::Utf8Str> &aMetricNames,
                                             const std::vector<ComPtr<IUnknown> > &aObjects,
                                             std::vector<ComPtr<IPerformanceMetric> > &aAffectedMetrics)
{
    pm::Filter filter(aMetricNames, aObjects);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS); /* Write lock is not needed atm since we are */
                                /* fiddling with enable bit only, but we */
                                /* care for those who come next :-). */

    HRESULT rc = S_OK;
    BaseMetricList filteredMetrics;
    BaseMetricList::iterator it;
    for (it = m.baseMetrics.begin(); it != m.baseMetrics.end(); ++it)
        if (filter.match((*it)->getObject(), (*it)->getName()))
        {
            rc = (*it)->disable();
            if (FAILED(rc))
                break;
            filteredMetrics.push_back(*it);
        }

    aAffectedMetrics.resize(filteredMetrics.size());
    int i = 0;
    for (it = filteredMetrics.begin();
         it != filteredMetrics.end() && SUCCEEDED(rc); ++it)
        rc = toIPerformanceMetric(*it, aAffectedMetrics[i++]);

    LogFlowThisFuncLeave();

    if (FAILED(rc))
        return setError(E_FAIL, "Failed to disable metrics for '%s'",
                        getFailedGuestName().c_str());
    return rc;
}

HRESULT PerformanceCollector::queryMetricsData(const std::vector<com::Utf8Str> &aMetricNames,
                                               const std::vector<ComPtr<IUnknown> > &aObjects,
                                               std::vector<com::Utf8Str> &aReturnMetricNames,
                                               std::vector<ComPtr<IUnknown> > &aReturnObjects,
                                               std::vector<com::Utf8Str> &aReturnUnits,
                                               std::vector<ULONG> &aReturnScales,
                                               std::vector<ULONG> &aReturnSequenceNumbers,
                                               std::vector<ULONG> &aReturnDataIndices,
                                               std::vector<ULONG> &aReturnDataLengths,
                                               std::vector<LONG> &aReturnData)
{
    pm::Filter filter(aMetricNames, aObjects);

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    /* Let's compute the size of the resulting flat array */
    size_t flatSize = 0;
    MetricList filteredMetrics;
    MetricList::iterator it;
    for (it = m.metrics.begin(); it != m.metrics.end(); ++it)
        if (filter.match((*it)->getObject(), (*it)->getName()))
        {
            filteredMetrics.push_back(*it);
            flatSize += (*it)->getLength();
        }

    int i = 0;
    size_t flatIndex = 0;
    size_t numberOfMetrics = filteredMetrics.size();
    aReturnMetricNames.resize(numberOfMetrics);
    aReturnObjects.resize(numberOfMetrics);
    aReturnUnits.resize(numberOfMetrics);
    aReturnScales.resize(numberOfMetrics);
    aReturnSequenceNumbers.resize(numberOfMetrics);
    aReturnDataIndices.resize(numberOfMetrics);
    aReturnDataLengths.resize(numberOfMetrics);
    aReturnData.resize(flatSize);

    for (it = filteredMetrics.begin(); it != filteredMetrics.end(); ++it, ++i)
    {
        ULONG *values, length, sequenceNumber;
        /** @todo We may want to revise the query method to get rid of excessive alloc/memcpy calls. */
        (*it)->query(&values, &length, &sequenceNumber);
        LogFlow(("PerformanceCollector::QueryMetricsData() querying metric %s returned %d values.\n",
                 (*it)->getName(), length));
        memcpy(&aReturnData[flatIndex], values, length * sizeof(*values));
        RTMemFree(values);
        aReturnMetricNames[i] = (*it)->getName();
        aReturnObjects[i] = (*it)->getObject();
        aReturnUnits[i] = (*it)->getUnit();
        aReturnScales[i] = (*it)->getScale();
        aReturnSequenceNumbers[i] = sequenceNumber;
        aReturnDataIndices[i] = (ULONG)flatIndex;
        aReturnDataLengths[i] = length;
        flatIndex += length;
    }

    return S_OK;
}

// public methods for internal purposes
///////////////////////////////////////////////////////////////////////////////

void PerformanceCollector::registerBaseMetric(pm::BaseMetric *baseMetric)
{
    //LogFlowThisFuncEnter();
    AutoCaller autoCaller(this);
    if (!SUCCEEDED(autoCaller.rc())) return;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    Log7Func(("{%p}: obj=%p name=%s\n", this, (void *)baseMetric->getObject(), baseMetric->getName()));
    m.baseMetrics.push_back(baseMetric);
    //LogFlowThisFuncLeave();
}

void PerformanceCollector::registerMetric(pm::Metric *metric)
{
    //LogFlowThisFuncEnter();
    AutoCaller autoCaller(this);
    if (!SUCCEEDED(autoCaller.rc())) return;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    Log7Func(("{%p}: obj=%p name=%s\n", this, (void *)metric->getObject(), metric->getName()));
    m.metrics.push_back(metric);
    //LogFlowThisFuncLeave();
}

void PerformanceCollector::unregisterBaseMetricsFor(const ComPtr<IUnknown> &aObject, const Utf8Str name)
{
    //LogFlowThisFuncEnter();
    AutoCaller autoCaller(this);
    if (!SUCCEEDED(autoCaller.rc())) return;

    pm::Filter filter(name, aObject);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    int n = 0;
    BaseMetricList::iterator it;
    for (it = m.baseMetrics.begin(); it != m.baseMetrics.end(); ++it)
        if (filter.match((*it)->getObject(), (*it)->getName()))
        {
            (*it)->unregister();
            ++n;
        }
    Log7Func(("{%p}: obj=%p, name=%s, marked %d metrics\n", this, (void *)aObject, name.c_str(), n));
    //LogFlowThisFuncLeave();
}

void PerformanceCollector::unregisterMetricsFor(const ComPtr<IUnknown> &aObject, const Utf8Str name)
{
    //LogFlowThisFuncEnter();
    AutoCaller autoCaller(this);
    if (!SUCCEEDED(autoCaller.rc())) return;

    pm::Filter filter(name, aObject);

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    Log7Func(("{%p}: obj=%p, name=%s\n", this, (void *)aObject, name.c_str()));
    MetricList::iterator it;
    for (it = m.metrics.begin(); it != m.metrics.end();)
        if (filter.match((*it)->getObject(), (*it)->getName()))
        {
            delete *it;
            it = m.metrics.erase(it);
        }
        else
            ++it;
    //LogFlowThisFuncLeave();
}

void PerformanceCollector::registerGuest(pm::CollectorGuest* pGuest)
{
    AutoCaller autoCaller(this);
    if (!SUCCEEDED(autoCaller.rc())) return;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    m.gm->registerGuest(pGuest);
}

void PerformanceCollector::unregisterGuest(pm::CollectorGuest* pGuest)
{
    AutoCaller autoCaller(this);
    if (!SUCCEEDED(autoCaller.rc())) return;

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    m.gm->unregisterGuest(pGuest);
}

void PerformanceCollector::suspendSampling()
{
    AutoCaller autoCaller(this);
    if (!SUCCEEDED(autoCaller.rc())) return;

    int rc = RTTimerLRStop(m.sampler);
    if (   RT_FAILURE(rc)
        && rc != VERR_TIMER_SUSPENDED)     /* calling suspendSampling() successively shouldn't assert. See @bugref{3495}. */
        AssertMsgFailed(("PerformanceCollector::suspendSampling(): RTTimerLRStop returned %Rrc\n", rc));
}

void PerformanceCollector::resumeSampling()
{
    AutoCaller autoCaller(this);
    if (!SUCCEEDED(autoCaller.rc())) return;

    int rc = RTTimerLRStart(m.sampler, 0);
    if (   RT_FAILURE(rc)
        && rc != VERR_TIMER_ACTIVE)     /* calling resumeSampling() successively shouldn't assert. See @bugref{3495}. */
        AssertMsgFailed(("PerformanceCollector::resumeSampling(): RTTimerLRStart returned %Rrc\n", rc));
}


// private methods
///////////////////////////////////////////////////////////////////////////////

/* static */
DECLCALLBACK(void) PerformanceCollector::staticSamplerCallback(RTTIMERLR hTimerLR, void *pvUser,
                                                               uint64_t iTick)
{
    AssertReturnVoid(pvUser != NULL);
    PerformanceCollector *collector = static_cast <PerformanceCollector *> (pvUser);
    Assert(collector->mMagic == PERFORMANCE_METRIC_MAGIC);
    if (collector->mMagic == PERFORMANCE_METRIC_MAGIC)
        collector->samplerCallback(iTick);

    NOREF(hTimerLR);
}

/*
 * Metrics collection is a three stage process:
 * 1) Pre-collection (hinting)
 *    At this stage we compose the list of all metrics to be collected
 *    If any metrics cannot be collected separately or if it is more
 *    efficient to collect several metric at once, these metrics should
 *    use hints to mark that they will need to be collected.
 * 2) Pre-collection (bulk)
 *    Using hints set at stage 1 platform-specific HAL
 *    instance collects all marked host-related metrics.
 *    Hinted guest-related metrics then get collected by CollectorGuestManager.
 * 3) Collection
 *    Metrics that are collected individually get collected and stored. Values
 *    saved in HAL and CollectorGuestManager are extracted and stored to
 *    individual metrics.
 */
void PerformanceCollector::samplerCallback(uint64_t iTick)
{
    Log4Func(("{%p}: ENTER\n", this));
    /* No locking until stage 3!*/

    pm::CollectorHints hints;
    uint64_t timestamp = RTTimeMilliTS();
    BaseMetricList toBeCollected;
    BaseMetricList::iterator it;
    /* Compose the list of metrics being collected at this moment */
    for (it = m.baseMetrics.begin(); it != m.baseMetrics.end(); ++it)
        if ((*it)->collectorBeat(timestamp))
        {
            (*it)->preCollect(hints, iTick);
            toBeCollected.push_back(*it);
        }

    if (toBeCollected.size() == 0)
    {
        Log4Func(("{%p}: LEAVE (nothing to collect)\n", this));
        return;
    }

    /* Let know the platform specific code what is being collected */
    m.hal->preCollect(hints, iTick);
#if 0
    /* Guest stats are now pushed by guests themselves */
    /* Collect the data in bulk from all hinted guests */
    m.gm->preCollect(hints, iTick);
#endif

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    /*
     * Before we can collect data we need to go through both lists
     * again to see if any base metrics are marked as unregistered.
     * Those should be destroyed now.
     */
    Log7Func(("{%p}: before remove_if: toBeCollected.size()=%d\n", this, toBeCollected.size()));
    toBeCollected.remove_if(std::mem_fun(&pm::BaseMetric::isUnregistered));
    Log7Func(("{%p}: after remove_if: toBeCollected.size()=%d\n", this, toBeCollected.size()));
    Log7Func(("{%p}: before remove_if: m.baseMetrics.size()=%d\n", this, m.baseMetrics.size()));
    for (it = m.baseMetrics.begin(); it != m.baseMetrics.end();)
        if ((*it)->isUnregistered())
        {
            delete *it;
            it = m.baseMetrics.erase(it);
        }
        else
            ++it;
    Log7Func(("{%p}: after remove_if: m.baseMetrics.size()=%d\n", this, m.baseMetrics.size()));
    /*
     * Now when we have destroyed all base metrics that could
     * try to pull data from unregistered CollectorGuest objects
     * it is safe to destroy them as well.
     */
    m.gm->destroyUnregistered();

    /* Finally, collect the data */
    std::for_each(toBeCollected.begin(), toBeCollected.end(),
                  std::mem_fun(&pm::BaseMetric::collect));
    Log4Func(("{%p}: LEAVE\n", this));
}

////////////////////////////////////////////////////////////////////////////////
// PerformanceMetric class
////////////////////////////////////////////////////////////////////////////////

// constructor / destructor
////////////////////////////////////////////////////////////////////////////////

PerformanceMetric::PerformanceMetric()
{
}

PerformanceMetric::~PerformanceMetric()
{
}

HRESULT PerformanceMetric::FinalConstruct()
{
    LogFlowThisFunc(("\n"));

    return BaseFinalConstruct();
}

void PerformanceMetric::FinalRelease()
{
    LogFlowThisFunc(("\n"));

    uninit();

    BaseFinalRelease();
}

// public initializer/uninitializer for internal purposes only
////////////////////////////////////////////////////////////////////////////////

HRESULT PerformanceMetric::init(pm::Metric *aMetric)
{
    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    m.name        = aMetric->getName();
    m.object      = aMetric->getObject();
    m.description = aMetric->getDescription();
    m.period      = aMetric->getPeriod();
    m.count       = aMetric->getLength();
    m.unit        = aMetric->getUnit();
    m.min         = aMetric->getMinValue();
    m.max         = aMetric->getMaxValue();

    autoInitSpan.setSucceeded();
    return S_OK;
}

HRESULT PerformanceMetric::init(pm::BaseMetric *aMetric)
{
    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    m.name        = aMetric->getName();
    m.object      = aMetric->getObject();
    m.description = "";
    m.period      = aMetric->getPeriod();
    m.count       = aMetric->getLength();
    m.unit        = aMetric->getUnit();
    m.min         = aMetric->getMinValue();
    m.max         = aMetric->getMaxValue();

    autoInitSpan.setSucceeded();
    return S_OK;
}

void PerformanceMetric::uninit()
{
    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
    {
        LogFlowThisFunc(("Already uninitialized.\n"));
        LogFlowThisFuncLeave();
        return;
    }
}

HRESULT PerformanceMetric::getMetricName(com::Utf8Str &aMetricName)
{
    /* this is const, no need to lock */
    aMetricName = m.name;
    return S_OK;
}

HRESULT PerformanceMetric::getObject(ComPtr<IUnknown> &aObject)
{
    /* this is const, no need to lock */
    aObject = m.object;
    return S_OK;
}

HRESULT PerformanceMetric::getDescription(com::Utf8Str &aDescription)
{
    /* this is const, no need to lock */
    aDescription = m.description;
    return S_OK;
}

HRESULT PerformanceMetric::getPeriod(ULONG *aPeriod)
{
    /* this is const, no need to lock */
    *aPeriod = m.period;
    return S_OK;
}

HRESULT PerformanceMetric::getCount(ULONG *aCount)
{
    /* this is const, no need to lock */
    *aCount = m.count;
    return S_OK;
}

HRESULT PerformanceMetric::getUnit(com::Utf8Str &aUnit)
{
    /* this is const, no need to lock */
    aUnit = m.unit;
    return S_OK;
}

HRESULT PerformanceMetric::getMinimumValue(LONG *aMinimumValue)
{
    /* this is const, no need to lock */
    *aMinimumValue = m.min;
    return S_OK;
}

HRESULT PerformanceMetric::getMaximumValue(LONG *aMaximumValue)
{
    /* this is const, no need to lock */
    *aMaximumValue = m.max;
    return S_OK;
}
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
