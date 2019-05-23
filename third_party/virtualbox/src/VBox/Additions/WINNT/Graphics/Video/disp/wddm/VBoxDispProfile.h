/* $Id: VBoxDispProfile.h $ */
/** @file
 * VBoxVideo Display D3D User mode dll
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

#ifndef ___VBoxDispProfile_h__
#define ___VBoxDispProfile_h__
#include "VBoxDispD3DCmn.h"

#include <iprt/ctype.h>
#include <iprt/time.h>

#define VBOXDISPPROFILE_MAX_SETSIZE 512
#define VBOXDISPPROFILE_GET_TIME_NANO() RTTimeNanoTS()
#define VBOXDISPPROFILE_GET_TIME_MILLI() RTTimeMilliTS()
#define VBOXDISPPROFILE_DUMP(_m) do {\
        LOGREL_EXACT(_m); \
    } while (0)

class VBoxDispProfileEntry
{
public:
    VBoxDispProfileEntry() :
        m_cCalls(0),
        m_cTime(0),
        m_pName(NULL)
    {}

    VBoxDispProfileEntry(const char *pName) :
        m_cCalls(0),
        m_cTime(0),
        m_pName(pName)
    {}

    void step(uint64_t cTime)
    {
        ++m_cCalls;
        m_cTime+= cTime;
    }

    void reset()
    {
        m_cCalls = 0;
        m_cTime = 0;
    }

    uint64_t getTime() const
    {
        return m_cTime;
    }

    uint32_t getNumCalls() const
    {
        return m_cCalls;
    }

    const char* getName() const
    {
        return m_pName;
    }

    void dump(void *pvObj, uint64_t cTotalEntriesTime, uint64_t cTotalTime) const
    {
//        VBOXDISPPROFILE_DUMP((pDevice, "Entry '%s': calls(%d), time: nanos(%I64u), micros(%I64u), millis(%I64u)\n",
//                m_pName, m_cCalls,
//                m_cTime, m_cTime/1000, m_cTime/1000000));

//        VBOXDISPPROFILE_DUMP(("'%s' [0x%p]: \t%d\t%u\t%u\t%u\t%f\t%f", m_pName, pvObj,
//                m_cCalls,
//                (uint32_t)m_cTime, (uint32_t)(m_cTime/1000), (uint32_t)(m_cTime/1000000),
//                (((double)m_cTime)/cTotalEntriesTime),
//                (((double)m_cTime)/cTotalTime)));

        VBOXDISPPROFILE_DUMP(("'%s' [0x%p]: \t%d\t%u\t%f\t%f", m_pName, pvObj,
                m_cCalls,
                (uint32_t)(m_cTime/1000000),
                (((double)m_cTime)/cTotalEntriesTime),
                (((double)m_cTime)/cTotalTime)));

    }
private:
    uint32_t m_cCalls;
    uint64_t m_cTime;
    const char * m_pName;
};

class VBoxDispProfileSet
{
public:
    VBoxDispProfileSet(const char *pName) :
        m_cEntries(0),
        m_cIterations(0),
        m_pName(pName)
    {
        m_cTime = VBOXDISPPROFILE_GET_TIME_NANO();
    }

    VBoxDispProfileSet() :
        m_cEntries(0),
        m_cIterations(0),
        m_pName("global")
    {
        m_cTime = VBOXDISPPROFILE_GET_TIME_NANO();
    }

    VBoxDispProfileEntry * alloc(const char *pName)
    {
        if (m_cEntries < RT_ELEMENTS(m_Entries))
        {
            VBoxDispProfileEntry * entry = &m_Entries[m_cEntries];
            ++m_cEntries;
            *entry = VBoxDispProfileEntry(pName);
            return entry;
        }
        return NULL;
    }

    VBoxDispProfileEntry * get(uint32_t u32Entry, const char *pName)
    {
        if (u32Entry < RT_ELEMENTS(m_Entries))
        {
            VBoxDispProfileEntry * entry = &m_Entries[u32Entry];
            if (entry->getName())
                return entry;
            ++m_cEntries;
            *entry = VBoxDispProfileEntry(pName);
            return entry;
        }
        return NULL;
    }

    uint32_t reportIteration()
    {
        return ++m_cIterations;
    }

    uint32_t getNumIterations() const
    {
        return m_cIterations;
    }

    uint32_t getNumEntries() const
    {
        return m_cEntries;
    }

#define VBOXDISPPROFILESET_FOREACHENTRY(_op) \
        for (uint32_t i = 0, e = 0; e < m_cEntries && i < RT_ELEMENTS(m_Entries); ++i) { \
            if (m_Entries[i].getName()) { \
                { \
                _op  \
                } \
                ++e; \
            } \
        } \

    void resetEntries()
    {
        VBOXDISPPROFILESET_FOREACHENTRY(  m_Entries[i].reset(); );
        m_cTime = VBOXDISPPROFILE_GET_TIME_NANO();
    }

    void reset()
    {
        m_cEntries = 0;
        m_cTime = VBOXDISPPROFILE_GET_TIME_NANO();
    }

    void dump(void *pvObj)
    {
        uint64_t cEntriesTime = 0;
        VBOXDISPPROFILESET_FOREACHENTRY( cEntriesTime += m_Entries[i].getTime(); );

        VBOXDISPPROFILE_DUMP((">>>> '%s' [0x%p]: Start of VBox Disp Dump: num entries(%d), et(%u), tt(%u) >>>>>", m_pName, pvObj, m_cEntries,
                (uint32_t)(cEntriesTime / 1000000), (uint32_t)(m_cTime / 1000000)));
        VBOXDISPPROFILE_DUMP(("Name\tCalls\tNanos\tMicros\tMillis\tentries_quota\ttotal_quota"));
        VBOXDISPPROFILESET_FOREACHENTRY(
                if (m_Entries[i].getNumCalls())
                    m_Entries[i].dump(pvObj, cEntriesTime, m_cTime); );
        VBOXDISPPROFILE_DUMP(("<<<< '%s' [0x%p]: End of VBox Disp Dump <<<<<", m_pName, pvObj));
    }

private:
    VBoxDispProfileEntry m_Entries[VBOXDISPPROFILE_MAX_SETSIZE];
    uint32_t m_cEntries;
    uint32_t m_cIterations;
    uint64_t m_cTime;
    const char * m_pName;
};

class VBoxDispProfileDummyPostProcess
{
public:
    void postProcess(){}
};

template<typename T, typename P> class VBoxDispProfileScopeLogger
{
public:
    VBoxDispProfileScopeLogger(T *pEntry, P PostProcess) :
        m_pEntry(pEntry),
        m_PostProcess(PostProcess),
        m_bDisable(FALSE)
    {
        m_cTime = VBOXDISPPROFILE_GET_TIME_NANO();
    }

    ~VBoxDispProfileScopeLogger()
    {
        if (!m_bDisable)
        {
            logStep();
        }
    }

    void disable()
    {
        m_bDisable = TRUE;
    }

    void logAndDisable()
    {
        logStep();
        disable();
    }

private:
    void logStep()
    {
        m_PostProcess.postProcess();
        uint64_t cNewTime = VBOXDISPPROFILE_GET_TIME_NANO();
        m_pEntry->step(cNewTime - m_cTime);
    }
    T *m_pEntry;
    P m_PostProcess;
    uint64_t m_cTime;
    BOOL m_bDisable;
};


class VBoxDispProfileFpsCounter
{
public:
    VBoxDispProfileFpsCounter(uint32_t cPeriods)
    {
        init(cPeriods);
    }

    VBoxDispProfileFpsCounter()
    {
        memset(&m_Data, 0, sizeof (m_Data));
    }

    ~VBoxDispProfileFpsCounter()
    {
        term();
    }

    void term()
    {
        if (m_Data.mpaPeriods)
        {
            RTMemFree(m_Data.mpaPeriods);
            m_Data.mpaPeriods = NULL;
        }
        if (m_Data.mpaCalls)
        {
            RTMemFree(m_Data.mpaCalls);
            m_Data.mpaCalls = NULL;
        }
        if (m_Data.mpaTimes)
        {
            RTMemFree(m_Data.mpaTimes);
            m_Data.mpaTimes = NULL;
        }
        m_Data.mcPeriods = 0;
    }

    /* to be called in case fps counter was created with default constructor */
    void init(uint32_t cPeriods)
    {
        memset(&m_Data, 0, sizeof (m_Data));
        m_Data.mcPeriods = cPeriods;
        if (cPeriods)
        {
            m_Data.mpaPeriods = (uint64_t *)RTMemAllocZ(sizeof (m_Data.mpaPeriods[0]) * cPeriods);
            m_Data.mpaCalls = (uint32_t *)RTMemAllocZ(sizeof (m_Data.mpaCalls[0]) * cPeriods);
            m_Data.mpaTimes = (uint64_t *)RTMemAllocZ(sizeof (m_Data.mpaTimes[0]) * cPeriods);
        }
    }

    void ReportFrame()
    {
        uint64_t cur = VBOXDISPPROFILE_GET_TIME_NANO();

        if(m_Data.mPrevTime)
        {
            uint64_t curPeriod = cur - m_Data.mPrevTime;

            m_Data.mPeriodSum += curPeriod - m_Data.mpaPeriods[m_Data.miPeriod];
            m_Data.mpaPeriods[m_Data.miPeriod] = curPeriod;

            m_Data.mCallsSum += m_Data.mCurCalls - m_Data.mpaCalls[m_Data.miPeriod];
            m_Data.mpaCalls[m_Data.miPeriod] = m_Data.mCurCalls;

            m_Data.mTimeUsedSum += m_Data.mCurTimeUsed - m_Data.mpaTimes[m_Data.miPeriod];
            m_Data.mpaTimes[m_Data.miPeriod] = m_Data.mCurTimeUsed;

            ++m_Data.miPeriod;
            m_Data.miPeriod %= m_Data.mcPeriods;
        }
        m_Data.mPrevTime = cur;
        ++m_Data.mcFrames;

        m_Data.mCurTimeUsed = 0;
        m_Data.mCurCalls = 0;
    }

    void step(uint64_t Time)
    {
        m_Data.mCurTimeUsed += Time;
        ++m_Data.mCurCalls;
    }

    uint64_t GetEveragePeriod()
    {
        return m_Data.mPeriodSum / m_Data.mcPeriods;
    }

    double GetFps()
    {
        return ((double)1000000000.0) / GetEveragePeriod();
    }

    double GetCps()
    {
        return GetFps() * m_Data.mCallsSum / m_Data.mcPeriods;
    }

    double GetTimeProcPercent()
    {
        return 100.0*m_Data.mTimeUsedSum/m_Data.mPeriodSum;
    }

    uint64_t GetNumFrames()
    {
        return m_Data.mcFrames;
    }
