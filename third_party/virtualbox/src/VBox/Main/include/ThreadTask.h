/** @file
 * VirtualBox ThreadTask class definition
 */

/*
 * Copyright (C) 2015-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ____H_THREADTASK
#define ____H_THREADTASK

#include "VBox/com/string.h"

/**
 * The class ThreadVoidData is used as a base class for any data which we want to pass into a thread
 */
struct ThreadVoidData
{
public:
    ThreadVoidData() { }
    virtual ~ThreadVoidData() { }
};


class ThreadTask
{
public:
    ThreadTask(const Utf8Str &t)
        : m_strTaskName(t)
        , mAsync(false)
    { }

    virtual ~ThreadTask()
    { }

    HRESULT createThread(void);
    HRESULT createThreadWithType(RTTHREADTYPE enmType);

    inline Utf8Str getTaskName() const { return m_strTaskName; }
    bool isAsync() { return mAsync; }

protected:
    HRESULT createThreadInternal(RTTHREADTYPE enmType);
    static DECLCALLBACK(int) taskHandlerThreadProc(RTTHREAD thread, void *pvUser);

    ThreadTask() : m_strTaskName("GenericTask")
    { }

    Utf8Str m_strTaskName;
    bool mAsync;

private:
    virtual void handler() = 0;
};

#endif

