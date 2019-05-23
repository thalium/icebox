/* $Id: shared_ptr.h $ */
/** @file
 * Simplified shared pointer.
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

#ifndef __SHARED_PTR_H__
#define __SHARED_PTR_H__

#ifdef __cplusplus
template<typename T>
class SharedPtr
{
    struct imp
    {
        imp(T *pTrg = NULL, int cnt = 1): ptr(pTrg),refcnt(cnt){}
        ~imp() { if (ptr) delete ptr;}

        T *ptr;
        int refcnt;
    };


    public:
    SharedPtr(T *t = NULL):p(NULL)
    {
        p = new imp(t);
    }

    ~SharedPtr()
    {
        p->refcnt--;

        if (p->refcnt == 0)
            delete p;
    }


    SharedPtr(const SharedPtr& rhs)
    {
        p = rhs.p;
        p->refcnt++;
    }

    const SharedPtr& operator= (const SharedPtr& rhs)
    {
        if (p == rhs.p) return *this;

        p->refcnt--;
        if (p->refcnt == 0)
            delete p;

        p = rhs.p;
        p->refcnt++;

        return *this;
    }


    T *get() const
    {
        return p->ptr;
    }


    T *operator->()
    {
        return p->ptr;
    }


    const T*operator->() const
    {
        return p->ptr;
    }


    int use_count()
    {
        return p->refcnt;
    }

    private:
    imp *p;
};
#endif

#endif