private:
    struct
    {
        uint64_t mPeriodSum;
        uint64_t *mpaPeriods;
        uint64_t mPrevTime;
        uint64_t mcFrames;
        uint32_t mcPeriods;
        uint32_t miPeriod;

        uint64_t mCallsSum;
        uint32_t *mpaCalls;

        uint64_t mTimeUsedSum;
        uint64_t *mpaTimes;

        uint64_t mCurTimeUsed;
        uint64_t mCurCalls;
    } m_Data;
};

#define VBOXDISPPROFILE_FUNCTION_LOGGER_DISABLE_CURRENT() do { \
        __vboxDispProfileFunctionLogger.disable();\
    } while (0)

#define VBOXDISPPROFILE_FUNCTION_LOGGER_LOG_AND_DISABLE_CURRENT() do { \
        __vboxDispProfileFunctionLogger.logAndDisable();\
    } while (0)

#ifdef VBOXDISPPROFILE_FUNCTION_LOGGER_GLOBAL_PROFILE
# define VBOXDISPPROFILE_FUNCTION_LOGGER_DEFINE(_p, _T, _v)  \
        static VBoxDispProfileEntry * __pVBoxDispProfileEntry = NULL; \
        if (!__pVBoxDispProfileEntry) { __pVBoxDispProfileEntry = _p.alloc(__FUNCTION__); } \
        VBoxDispProfileScopeLogger<VBoxDispProfileEntry, _T> __vboxDispProfileFunctionLogger(__pVBoxDispProfileEntry, _v);
