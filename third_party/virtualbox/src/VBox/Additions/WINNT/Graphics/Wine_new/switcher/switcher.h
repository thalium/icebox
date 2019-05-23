/* $Id: switcher.h $ */
/** @file
 * VBox D3D8/9 dll switcher
 */

/*
 * Copyright (C) 2009-2017 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef ___CROPENGL_SWITCHER_H_
#define ___CROPENGL_SWITCHER_H_

typedef BOOL (APIENTRY *DrvValidateVersionProc)(DWORD version);

#define SW_FILLPROC(dispatch, hdll, name) \
    dispatch.p##name = ((hdll) != NULL) ? (name##Proc) GetProcAddress((hdll), #name) : vbox##name##Stub;

#define SW_DISPINIT(dispatch)                                   \
    {                                                           \
        if (!dispatch.initialized)                              \
        {                                                       \
            InitD3DExports(dispatch.vboxName, dispatch.msName); \
            dispatch.initialized = 1;                           \
        }                                                       \
    }

#define SW_CHECKRET(dispatch, func, failret)   \
    {                                          \
        SW_DISPINIT(dispatch)                  \
        if (!dispatch.p##func)                 \
            return failret;                    \
    }

#define SW_CHECKCALL(dispatch, func)    \
    {                                   \
        SW_DISPINIT(dispatch)           \
        if (!dispatch.p##func) return;  \
    }

extern BOOL IsVBox3DEnabled(void);
extern BOOL CheckOptions(void);
extern void FillD3DExports(HANDLE hDLL);
extern void InitD3DExports(const char *vboxName, const char *msName);

#endif /* #ifndef ___CROPENGL_SWITCHER_H_ */