#else
# ifndef VBOXDISPPROFILE_FUNCTION_LOGGER_INDEX_GEN
#  error "VBOXDISPPROFILE_FUNCTION_LOGGER_INDEX_GEN should be fedined!"
# endif
# define VBOXDISPPROFILE_FUNCTION_LOGGER_DEFINE(_p, _T, _v)  \
        static uint32_t __u32VBoxDispProfileIndex = VBOXDISPPROFILE_FUNCTION_LOGGER_INDEX_GEN(); \
        VBoxDispProfileEntry * __pVBoxDispProfileEntry = _p.get(__u32VBoxDispProfileIndex, __FUNCTION__); \
        VBoxDispProfileScopeLogger<VBoxDispProfileEntry, _T> __vboxDispProfileFunctionLogger(__pVBoxDispProfileEntry, _v);
#endif

#define VBOXDISPPROFILE_STATISTIC_LOGGER_DISABLE_CURRENT() do { \
        __vboxDispProfileStatisticLogger.disable();\
    } while (0)

#define VBOXDISPPROFILE_STATISTIC_LOGGER_LOG_AND_DISABLE_CURRENT() do { \
        __vboxDispProfileStatisticLogger.logAndDisable();\
    } while (0)


#define VBOXDISPPROFILE_STATISTIC_LOGGER_DEFINE(_p, _T, _v)  \
        VBoxDispProfileScopeLogger<VBoxDispProfileFpsCounter, _T> __vboxDispProfileStatisticLogger(_p, _v);

//#define VBOXDISPPROFILE_FUNCTION_PROLOGUE(_p) \
//        VBOXDISPPROFILE_FUNCTION_LOGGER_DEFINE(_p)

#endif /* #ifndef ___VBoxDispProfile_h__ */
